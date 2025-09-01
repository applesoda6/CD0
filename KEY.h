#ifndef __KEY_H__
#define __KEY_H__

#include "cmsis_os.h"
#include "stm32f10x.h"

/* ----------------- 硬件映射 ----------------- */
#define PIN_WK_UP_PORT      GPIOA
#define PIN_WK_UP_PIN       GPIO_Pin_0      /* WK_UP: PA0, 高有效 */

#define PIN_KEY0_PORT       GPIOC
#define PIN_KEY0_PIN        GPIO_Pin_1      /* KEY0 : PC1, 低有效 */

#define PIN_KEY1_PORT       GPIOC
#define PIN_KEY1_PIN        GPIO_Pin_13     /* KEY1 : PC13, 低有效 */

/* ----------------- KEY 扫描参数 ----------------- */
#define KEY_SCAN_PERIOD_MS  (10)
#define KEY_DEBOUNCE_TICKS  (3)      /* 3*10ms = 30ms */
#define KEY_LONG_TICKS      (170)    /* 170*10ms = 1.7s */

/* ----------------- KEY 内部类型 ----------------- */
typedef enum
{
    KEY_IDX_WKUP = 0,
    KEY_IDX_KEY0 = 1,
    KEY_IDX_KEY1 = 2,
    KEY_NUM_KEYS = 3
} key_index_t;

typedef enum
{
    KEY_STATE_Release = 0,  /* idle */
    KEY_STATE_Press = 1,  /* pressed < long threshold */
    KEY_STATE_Long = 2,  /* long pressed */
    KEY_STATE_LongRel = 3   /* long released (一周期标志) */
} key_state_t;

typedef enum
{
    KEVT_None = 0,
    KEVT_Short = 1,
    KEVT_Long = 2
} key_event_t;

typedef struct
{
    uint8_t      stable_cnt;
    uint16_t     press_ticks;
    uint8_t      last_raw;
    key_state_t  state;
    key_event_t  event;
} KEY_STATUS;

/* ----------------- 对外 API ----------------- */
#ifdef __cplusplus
extern "C" {
#endif

    void KEY_InitGPIO(void);
    void TSK_KEY_Start(void);

#ifdef __cplusplus
}
#endif

#endif /* __KEY_H__ */
