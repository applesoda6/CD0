#ifndef _THREAD_POWER_H
#define _THREAD_POWER_H

#include "cmsis_os.h"
#include "CD.h"
#include "OLED.h"

/* POWER 线程导出名（可按需） */
#define PowerThread  Thread_POWER

/* ===================== POWER 接收事件（谁接收谁定义） ===================== */
typedef enum {
    KEY_RECEIVE_ON = 0,         /* 来自 KEY：开机请求 */
    KEY_RECEIVE_OFF,            /* 来自 KEY：关机请求 */
    CD_ON_RECEIVE_OK,           /* 来自 CD：开机完成 */
    CD_OFF_RECEIVE_OK,          /* 来自 CD：关机完成 */
    OLED_ON_RECEIVE_OK,         /* 来自 OLED：开机完成 */
    OLED_OFF_RECEIVE_OK,        /* 来自 OLED：关机完成 */
    ALL_RECEIVE_OK,             /* 内部：所有模块就绪 */
    TIMER_OUT,                  /* 内部：超时 */
    ERROR_OUT_TRYCOUNT_THREE,   /* 内部：重试达上限 */
    POWER_RECV_MSG_END
} POWER_RECV_MSG;

/* 对外动作标签（语义用，真正发信用接收方事件号） */
typedef enum {
    SEND_OLED_POWERON = 0,
    SEND_OLED_POWEROFF,
    SEND_OLED_POWERERROR,
    SEND_CD_POWERON,
    SEND_CD_POWEROFF,
    SEND_CD_POWERERROR,
    MSG_POWER_SEND_END
} POWER_SEND_MSG;

/* POWER 过程阶段/模式（内部状态机） */
typedef enum {
    POWER_PROCESS_STAGE_0 = 0,  /* 触发关机中 */
    POWER_PROCESS_STAGE_1,      /* 空阶段 */
    POWER_PROCESS_STAGE_2,      /* 触发开机中 */
    POWER_PROCESS_STAGE_3,      /* 模块状态处理 */
    POWER_PROCESS_STAGE_4,      /* 开机完成 */
    POWER_PROCESS_STAGE_5,      /* 关机完成 */
    POWER_PROCESS_STAGE_6,      /* 超时错误 */
    POWER_PROCESS_STAGE_7,      /* 重试上限 */
    POWER_PROCESS_STAGE_END
} POWER_PROCESS_STAGE;

typedef enum {
    POWER_MODE_ON = 0,
    POWER_MODE_OFF,
    POWER_MODE_ONING,
    POWER_MODE_OFFING,
    POWER_MODE_END
} POWER_MODE;

/* ===================== 对外资源与函数 ===================== */
#ifdef __cplusplus
extern "C" {
#endif

    extern osMailQId g_mailq_power;                    /* POWER 接收邮箱 */
    extern POWER_PROCESS_STAGE g_CurPowerProcessStage; /* 当前阶段 */
    extern POWER_MODE          g_CurPowerMode;         /* 当前模式 */

    /* 线程/初始化 */
    void Thread_POWER(void const* arg);
    void PowerThreadInit(void);

    /* 兼容旧接口（OLED ACK/内部触发用）：向 POWER 投递 */
    int  SendToPower(uint8_t sender, uint8_t receiver, POWER_RECV_MSG event, const char* option[3]);

#ifdef __cplusplus
}
#endif

#endif /* _THREAD_POWER_H */
