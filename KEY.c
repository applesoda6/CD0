#include "KEY.h"
#include "POWER.h"
#include "CD.h"
#include <string.h>

/* 外部邮箱（接收方） */
extern osMailQId g_mailq_power;
extern osMailQId g_mailq_cd;

/* 内部状态 */
static KEY_STATUS g_keys[KEY_NUM_KEYS];
static int8_t     g_active_idx = -1;

/* 读取硬件原始状态 */
static inline uint8_t Key_ReadRaw(key_index_t idx)
{
    switch (idx) {
    case KEY_IDX_WKUP:
        return (GPIO_ReadInputDataBit(PIN_WK_UP_PORT, PIN_WK_UP_PIN) != Bit_RESET) ? 1u : 0u;
    case KEY_IDX_KEY0:
        return (GPIO_ReadInputDataBit(PIN_KEY0_PORT, PIN_KEY0_PIN) == Bit_RESET) ? 1u : 0u;
    case KEY_IDX_KEY1:
        return (GPIO_ReadInputDataBit(PIN_KEY1_PORT, PIN_KEY1_PIN) == Bit_RESET) ? 1u : 0u;
    default:
        return 0u;
    }
}

/* 统一发送工具：向接收方邮箱分配并投递 */
static inline void Key_Post(osMailQId q, uint8_t evt, uint32_t a, uint32_t b, uint32_t c, uint8_t target)
{
    if (!q) return;
    MsgMail_t* m = (MsgMail_t*)osMailAlloc(q, 0);
    if (!m) return;
    memset(m, 0, sizeof(*m));
    m->eventID = evt;       /* 必须是接收方定义的事件号 */
    m->scourceID = MOD_KEY;
    m->targetID = target;
    m->option[0] = a; m->option[1] = b; m->option[2] = c;
    osMailPut(q, m);
}

/* 上报短/长按事件 */
static void Key_RaiseEvent(key_index_t idx, key_event_t ev)
{
    if (ev == KEVT_None) return;

    switch (idx) {
    case KEY_IDX_WKUP:
        if (ev == KEVT_Short) {
            Key_Post(g_mailq_power, KEY_RECEIVE_ON, 0, 0, 0, MOD_POWER);
        }
        else { /* Long */
            Key_Post(g_mailq_power, KEY_RECEIVE_OFF, 0, 0, 0, MOD_POWER);
        }
        break;

    case KEY_IDX_KEY0:
        if (ev == KEVT_Short) {
            Key_Post(g_mailq_cd, EVT_KEY_LOAD_EJECT, 0, 0, 0, MOD_CD);
        }
        else { /* Long → Prev 长按开始 */
            Key_Post(g_mailq_cd, EVT_KEY_PREV_START, 0, 0, 0, MOD_CD);
        }
        break;

    case KEY_IDX_KEY1:
        if (ev == KEVT_Short) {
            Key_Post(g_mailq_cd, EVT_KEY_PLAY_PAUSE, 0, 0, 0, MOD_CD);
        }
        else { /* Long → Next 长按开始 */
            Key_Post(g_mailq_cd, EVT_KEY_NEXT_START, 0, 0, 0, MOD_CD);
        }
        break;

    default:
        break;
    }
}

/* 10ms 扫描 */
static void Key_ScanEvery10ms(void)
{
    int i;
    for (i = 0; i < (int)KEY_NUM_KEYS; ++i) {
        uint8_t raw = Key_ReadRaw((key_index_t)i);

        if (raw == g_keys[i].last_raw) {
            if (g_keys[i].stable_cnt < 0xFFu) g_keys[i].stable_cnt++;
        }
        else {
            g_keys[i].stable_cnt = 0u;
            g_keys[i].last_raw = raw;
        }

        if (g_keys[i].stable_cnt == (uint8_t)KEY_DEBOUNCE_TICKS) {
            if (raw != 0u) {
                /* press confirmed */
                if (g_active_idx != i) {
                    if ((g_active_idx >= 0) && (g_keys[g_active_idx].state == KEY_STATE_Long)) {
                        /* 另一键长按切换释放：补 STOP */
                        if (g_active_idx == KEY_IDX_KEY0) {
                            Key_Post(g_mailq_cd, EVT_KEY_PREV_STOP, 0, 0, 0, MOD_CD);
                        }
                        else if (g_active_idx == KEY_IDX_KEY1) {
                            Key_Post(g_mailq_cd, EVT_KEY_NEXT_STOP, 0, 0, 0, MOD_CD);
                        }
                        g_keys[g_active_idx].state = KEY_STATE_Release;
                    }
                    g_active_idx = (int8_t)i;
                    g_keys[i].press_ticks = 0u;
                    g_keys[i].state = KEY_STATE_Press;
                    g_keys[i].event = KEVT_None;
                }
            }
            else {
                /* release confirmed */
                if (g_active_idx == i) {
                    if (g_keys[i].state == KEY_STATE_Press) {
                        /* 短按 */
                        g_keys[i].event = KEVT_Short;
                        Key_RaiseEvent((key_index_t)i, g_keys[i].event);
                        g_keys[i].event = KEVT_None;
                        g_keys[i].state = KEY_STATE_Release;
                    }
                    else if (g_keys[i].state == KEY_STATE_Long) {
                        /* 长按释放：发 STOP */
                        if (i == KEY_IDX_KEY0) {
                            Key_Post(g_mailq_cd, EVT_KEY_PREV_STOP, 0, 0, 0, MOD_CD);
                        }
                        else if (i == KEY_IDX_KEY1) {
                            Key_Post(g_mailq_cd, EVT_KEY_NEXT_STOP, 0, 0, 0, MOD_CD);
                        }
                        g_keys[i].state = KEY_STATE_Release;
                    }
                    else {
                        g_keys[i].state = KEY_STATE_Release;
                    }
                    g_active_idx = -1;
                }
            }
        }
    }

    if (g_active_idx >= 0) {
        KEY_STATUS* k = &g_keys[g_active_idx];

        if ((k->state == KEY_STATE_Press) || (k->state == KEY_STATE_Long)) {
            if (k->press_ticks < 0xFFFFu) k->press_ticks++;

            if ((k->state == KEY_STATE_Press) && (k->press_ticks >= (uint16_t)KEY_LONG_TICKS)) {
                k->state = KEY_STATE_Long;
                k->event = KEVT_Long;
                Key_RaiseEvent((key_index_t)g_active_idx, k->event);   /* 发送 START */
                k->event = KEVT_None;
            }
        }
    }
}

/* 线程 */
static void Thread_KEY(void const* argument)
{
    (void)argument;
    int i;
    for (i = 0; i < (int)KEY_NUM_KEYS; ++i) {
        g_keys[i].stable_cnt = 0u;
        g_keys[i].press_ticks = 0u;
        g_keys[i].last_raw = 0u;
        g_keys[i].state = KEY_STATE_Release;
        g_keys[i].event = KEVT_None;
    }

    for (;;) {
        Key_ScanEvery10ms();
        osDelay((uint32_t)KEY_SCAN_PERIOD_MS);
    }
}

osThreadDef(Thread_KEY, osPriorityAboveNormal, 1, 256);

/* 公共 API */
void KEY_InitGPIO(void)
{
    GPIO_InitTypeDef gi;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);

    gi.GPIO_Pin = PIN_WK_UP_PIN; gi.GPIO_Mode = GPIO_Mode_IPD; gi.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(PIN_WK_UP_PORT, &gi);

    gi.GPIO_Pin = PIN_KEY0_PIN;  gi.GPIO_Mode = GPIO_Mode_IPU; gi.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(PIN_KEY0_PORT, &gi);

    gi.GPIO_Pin = PIN_KEY1_PIN;  gi.GPIO_Mode = GPIO_Mode_IPU; gi.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(PIN_KEY1_PORT, &gi);
}

void TSK_KEY_Start(void)
{
    osThreadCreate(osThread(Thread_KEY), NULL);
}
