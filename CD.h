#ifndef  __CD_H
#define  __CD_H
#include "stm32f10x.h"                  // Device header
#include "cmsis_os.h"                   // ARM::CMSIS:RTOS:Keil RTX
#include "Power.h"
#include "OLED.h"

// 外部变量声明
extern osPoolId PowerPool;
extern osMessageQId PowerMessages; 
extern osMailQId Mail;
extern osTimerId Timer;

/* CD线程函数声明 */
void CD_Thread (void const *argument);
/* CD线程初始化函数声明 */
void CD_ThreadInit(void);
/* 定时器回调函数声明 */
void Timer_Callback  (void const *arg);

/* 状态保存和恢复相关函数 */
void SaveState(void);
void RestoreState(void);

/* CD与Power模块响应相关函数 */
void CD_Respond_ON(void);
void CD_Respond_OFF(void);

/* CD与OLED模块状态和曲目信息发送相关函数 */
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

/* CD接收的消息类型 */
typedef  enum
{
    CD_Power_ON = 0x01,   // 上电
    CD_Power_OFF = 0x02,  // 断电
    CD_Load_Eject = 0x03, // 载入/弹出
    CD_Play_Pause = 0x04, // 播放/暂停
    CD_Previous = 0x05,   // 上一曲
    CD_Next = 0x06,       // 下一曲
    CD_Wait_3s            // 等待3秒
}CD_Msg;   //CD 接收消息枚举

typedef struct
{
    CD_Msg CD_Res;
}CD_Msg_Type;

typedef enum {
    NoDisc, // 无CD
    Disc,  // 有CD 
    Load,  // 载入
    Eject, // 弹出
    Stop, // 停止
    Play, // 播放
    Pause,// 暂停
    Previous, // 上一曲
    Next // 下一曲
}CD_State;//CD状态枚举

typedef struct {
    CD_State Current;//当前状态
    CD_Msg Msg;  //接收到的消息
    CD_State Next; //下一个状态
    void (*fun)(const char *name, int num, int ptime); //执行函数，新增参数
} State_Transition; //状态迁移

void CdStateChange(CD_Msg m);
int CheckTransition(CD_State s, CD_Msg m);

#endif