#ifndef  __CD_H
#define  __CD_H
#include "stm32f10x.h"                  // Device header
#include "cmsis_os.h"                   // ARM::CMSIS:RTOS:Keil RTX
#include "Power.h"
#include "OLED.h"

// �ⲿ��������
extern osPoolId PowerPool;
extern osMessageQId PowerMessages; 
extern osMailQId Mail;
extern osTimerId Timer;

/* CD�̺߳������� */
void CD_Thread (void const *argument);
/* CD�̳߳�ʼ���������� */
void CD_ThreadInit(void);
/* ��ʱ���ص��������� */
void Timer_Callback  (void const *arg);

/* ״̬����ͻָ���غ��� */
void SaveState(void);
void RestoreState(void);

/* CD��Powerģ����Ӧ��غ��� */
void CD_Respond_ON(void);
void CD_Respond_OFF(void);

/* CD��OLEDģ��״̬����Ŀ��Ϣ������غ��� */
void CD_Send_Load(const char *name, int num, int ptime);
void CD_Send_Eject(const char *name, int num, int ptime);
void CD_Send_Play(const char *name, int num, int ptime);
void CD_Send_Pause(const char *name, int num, int ptime);
void CD_Send_Stop(const char *name, int num, int ptime);
void CD_Send_NoDisc(void);
void CD_Send_FastPreviousing(const char *name, int num, int ptime);
void CD_Send_FastNexting(const char *name, int num, int ptime);
void CD_Send_Previous(const char *name, int num, int ptime);
void CD_Send_Next(const char *name, int num, int ptime);

/* CD���յ���Ϣ���� */
typedef  enum
{
    CD_Power_ON = 0x01,   // �ϵ�
    CD_Power_OFF = 0x02,  // �ϵ�
    CD_Load_Eject = 0x03, // ����/����
    CD_Play_Pause = 0x04, // ����/��ͣ
    CD_Previous = 0x05,   // ��һ��
    CD_Next = 0x06,       // ��һ��
    CD_Wait_3s            // �ȴ�3��
}CD_Msg;   //CD ������Ϣö��

typedef struct
{
    CD_Msg CD_Res;
}CD_Msg_Type;

typedef enum {
    NoDisc, // ��CD
    Disc,  // ��CD 
    Load,  // ����
    Eject, // ����
    Stop, // ֹͣ
    Play, // ����
    Pause,// ��ͣ
    Previous, // ��һ��
    Next // ��һ��
}CD_State;//CD״̬ö��

typedef struct {
    CD_State Current;//��ǰ״̬
    CD_Msg Msg;  //���յ�����Ϣ
    CD_State Next; //��һ��״̬
    void (*fun)(const char *name, int num, int ptime); //ִ�к�������������
} State_Transition; //״̬Ǩ��

void CdStateChange(CD_Msg m);
int CheckTransition(CD_State s, CD_Msg m);

#endif