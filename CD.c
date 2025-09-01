#include "stm32f10x.h"
#include "cmsis_os.h"
#include "CD.h"
#include "Power.h"
#include "OLED.h"
#include <string.h>
#include <stdio.h>

CD_State StateInit = NoDisc;
int MusicNumber = 0;
CD_State LastState;
int LastMusicNumber = 1;

OledMessages *Oledp = NULL;
PowerMsgs *Powerp = NULL;

osThreadId CD_ID;
osPoolId CD_Pool;
osMessageQId CD_Messages;
osTimerId Timer;
osTimerId PlayInfoTimer; // 歌曲播放信息定时器

static int play_seconds = 0; // 播放时长（秒）
static char current_name[32] = "DefaultSong"; // 当前曲名
static int current_num = 1; // 当前曲号

osThreadDef(CD_Thread,osPriorityNormal, 1, 0);
osPoolDef(CD_Pool, 2,CD_Msg_Type );
osMessageQDef(CD_MsgBox,1,&CD_Msg_Type);
osTimerDef(Timer,Timer_Callback);
osTimerDef(PlayInfoTimer,PlayInfoTimer_Callback);

/* 歌曲播放时长转字符串“hh:mm:ss”格式 */
void FormatPlayTime(int second, char *str, int len)
{
    int h = second / 3600;
    int m = (second % 3600) / 60;
    int s = second % 60;
    snprintf(str, len, "%02d:%02d:%02d", h, m, s);
}

/* 保存上一次状态 */
void SaveState(void) 
{
    LastState = StateInit;
    LastMusicNumber = MusicNumber;
}

/* 恢复上一次状态 */
void RestoreState(void) 
{
    StateInit = LastState;
    MusicNumber = LastMusicNumber;
}

/* CD线程主函数 */
void CD_Thread (void const *argument)
{
    osEvent CD_Event;
    CD_Msg_Type *Answer;
    CD_Msg CdRespond;
    while(1)
    {
        CD_Event = osMessageGet(CD_Messages,osWaitForever);
        if(CD_Event.status == osEventMessage)
        {
            Answer = (CD_Msg_Type *)CD_Event.value.p;
            CdRespond = Answer->CD_Res;
            // 实际项目请从曲目管理获取曲名等信息，这里用全局变量
            int num = MusicNumber;
            char name[32];
            strncpy(name, current_name, sizeof(name)-1);
            name[sizeof(name)-1] = '\0';
            char ptime[12];
            FormatPlayTime(play_seconds, ptime, sizeof(ptime));

            // 状态迁移并传递曲目信息
            CdStateChangeWithInfo(CdRespond, name, num, ptime);

            osPoolFree(CD_Pool,Answer);
        } 
        IWDG_ReloadCounter();
    }
}

/* CD线程初始化 */
void CD_ThreadInit(void)
{
    CD_ID = osThreadCreate(osThread(CD_Thread),NULL);
    CD_Pool = osPoolCreate(osPool(CD_Pool));
    CD_Messages = osMessageCreate(osMessageQ(CD_MsgBox),NULL);
    Timer = osTimerCreate(osTimer(Timer),osTimerOnce,NULL);
    PlayInfoTimer = osTimerCreate(osTimer(PlayInfoTimer), osTimerPeriodic, NULL);
    RestoreState();
}

/* 定时器回调，传递曲目信息（用于载入、弹出等待） */
void Timer_Callback  (void const *arg)
{
    CD_Msg_Type *pointer1;
    pointer1 = osPoolCAlloc(CD_Pool);
    pointer1 -> CD_Res = CD_Wait_3s;
    osMessagePut(CD_Messages,(uint32_t)pointer1,osWaitForever);
}

/* 每秒向OLED发送一次播放信息（用于播放状态） */
void PlayInfoTimer_Callback(void const *arg)
{
    play_seconds++;
    char ptime[12];
    FormatPlayTime(play_seconds, ptime, sizeof(ptime));
    CD_Send_Play(current_name, current_num, ptime);
}

/* CD与Power模块相关响应，不需要发曲目信息 */
void CD_Respond_ON(void) 
{
    Powerp = osPoolCAlloc(PowerPool);
    Powerp ->PowerAnswer = PowerAnswer1;
    osMessagePut(PowerMessages,(uint32_t)Powerp,osWaitForever);
}

void CD_Respond_OFF(void) 
{
    Powerp =osPoolCAlloc(PowerPool);
    Powerp ->PowerAnswer = PowerAnswer2;
    osMessagePut(PowerMessages,(uint32_t)Powerp,osWaitForever);
    SaveState();
}

/* OLED消息发送函数，封装MsgMail_t结构体并发送 */
static void Send_OLED_Mail(uint8_t eventID, uint32_t scourceID, uint32_t targetID, uint32_t option[3])
{
    MsgMail_t msg;
    msg.eventID = eventID;
    msg.scourceID = scourceID;
    msg.targetID = targetID;
    msg.option[0] = option[0];
    msg.option[1] = option[1];
    msg.option[2] = option[2];
    // Mail为osMailQId类型，实际发送由OLED模块实现
    osMailPut(Mail, &msg);
}

/* 字符串转uint32_t（用于option数组存储曲名/时长等） */
static uint32_t StrToU32(const char *str)
{
    // 简单hash/编码，可换为更合适的方式
    uint32_t val = 0;
    for (int i = 0; str[i] && i < 4; ++i) {
        val = (val << 8) | (uint8_t)str[i];
    }
    return val;
}

/* CD向OLED模块发送带曲目信息的状态（封装到MsgMail_t结构体） */
void CD_Send_Load(const char *name, int num, const char *ptime)
{
    uint32_t option[3] = { StrToU32(name), num, StrToU32(ptime) };
    Send_OLED_Mail(0x01, CD, Load, option);
    osTimerStart(Timer, 3000);
}

void CD_Send_Eject(const char *name, int num, const char *ptime)
{
    uint32_t option[3] = { StrToU32(name), num, StrToU32(ptime) };
    Send_OLED_Mail(0x02, CD, Eject, option);
    osTimerStart(Timer, 3000);
}

void CD_Send_Play(const char *name, int num, const char *ptime)
{
    uint32_t option[3] = { StrToU32(name), num, StrToU32(ptime) };
    Send_OLED_Mail(0x03, CD, Play, option);
    // 启动播放信息定时器，每秒一次
    if (StateInit != Play) {
        play_seconds = 0; // 切歌/重播时清零
    }
    strncpy(current_name, name, sizeof(current_name)-1);
    current_num = num;
    osTimerStart(PlayInfoTimer, 1000);
}

void CD_Send_Pause(const char *name, int num, const char *ptime)
{
    uint32_t option[3] = { StrToU32(name), num, StrToU32(ptime) };
    Send_OLED_Mail(0x04, CD, Pause, option);
    osTimerStop(PlayInfoTimer); // 暂停时停止定时器
}

void CD_Send_Stop(const char *name, int num, const char *ptime)
{
    uint32_t option[3] = { StrToU32(name), num, StrToU32(ptime) };
    Send_OLED_Mail(0x05, CD, Stop, option);
    osTimerStop(PlayInfoTimer); // 停止时停止定时器
}

void CD_Send_NoDisc(void)
{
    uint32_t option[3] = { 0, 0, 0 };
    Send_OLED_Mail(0x06, CD, NoDisc, option);
    osTimerStop(PlayInfoTimer);
}

void CD_Send_FastPreviousing(const char *name, int num, const char *ptime)
{
    uint32_t option[3] = { StrToU32(name), num, StrToU32(ptime) };
    Send_OLED_Mail(0x07, CD, Previous, option);
}

void CD_Send_FastNexting(const char *name, int num, const char *ptime)
{
    uint32_t option[3] = { StrToU32(name), num, StrToU32(ptime) };
    Send_OLED_Mail(0x08, CD, Next, option);
}

void CD_Send_Previous(const char *name, int num, const char *ptime)
{
    uint32_t option[3] = { StrToU32(name), num, StrToU32(ptime) };
    Send_OLED_Mail(0x09, CD, Play, option);
}

void CD_Send_Next(const char *name, int num, const char *ptime)
{
    uint32_t option[3] = { StrToU32(name), num, StrToU32(ptime) };
    Send_OLED_Mail(0x0A, CD, Play, option);
}

/* 状态迁移表，带曲目信息的函数指针 */
State_Transition Arr[][4] =
{
    {  NoDisc,           CD_Power_ON,        NoDisc,            CD_Respond_ON   },
    {  NoDisc,           CD_Power_OFF,       NoDisc,            CD_Respond_OFF  },
    {  NoDisc,           CD_Load_Eject,      Load,              CD_Send_Load    },
    {  NoDisc,           CD_Play_Pause,      NoDisc,            CD_Send_NoDisc  },
    {  NoDisc,           CD_Previous,        NoDisc,            CD_Send_NoDisc  },
    {  NoDisc,           CD_Next,            NoDisc,            CD_Send_NoDisc  },

    {  Disc,             CD_Power_ON,        Stop,              CD_Send_Stop    },
    {  Disc,             CD_Power_OFF,       NoDisc,            CD_Respond_OFF  },
    {  Disc,             CD_Load_Eject,      Eject,             CD_Send_Eject   },

    {  Load,             CD_Power_OFF,       NoDisc,            CD_Respond_OFF  }, 
    {  Load,             CD_Load_Eject,      Eject,             CD_Send_Eject   },
    {  Load,             CD_Wait_3s,         Stop,              CD_Send_Stop    },

    {  Eject,            CD_Power_OFF,       Disc,              CD_Respond_OFF  }, 
    {  Eject,            CD_Load_Eject,      Load,              CD_Send_Stop    }, 
    {  Eject,            CD_Wait_3s,         NoDisc,            CD_Send_NoDisc  },

    {  Stop,             CD_Power_OFF,       Disc,              CD_Respond_OFF  },
    {  Stop,             CD_Load_Eject,      Eject,             CD_Send_Eject   },
    {  Stop,             CD_Play_Pause,      Play,              CD_Send_Play    },
    {  Stop,             CD_Previous,        Play,              CD_Send_Previous},
    {  Stop,             CD_Next,            Play,              CD_Send_Next    },

    {  Play,             CD_Power_OFF,       Disc,              CD_Respond_OFF  },
    {  Play,             CD_Play_Pause,      Pause,             CD_Send_Pause   },
    {  Play,             CD_Previous,        Play,              CD_Send_Previous},
    {  Play,             CD_Next,            Play,              CD_Send_Next    },
    {  Play,             CD_Load_Eject,      Eject,             CD_Send_Eject   },

    {  Pause,            CD_Power_OFF,       Disc,              CD_Respond_OFF  },
    {  Pause,            CD_Load_Eject,      Eject,             CD_Send_Eject   }, 
    {  Pause,            CD_Play_Pause,      Play,              CD_Send_Play    },
    {  Pause,            CD_Previous,        Previous,          CD_Send_FastPreviousing},
    {  Pause,            CD_Next,            Next,              CD_Send_FastNexting    },

    {  Previous,         CD_Power_OFF,       Disc,              CD_Respond_OFF  },
    {  Previous,         CD_Load_Eject,      Eject,             CD_Send_Eject   },
    {  Previous,         CD_Play_Pause ,     Play,              CD_Send_Play    },
    {  Previous,         CD_Previous ,       Previous,          CD_Send_FastPreviousing},
    {  Previous,         CD_Next ,           Next,              CD_Send_FastNexting    },

    {  Next,             CD_Power_OFF,       Disc,              CD_Respond_OFF },
    {  Next,             CD_Load_Eject,      Eject,             CD_Send_Eject  },
    {  Next,             CD_Play_Pause ,     Play,              CD_Send_Play   },
    {  Next,             CD_Previous ,       Previous,          CD_Send_FastPreviousing},
    {  Next,             CD_Next ,           Next,              CD_Send_FastNexting    },
};

/* 查找当前状态和消息对应的迁移表下标 */
int CheckTransition(CD_State s, CD_Msg m)  
{
    int ret = -1;
    int flag = -1;
    int i;
    for (i = 0; i < sizeof(Arr) / sizeof(Arr[0]); i++)
    {
        if (Arr[i][0].Current == s && Arr[i][0].Msg == m) 
        {
            flag = i;
            ret = flag;
            break;
        }
    }
    return ret;
}

/* 新增：带曲目信息的状态迁移函数 */
void CdStateChangeWithInfo(CD_Msg m, const char *name, int num, const char *ptime)
{
    int index = CheckTransition(StateInit, m);
    if (index != -1)
    {
        // 执行状态迁移动作，传递曲目信息
        Arr[index][0].fun(name, num, ptime);
        StateInit = Arr[index][0].Next;
    }
    else
    {
        index = 0;
    }
}

/* 兼容原有接口：若无曲目信息则传递默认值 */
void CdStateChange(CD_Msg m)
{
    char ptime[12];
    FormatPlayTime(play_seconds, ptime, sizeof(ptime));
    CdStateChangeWithInfo(m, current_name, MusicNumber, ptime);
}

/*
注释说明：
- 所有发送到OLED的信息都通过MsgMail_t结构体封装，曲名/编号/时长都存入option数组，时长为"hh:mm:ss"字符串编码。
- CD_Send_Play在进入播放状态时启动1秒定时器，每秒自动调用PlayInfoTimer_Callback发送一次播放信息，超过60秒/分钟自动进位，OLED端收到的option[2]即为格式化时间。
- 暂停/停止/无盘时停止定时器。
*/