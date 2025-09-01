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
osTimerId PlayInfoTimer; // ����������Ϣ��ʱ��

static int play_seconds = 0; // ����ʱ�����룩
static char current_name[32] = "DefaultSong"; // ��ǰ����
static int current_num = 1; // ��ǰ����

osThreadDef(CD_Thread,osPriorityNormal, 1, 0);
osPoolDef(CD_Pool, 2,CD_Msg_Type );
osMessageQDef(CD_MsgBox,1,&CD_Msg_Type);
osTimerDef(Timer,Timer_Callback);
osTimerDef(PlayInfoTimer,PlayInfoTimer_Callback);

/* ��������ʱ��ת�ַ�����hh:mm:ss����ʽ */
void FormatPlayTime(int second, char *str, int len)
{
    int h = second / 3600;
    int m = (second % 3600) / 60;
    int s = second % 60;
    snprintf(str, len, "%02d:%02d:%02d", h, m, s);
}

/* ������һ��״̬ */
void SaveState(void) 
{
    LastState = StateInit;
    LastMusicNumber = MusicNumber;
}

/* �ָ���һ��״̬ */
void RestoreState(void) 
{
    StateInit = LastState;
    MusicNumber = LastMusicNumber;
}

/* CD�߳������� */
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
            // ʵ����Ŀ�����Ŀ�����ȡ��������Ϣ��������ȫ�ֱ���
            int num = MusicNumber;
            char name[32];
            strncpy(name, current_name, sizeof(name)-1);
            name[sizeof(name)-1] = '\0';
            char ptime[12];
            FormatPlayTime(play_seconds, ptime, sizeof(ptime));

            // ״̬Ǩ�Ʋ�������Ŀ��Ϣ
            CdStateChangeWithInfo(CdRespond, name, num, ptime);

            osPoolFree(CD_Pool,Answer);
        } 
        IWDG_ReloadCounter();
    }
}

/* CD�̳߳�ʼ�� */
void CD_ThreadInit(void)
{
    CD_ID = osThreadCreate(osThread(CD_Thread),NULL);
    CD_Pool = osPoolCreate(osPool(CD_Pool));
    CD_Messages = osMessageCreate(osMessageQ(CD_MsgBox),NULL);
    Timer = osTimerCreate(osTimer(Timer),osTimerOnce,NULL);
    PlayInfoTimer = osTimerCreate(osTimer(PlayInfoTimer), osTimerPeriodic, NULL);
    RestoreState();
}

/* ��ʱ���ص���������Ŀ��Ϣ���������롢�����ȴ��� */
void Timer_Callback  (void const *arg)
{
    CD_Msg_Type *pointer1;
    pointer1 = osPoolCAlloc(CD_Pool);
    pointer1 -> CD_Res = CD_Wait_3s;
    osMessagePut(CD_Messages,(uint32_t)pointer1,osWaitForever);
}

/* ÿ����OLED����һ�β�����Ϣ�����ڲ���״̬�� */
void PlayInfoTimer_Callback(void const *arg)
{
    play_seconds++;
    char ptime[12];
    FormatPlayTime(play_seconds, ptime, sizeof(ptime));
    CD_Send_Play(current_name, current_num, ptime);
}

/* CD��Powerģ�������Ӧ������Ҫ����Ŀ��Ϣ */
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

/* OLED��Ϣ���ͺ�������װMsgMail_t�ṹ�岢���� */
static void Send_OLED_Mail(uint8_t eventID, uint32_t scourceID, uint32_t targetID, uint32_t option[3])
{
    MsgMail_t msg;
    msg.eventID = eventID;
    msg.scourceID = scourceID;
    msg.targetID = targetID;
    msg.option[0] = option[0];
    msg.option[1] = option[1];
    msg.option[2] = option[2];
    // MailΪosMailQId���ͣ�ʵ�ʷ�����OLEDģ��ʵ��
    osMailPut(Mail, &msg);
}

/* �ַ���תuint32_t������option����洢����/ʱ���ȣ� */
static uint32_t StrToU32(const char *str)
{
    // ��hash/���룬�ɻ�Ϊ�����ʵķ�ʽ
    uint32_t val = 0;
    for (int i = 0; str[i] && i < 4; ++i) {
        val = (val << 8) | (uint8_t)str[i];
    }
    return val;
}

/* CD��OLEDģ�鷢�ʹ���Ŀ��Ϣ��״̬����װ��MsgMail_t�ṹ�壩 */
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
    // ����������Ϣ��ʱ����ÿ��һ��
    if (StateInit != Play) {
        play_seconds = 0; // �и�/�ز�ʱ����
    }
    strncpy(current_name, name, sizeof(current_name)-1);
    current_num = num;
    osTimerStart(PlayInfoTimer, 1000);
}

void CD_Send_Pause(const char *name, int num, const char *ptime)
{
    uint32_t option[3] = { StrToU32(name), num, StrToU32(ptime) };
    Send_OLED_Mail(0x04, CD, Pause, option);
    osTimerStop(PlayInfoTimer); // ��ͣʱֹͣ��ʱ��
}

void CD_Send_Stop(const char *name, int num, const char *ptime)
{
    uint32_t option[3] = { StrToU32(name), num, StrToU32(ptime) };
    Send_OLED_Mail(0x05, CD, Stop, option);
    osTimerStop(PlayInfoTimer); // ֹͣʱֹͣ��ʱ��
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

/* ״̬Ǩ�Ʊ�����Ŀ��Ϣ�ĺ���ָ�� */
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

/* ���ҵ�ǰ״̬����Ϣ��Ӧ��Ǩ�Ʊ��±� */
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

/* ����������Ŀ��Ϣ��״̬Ǩ�ƺ��� */
void CdStateChangeWithInfo(CD_Msg m, const char *name, int num, const char *ptime)
{
    int index = CheckTransition(StateInit, m);
    if (index != -1)
    {
        // ִ��״̬Ǩ�ƶ�����������Ŀ��Ϣ
        Arr[index][0].fun(name, num, ptime);
        StateInit = Arr[index][0].Next;
    }
    else
    {
        index = 0;
    }
}

/* ����ԭ�нӿڣ�������Ŀ��Ϣ�򴫵�Ĭ��ֵ */
void CdStateChange(CD_Msg m)
{
    char ptime[12];
    FormatPlayTime(play_seconds, ptime, sizeof(ptime));
    CdStateChangeWithInfo(m, current_name, MusicNumber, ptime);
}

/*
ע��˵����
- ���з��͵�OLED����Ϣ��ͨ��MsgMail_t�ṹ���װ������/���/ʱ��������option���飬ʱ��Ϊ"hh:mm:ss"�ַ������롣
- CD_Send_Play�ڽ��벥��״̬ʱ����1�붨ʱ����ÿ���Զ�����PlayInfoTimer_Callback����һ�β�����Ϣ������60��/�����Զ���λ��OLED���յ���option[2]��Ϊ��ʽ��ʱ�䡣
- ��ͣ/ֹͣ/����ʱֹͣ��ʱ����
*/