#ifndef  __CD_H
#define  __CD_H
#include "stm32f10x.h"                  // Device header
#include "cmsis_os.h"                   // ARM::CMSIS:RTOS:Keil RTX
#include "Power.h"
#include "OLED.h"

//�ⲿ��������
extern osPoolId PowerPool;
extern osMessageQId PowerMessages; 
extern osMailQId Mail;
extern osTimerId Timer;

/* CD�̴߳������� */
void CD_Thread (void const *argument);
/* CD�̳߳�ʼ������ */
void CD_ThreadInit(void);
/* ��ʱ���ص����� */
void Timer_Callback  (void const *arg);

/* ����ͻָ����� */
void SaveState(void);
void RestoreState(void);

/* CD��Power�ظ����ػ����� */
void CD_Respond_ON(void);
void CD_Respond_OFF(void);
/* CD��OLED����CD״̬���� */
void CD_Send_Load(void);
void CD_Send_Eject(void);
void CD_Send_Play(void);
void CD_Send_Pause(void);
void CD_Send_Stop(void);
void CD_Send_NoDisc(void);
void CD_Send_FastPreviousing(void);
void CD_Send_FastNexting(void);
void CD_Send_Previous(void);
void CD_Send_Next(void);

/* CD �յ�����Ϣ */
typedef  enum
    {
    CD_Power_ON = 0x10, //����
    CD_Power_OFF = 0x12, // �ػ�
    CD_Load_Eject = 0x01, //���غ͵���
    CD_Play_Pause = 0x02, //���ź���ͣ
    CD_Previous = 0x03, //��һ��
    CD_Next = 0x04, //��һ��
    CD_Wait_3s = 0x05, //�ȴ�3s
}CD_Msg;   //CD �յ���Ϣ��ö������


typedef struct
    {
    CD_Msg CD_Res;
}CD_Msg_Type;



typedef enum {
	NoDisc, //��CD
	Disc,  //��CD 
	Load,  //����
	Eject, //����
	Stop, //ֹͣ
	Play, //����
	Pause,//��ͣ
	Previous, //��һ��
	Next //��һ��
    
}CD_State;//CD��״̬��ö������

typedef struct {
    CD_State Current;//��ǰ״̬
    CD_Msg Msg;  //���յ�����Ϣ
    CD_State Next; //��һ״̬
    void (*fun)(); //ִ�к���
} State_Transition; //״̬Ǩ��


void CdStateChange(CD_Msg m);
int CheckTransition(CD_State s, CD_Msg m);
#endif


