#ifndef _THREAD_POWER_H
#define _THREAD_POWER_H

#include "cmsis_os.h"
#include "CD.h"
#include "OLED.h"

/* POWER �̵߳��������ɰ��裩 */
#define PowerThread  Thread_POWER

/* ===================== POWER �����¼���˭����˭���壩 ===================== */
typedef enum {
    KEY_RECEIVE_ON = 0,         /* ���� KEY���������� */
    KEY_RECEIVE_OFF,            /* ���� KEY���ػ����� */
    CD_ON_RECEIVE_OK,           /* ���� CD��������� */
    CD_OFF_RECEIVE_OK,          /* ���� CD���ػ���� */
    OLED_ON_RECEIVE_OK,         /* ���� OLED��������� */
    OLED_OFF_RECEIVE_OK,        /* ���� OLED���ػ���� */
    ALL_RECEIVE_OK,             /* �ڲ�������ģ����� */
    TIMER_OUT,                  /* �ڲ�����ʱ */
    ERROR_OUT_TRYCOUNT_THREE,   /* �ڲ������Դ����� */
    POWER_RECV_MSG_END
} POWER_RECV_MSG;

/* ���⶯����ǩ�������ã����������ý��շ��¼��ţ� */
typedef enum {
    SEND_OLED_POWERON = 0,
    SEND_OLED_POWEROFF,
    SEND_OLED_POWERERROR,
    SEND_CD_POWERON,
    SEND_CD_POWEROFF,
    SEND_CD_POWERERROR,
    MSG_POWER_SEND_END
} POWER_SEND_MSG;

/* POWER ���̽׶�/ģʽ���ڲ�״̬���� */
typedef enum {
    POWER_PROCESS_STAGE_0 = 0,  /* �����ػ��� */
    POWER_PROCESS_STAGE_1,      /* �ս׶� */
    POWER_PROCESS_STAGE_2,      /* ���������� */
    POWER_PROCESS_STAGE_3,      /* ģ��״̬���� */
    POWER_PROCESS_STAGE_4,      /* ������� */
    POWER_PROCESS_STAGE_5,      /* �ػ���� */
    POWER_PROCESS_STAGE_6,      /* ��ʱ���� */
    POWER_PROCESS_STAGE_7,      /* �������� */
    POWER_PROCESS_STAGE_END
} POWER_PROCESS_STAGE;

typedef enum {
    POWER_MODE_ON = 0,
    POWER_MODE_OFF,
    POWER_MODE_ONING,
    POWER_MODE_OFFING,
    POWER_MODE_END
} POWER_MODE;

/* ===================== ������Դ�뺯�� ===================== */
#ifdef __cplusplus
extern "C" {
#endif

    extern osMailQId g_mailq_power;                    /* POWER �������� */
    extern POWER_PROCESS_STAGE g_CurPowerProcessStage; /* ��ǰ�׶� */
    extern POWER_MODE          g_CurPowerMode;         /* ��ǰģʽ */

    /* �߳�/��ʼ�� */
    void Thread_POWER(void const* arg);
    void PowerThreadInit(void);

    /* ���ݾɽӿڣ�OLED ACK/�ڲ������ã����� POWER Ͷ�� */
    int  SendToPower(uint8_t sender, uint8_t receiver, POWER_RECV_MSG event, const char* option[3]);

#ifdef __cplusplus
}
#endif

#endif /* _THREAD_POWER_H */
