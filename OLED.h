#ifndef _THREAD_OLEDSHOW_H
#define _THREAD_OLEDSHOW_H

#include "cmsis_os.h"
#include "OLED_driver.h"
#include "CD.h"    /* 为了使用 MsgMail_t / MOD_* */

/* OLED 接收的事件（谁接收谁定义） */
typedef enum
{
    NO_DISC = 0x00,
    LOADING = 0x01,
    EJECTING = 0x02,
    PLAY = 0x03,
    PAUSE = 0x04,
    PREVIOUS = 0x05,
    NEXT = 0x06,
    STOP = 0x07,

    OLED_TURN_ON = 0x08,        /* 电源模块给 OLED：开屏 */
    OLED_TURN_OFF = 0x09,       /* 电源模块给 OLED：关屏 */
    OLED_TURNER_ERROR           /* 电源模块给 OLED：电源错误 */
} ToOledMsg;

/* OLED 接收邮箱（在 OLED.c 创建） */
#ifdef __cplusplus
extern "C" {
#endif

    extern osMailQId g_mailq_oled;

    /* OLED 线程与行为接口 */
    void OledThreadInit(void);
    void Show_PowerOn(void);
    void Show_PowerOff(void);
    void Show_PowerError(void);
    void Show_NODISC(void);
    void Show_LOADING(void);
    void Show_EJECTING(void);
    void Show_PLAY(void);
    void Show_PAUSE(void);
    void Show_STOP(void);
    void Show_MusicNum_And_Time(const char* connectchar);
    void FastPreviousing(void);
    void FatsNexting(void);

#ifdef __cplusplus
}
#endif

#endif /* _THREAD_OLEDSHOW_H */
