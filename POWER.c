#include "POWER.h"
#include "CD.h"
#include "OLED.h"
#include <string.h>

/* ---------------- RTOS 资源：POWER 接收邮箱 + 线程 + 定时器 ---------------- */
osMailQDef(POWER_MAILQ, 16, MsgMail_t);
osMailQId g_mailq_power = NULL;

static osTimerId    PowertimerGuardId;
static osThreadId   PowerId;

/* ---------------- 全局状态保持 ---------------- */
POWER_PROCESS_STAGE g_CurPowerProcessStage = POWER_PROCESS_STAGE_0;
POWER_MODE          g_CurPowerMode = POWER_MODE_OFF;

typedef struct {
    uint8_t cd_power_state;    /* 0=未就绪,1=就绪 */
    uint8_t oled_power_state;  /* 0=未就绪,1=就绪 */
} ModulePowerStates;

static ModulePowerStates g_module_states = { 0, 0 };
static volatile POWER_RECV_MSG g_lastPowerEvent = KEY_RECEIVE_ON;

/* ---------------- 转移/动作矩阵 ---------------- */
const POWER_PROCESS_STAGE POWER_TransitionMatrix[POWER_RECV_MSG_END][POWER_MODE_END] = {
    /* KEY_RECEIVE_ON            */ {POWER_PROCESS_STAGE_2, POWER_PROCESS_STAGE_1, POWER_PROCESS_STAGE_1, POWER_PROCESS_STAGE_2},
    /* KEY_RECEIVE_OFF           */ {POWER_PROCESS_STAGE_1, POWER_PROCESS_STAGE_0, POWER_PROCESS_STAGE_0, POWER_PROCESS_STAGE_1},
    /* CD_ON_RECEIVE_OK          */ {POWER_PROCESS_STAGE_1, POWER_PROCESS_STAGE_1, POWER_PROCESS_STAGE_3, POWER_PROCESS_STAGE_1},
    /* CD_OFF_RECEIVE_OK         */ {POWER_PROCESS_STAGE_1, POWER_PROCESS_STAGE_1, POWER_PROCESS_STAGE_1, POWER_PROCESS_STAGE_3},
    /* OLED_ON_RECEIVE_OK        */ {POWER_PROCESS_STAGE_1, POWER_PROCESS_STAGE_1, POWER_PROCESS_STAGE_3, POWER_PROCESS_STAGE_1},
    /* OLED_OFF_RECEIVE_OK       */ {POWER_PROCESS_STAGE_1, POWER_PROCESS_STAGE_1, POWER_PROCESS_STAGE_1, POWER_PROCESS_STAGE_3},
    /* ALL_RECEIVE_OK            */ {POWER_PROCESS_STAGE_1, POWER_PROCESS_STAGE_1, POWER_PROCESS_STAGE_4, POWER_PROCESS_STAGE_5},
    /* TIMER_OUT                 */ {POWER_PROCESS_STAGE_1, POWER_PROCESS_STAGE_1, POWER_PROCESS_STAGE_6, POWER_PROCESS_STAGE_6},
    /* ERROR_OUT_TRYCOUNT_THREE  */ {POWER_PROCESS_STAGE_1, POWER_PROCESS_STAGE_1, POWER_PROCESS_STAGE_7, POWER_PROCESS_STAGE_7},
};

typedef void    (*SetPowerModeFunc)(void);
typedef void    (*VoidFunc)(void);

typedef struct {
    SetPowerModeFunc set_power_mode;
    VoidFunc         act1;
    VoidFunc         act2;
} PowerStageAction;

/* 前置声明（动作） */
static void startPowerCheckTimer(void);
static void stopPowerCheckTimer(void);
static void PowertimerGuard_Reset(void);
static void saveModuleState(void);
static void checkAllModulesReady(void);
static void sendErrorToAllModule(void);
static void sendPowerTimeoutError(void);
static void sendAllModulePowerOn(void);
static void sendAllModulePowerOff(void);

static void SetPowerMode_On(void) { g_CurPowerMode = POWER_MODE_ON; }
static void SetPowerMode_Off(void) { g_CurPowerMode = POWER_MODE_OFF; }
static void SetPowerMode_Oning(void) { g_CurPowerMode = POWER_MODE_ONING; }
static void SetPowerMode_Offing(void) { g_CurPowerMode = POWER_MODE_OFFING; }

static const PowerStageAction POWER_ActionMatrix[POWER_PROCESS_STAGE_END] = {
    /*0 关机中 */ {SetPowerMode_Offing, startPowerCheckTimer, sendAllModulePowerOff},
    /*1 空     */ {NULL,                NULL,                 NULL},
    /*2 开机中 */ {SetPowerMode_Oning,  startPowerCheckTimer, sendAllModulePowerOn},
    /*3 收集OK */ {NULL,                saveModuleState,      checkAllModulesReady},
    /*4 开机好 */ {SetPowerMode_On,     stopPowerCheckTimer,  NULL},
    /*5 关机好 */ {SetPowerMode_Off,    stopPowerCheckTimer,  NULL},
    /*6 超时   */ {NULL,                sendPowerTimeoutError, PowertimerGuard_Reset},
    /*7 重试满 */ {SetPowerMode_Off,    sendErrorToAllModule, NULL},
};

/* ---------------- 工具：向 OLED / CD 发信 ---------------- */
extern osMailQId g_mailq_oled;
extern osMailQId g_mailq_cd;

static inline void power_send_to_oled(uint8_t evt, uint32_t a, uint32_t b, uint32_t c)
{
    if (!g_mailq_oled) return;
    MsgMail_t* m = (MsgMail_t*)osMailAlloc(g_mailq_oled, 0);
    if (!m) return;
    memset(m, 0, sizeof(*m));
    m->eventID = evt;          /* OLED.h 定义 */
    m->scourceID = MOD_POWER;
    m->targetID = MOD_OLED;
    m->option[0] = a; m->option[1] = b; m->option[2] = c;
    osMailPut(g_mailq_oled, m);
}

static inline void power_send_to_cd(uint8_t evt, uint32_t a, uint32_t b, uint32_t c)
{
    if (!g_mailq_cd) return;
    MsgMail_t* m = (MsgMail_t*)osMailAlloc(g_mailq_cd, 0);
    if (!m) return;
    memset(m, 0, sizeof(*m));
    m->eventID = evt;          /* CD.h 定义（如 EVT_PWR_ON/OFF） */
    m->scourceID = MOD_POWER;
    m->targetID = MOD_CD;
    m->option[0] = a; m->option[1] = b; m->option[2] = c;
    osMailPut(g_mailq_cd, m);
}

/* ---------------- 向 POWER 自投递（兼容旧签名） ---------------- */
int SendToPower(uint8_t sender, uint8_t receiver, POWER_RECV_MSG event, const char* option[3])
{
    (void)receiver;
    if (!g_mailq_power) return -1;
    MsgMail_t* m = (MsgMail_t*)osMailAlloc(g_mailq_power, 0);
    if (!m) return -2;
    memset(m, 0, sizeof(*m));
    m->eventID = (uint8_t)event;
    m->scourceID = (uint32_t)sender;
    m->targetID = MOD_POWER;
    if (option) {
        m->option[0] = (uint32_t)option[0];
        m->option[1] = (uint32_t)option[1];
        m->option[2] = (uint32_t)option[2];
    }
    return (osMailPut(g_mailq_power, m) == osOK) ? 0 : -3;
}

/* ---------------- 定时器 ---------------- */
static void PowertimerGuard_Callback(void const* arg)
{
    (void)arg;
    const char* opt[3] = { 0 };
    (void)opt;
    SendToPower(MOD_POWER, MOD_POWER, TIMER_OUT, opt);
}
static void startPowerCheckTimer(void) { osTimerStart(PowertimerGuardId, 50); }
static void stopPowerCheckTimer(void) { osTimerStop(PowertimerGuardId); }

static void PowertimerGuard_Reset(void)
{
    static uint8_t retry = 0;
    if (retry < 3) {
        osTimerStop(PowertimerGuardId);
        osTimerStart(PowertimerGuardId, 50);
        retry++;
    }
    else {
        const char* opt[3] = { 0 };
        (void)opt;
        SendToPower(MOD_POWER, MOD_POWER, ERROR_OUT_TRYCOUNT_THREE, opt);
        retry = 0;
    }
}

/* ---------------- 与其他模块交互（动作里会调用） ---------------- */
static void sendAllModulePowerOn(void)
{
    g_module_states.cd_power_state = 0;
    g_module_states.oled_power_state = 0;

    power_send_to_oled(OLED_TURN_ON, 0, 0, 0);
    power_send_to_cd(EVT_PWR_ON, 0, 0, 0);
}

static void sendAllModulePowerOff(void)
{
    g_module_states.cd_power_state = 1;
    g_module_states.oled_power_state = 1;

    power_send_to_oled(OLED_TURN_OFF, 0, 0, 0);
    power_send_to_cd(EVT_PWR_OFF, 0, 0, 0);
}


static void sendErrorToAllModule(void)
{
    power_send_to_oled(OLED_TURNER_ERROR, 0, 0, 0);
    /* 若需要 CD 错误事件，可扩展 */
}

static void sendPowerTimeoutError(void)
{
    if (!g_module_states.cd_power_state)   power_send_to_cd(EVT_PWR_OFF, 0, 0, 0);
    if (!g_module_states.oled_power_state) power_send_to_oled(OLED_TURNER_ERROR, 0, 0, 0);
}

static uint8_t isAllModuleOK(void)
{
    return (g_module_states.cd_power_state && g_module_states.oled_power_state) ? 1u : 0u;
}

static void saveModuleState(void)
{
    POWER_RECV_MSG r = g_lastPowerEvent;
    switch (r) {
    case CD_ON_RECEIVE_OK:    if (g_CurPowerMode == POWER_MODE_ONING)  g_module_states.cd_power_state = 1; break;
    case CD_OFF_RECEIVE_OK:   if (g_CurPowerMode == POWER_MODE_OFFING) g_module_states.cd_power_state = 0; break;
    case OLED_ON_RECEIVE_OK:  if (g_CurPowerMode == POWER_MODE_ONING)  g_module_states.oled_power_state = 1; break;
    case OLED_OFF_RECEIVE_OK: if (g_CurPowerMode == POWER_MODE_OFFING) g_module_states.oled_power_state = 0; break;
    default: break;
    }
}

static void checkAllModulesReady(void)
{
    if (isAllModuleOK()) {
        const char* opt[3] = { 0 };
        (void)opt;
        SendToPower(MOD_POWER, MOD_POWER, ALL_RECEIVE_OK, opt);
    }
}

/* ---------------- 状态机驱动 ---------------- */
static POWER_PROCESS_STAGE nextStage(POWER_RECV_MSG r, POWER_MODE m)
{
    if (r >= POWER_RECV_MSG_END || m >= POWER_MODE_END) return g_CurPowerProcessStage;
    return POWER_TransitionMatrix[r][m];
}

static void doActions(POWER_PROCESS_STAGE s)
{
    if (s >= POWER_PROCESS_STAGE_END) return;
    {
        const PowerStageAction* a = &POWER_ActionMatrix[s];
        if (a->set_power_mode) a->set_power_mode();
        if (a->act1)           a->act1();
        if (a->act2)           a->act2();
    }
}

static void ChangeModelState(POWER_RECV_MSG r)
{
    POWER_PROCESS_STAGE ns = nextStage(r, g_CurPowerMode);
    if (ns != g_CurPowerProcessStage) {
        g_lastPowerEvent = r;
        doActions(ns);
        g_CurPowerProcessStage = ns;
    }
}

/* ---------------- 线程/初始化 ---------------- */
static void Thread_POWER(void const* arg)
{
    (void)arg;
    for (;;) {
        osEvent ev = osMailGet(g_mailq_power, osWaitForever);
        if (ev.status != osEventMail) continue;
        {
            MsgMail_t* m = (MsgMail_t*)ev.value.p;
            if (m && m->targetID == MOD_POWER) {
                ChangeModelState((POWER_RECV_MSG)m->eventID);
            }
            osMailFree(g_mailq_power, m);
        }
    }
}

void PowerThreadInit(void)
{
    g_mailq_power = osMailCreate(osMailQ(POWER_MAILQ), NULL);

    osTimerDef(PowertimerGuard, PowertimerGuard_Callback);
    PowertimerGuardId = osTimerCreate(osTimer(PowertimerGuard), osTimerOnce, NULL);

    osThreadDef(PowerThread, osPriorityHigh, 1,2000);
    PowerId = osThreadCreate(osThread(PowerThread), NULL);
	power_send_to_oled(OLED_TURN_ON, 0, 0, 0);
    (void)PowerId;
}
