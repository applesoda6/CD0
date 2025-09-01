#ifndef _THREAD_OLEDSHOW_H
#define _THREAD_OLEDSHOW_H

#include "cmsis_os.h"
#include "OLED_driver.h"
#include "CD.h"    /* Ϊ��ʹ�� MsgMail_t / MOD_* */

/* OLED ���յ��¼���˭����˭���壩 */
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

    OLED_TURN_ON = 0x08,        /* ��Դģ��� OLED������ */
    OLED_TURN_OFF = 0x09,       /* ��Դģ��� OLED������ */
    OLED_TURNER_ERROR           /* ��Դģ��� OLED����Դ���� */
} ToOledMsg;

/* OLED �������䣨�� OLED.c ������ */
#ifdef __cplusplus
extern "C" {
#endif

    extern osMailQId g_mailq_oled;

    /* OLED �߳�����Ϊ�ӿ� */
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
