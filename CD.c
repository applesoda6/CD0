#include "stm32f10x.h"                  // Device header
#include "cmsis_os.h"                   // ARM::CMSIS:RTOS:Keil RTX
#include "CD.h" 
#include "Power.h"
#include "OLED.h"


    
    CD_State StateInit = NoDisc;                 //CD��ʼ��״̬
    int MusicNumber = 0;                        //��ʼ����Ŀ
    CD_State LastState;                        //CD�ػ�ǰ��״̬
    int LastMusicNumber = 1;                  // ��ʼ���ػ�ǰ����ĿΪ1
    
    OledMessages *Oledp = NULL;                //��Oled������Ϣ��ָ��
    PowerMsgs *Powerp = NULL;                 // ��Power������Ϣ��ָ��

    osThreadId CD_ID;                       //�߳�ID����
    osPoolId CD_Pool;                      //�ڴ��ID����
    osMessageQId CD_Messages;             //��Ϣ����ID����
    osTimerId Timer;                     //��ʱ��ID����

    osThreadDef(CD_Thread,osPriorityNormal, 1, 0);         //�����߳̽ṹ�����
    osPoolDef(CD_Pool, 2,CD_Msg_Type );                 //�����ڴ�ؽṹ�����
    osMessageQDef(CD_MsgBox,1,&CD_Msg_Type);           //������Ϣ���нṹ�����
    osTimerDef(Timer,Timer_Callback);                 //���嶨ʱ��1�ṹ�����



    /* ����LastState�ĺ��� */
void SaveState(void) 
{
    LastState = StateInit;
    LastMusicNumber = MusicNumber;
    
}


    /* �ָ�LastState�ĺ��� */
void RestoreState(void) 
{
    StateInit = LastState;
    MusicNumber = LastMusicNumber;
    
}


    /* CD�߳���ں������� */
void CD_Thread (void const *argument)
{
        osEvent CD_Event; //���ڴ洢����Ϣ�����л�ȡ���¼���Ϣ
		CD_Msg_Type *Answer;//ָ����Ϣ�ṹ���ָ�룬���ڴ������Ϣ�����н��յ�����Ϣ
		CD_Msg CdRespond;//���ڴ洢��Ϣ�е� CD_Res���� CD ����Ӧ��Ϣ��

    while(1)
    {
			CD_Event = osMessageGet(CD_Messages,osWaitForever);//��ȡ�¼���Ϣ
			if(CD_Event.status == osEventMessage)  //�жϻ�ȡ���¼��Ƿ�Ϊһ����Ϣ�¼�
		{
			Answer = (CD_Msg_Type *)CD_Event.value.p;//����Ϣֵת��Ϊָ����Ϣ�ṹ�� CD_Msg_Type ��ָ�� Answer��
			CdRespond = Answer->CD_Res;//����Ϣ�ṹ������ȡ��Ӧ��Ϣ CD_Res ���洢�� CdRespond ��
			CdStateChange(CdRespond);//���� CdStateChange ����������Ӧ��Ϣ��
			osPoolFree(CD_Pool,Answer);//ʹ�� osPoolFree �����ͷ���Ϣ�ṹ����ռ�õ��ڴ��
		} 
        IWDG_ReloadCounter();
    }
}



    /* CD�̳߳�ʼ�� */
void CD_ThreadInit(void)
{
        CD_ID = osThreadCreate(osThread(CD_Thread),NULL); //����CD�̶߳���
        CD_Pool = osPoolCreate(osPool(CD_Pool));  //����CD�ڴ�ض���
        CD_Messages = osMessageCreate(osMessageQ(CD_MsgBox),NULL);//����CD��Ϣ���ж���
        Timer = osTimerCreate(osTimer(Timer),osTimerOnce,NULL);//����CDTimer1����3s��
        
        RestoreState();// ���ûָ��ϴα��溯��
}
 

    /* ��ʱ���ص����� */
void Timer_Callback  (void const *arg)
{
        CD_Msg_Type *pointer1;
        pointer1 = osPoolCAlloc(CD_Pool);//��CD_Pool�ڴ���з���
        pointer1 -> CD_Res = CD_Wait_3s;//������Ϣ���ݵȴ�3s
        osMessagePut(CD_Messages,(uint32_t)pointer1,osWaitForever);//����Ϣ����CD_Messages��Ϣ������
    
}

/* CD��Power���Ϳ������� */
void CD_Respond_ON(void) 
{
    Powerp = osPoolCAlloc(PowerPool);//��Power�ڴ���з���һ����Ϣ�ṹ��
    Powerp ->PowerAnswer = PowerAnswer1;//������Ϣ����ΪPowerAnswer1������
    osMessagePut(PowerMessages,(uint32_t)Powerp,osWaitForever);//����Ϣ������Ϣ������
    

}

/* CD��Power���͹ػ����� */
void CD_Respond_OFF(void) 
{
    Powerp =osPoolCAlloc(PowerPool);//��Power�ڴ���з���һ����Ϣ�ṹ��
    Powerp ->PowerAnswer = PowerAnswer2;//������Ϣ����ΪPowerAnswer2����ػ�
    osMessagePut(PowerMessages,(uint32_t)Powerp,osWaitForever);//����Ϣ����PowerMessage��Ϣ������

    SaveState();//���ñ��浱ǰ״̬����
}

/* CD��OLED����Load���� */
void CD_Send_Load(void) 
{
        Oledp = osMailCAlloc(Mail,osWaitForever);//��Mail�ڴ���з���һ����Ϣ�ṹ��
        Oledp->OledResCd=LOADING;//������Ϣ����ΪLOADING
        osMailPut(Mail,Oledp);//����Ϣ����Mail��Ϣ������
        osTimerStart(Timer,3000);//������ʱ��������ʱ��Ϊ3s

}

/* CD��OLED����Eject���� */
void CD_Send_Eject(void) 
{
        Oledp = osMailCAlloc(Mail,osWaitForever);//��Mail�ڴ���з���һ����Ϣ�ṹ��
        Oledp->OledResCd=EJECTING;//������Ϣ����ΪEJECTING
        osMailPut(Mail,Oledp);//����Ϣ����Mail��Ϣ������
        osTimerStart(Timer,3000);//������ʱ��������ʱ��Ϊ3s

}

/* CD��OLED����Play���� */
void CD_Send_Play(void) 
{
        Oledp = osMailCAlloc(Mail,osWaitForever);//��Mail�ڴ���з���һ����Ϣ�ṹ��
        Oledp->OledResCd=PLAY;//������Ϣ����ΪPLAY
        Oledp->Music_Num=MusicNumber;//����Music ��Ŀ
        osMailPut(Mail,Oledp);//����Ϣ����Mail��Ϣ������

}

/* CD��OLED����PAUSE���� */
void CD_Send_Pause(void) 
{
        Oledp = osMailCAlloc(Mail,osWaitForever);//��Mail�ڴ���з���һ����Ϣ�ṹ��
        Oledp->OledResCd=PAUSE;//������Ϣ����ΪPAUSE
        Oledp->Music_Num=MusicNumber;//����Music ��Ŀ
        osMailPut(Mail,Oledp);//����Ϣ����Mail��Ϣ������
    
}

/* CD��OLED����STOP���� */
void CD_Send_Stop(void)
{
   Oledp = osMailCAlloc(Mail, osWaitForever);//��Mail�ڴ���з���һ����Ϣ�ṹ��
   Oledp->OledResCd = STOP;//������Ϣ����ΪSTOP
   osMailPut(Mail, Oledp);//����Ϣ����Mail��Ϣ������
    
}

/* CD��OLED����NO_DISC���� */
void CD_Send_NoDisc(void)
{
	Oledp = osMailCAlloc(Mail, osWaitForever);//��Mail�ڴ���з���һ����Ϣ�ṹ��
   Oledp->OledResCd = NO_DISC;//������Ϣ����ΪNO_DISC
   osMailPut(Mail, Oledp);//����Ϣ����Mail��Ϣ������
    
}

/* CD��OLED����������һ������ */
void CD_Send_FastPreviousing(void)
{
    int Num = 100; //����ģ�⹲100�׸�
    int i;
    for (i = 0;i < Num;i++)
    {
        MusicNumber--;
			  osDelay(500);  //�ӳ�0.5s
        if (MusicNumber < 1)
        {
            MusicNumber = 100;
        }
    
        Oledp = osMailCAlloc(Mail, osWaitForever);//��Mail�ڴ���з���һ����Ϣ�ṹ��
        Oledp->OledResCd = PREVIOUS;//������Ϣ����ΪPREVIOUS
        Oledp->Music_Num = MusicNumber;//����Music��Ŀ
        osMailPut(Mail, Oledp);//����Ϣ����Mail��Ϣ������
				if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_1) == 1) //������Ϊ�ߵ�ƽʱ
				break;				  		
    }
		Oledp->OledResCd = PAUSE;
}

/* CD��OLED����������һ����Ϣ */
void CD_Send_FastNexting(void)
{
    int Num = 100;//����ģ�⹲100�׸�
    int i;
    for (i = 0;i < Num;i++)
    {	
        MusicNumber++;
			  osDelay(500);//�ӳ�0.5s
        if (MusicNumber > 100)
        {
            MusicNumber = 1;
        }
    
        Oledp = osMailCAlloc(Mail, osWaitForever);//��Mail�ڴ���з���һ����Ϣ�ṹ��
        Oledp->OledResCd = NEXT;//������Ϣ����ΪNEXT
        Oledp->Music_Num = MusicNumber;//����Music��Ŀ
        osMailPut(Mail, Oledp);	//����Ϣ����Mail��Ϣ������
        if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_13) == 1)//������Ϊ�ߵ�ƽʱ
				break;				
    }
		Oledp->OledResCd = PAUSE; //������Ϣ����ΪPAUSE

}

/* CD��OLED������һ������ */
void CD_Send_Previous(void)
{
        MusicNumber--;
        if(MusicNumber < 1)
        {
                MusicNumber=100;
        }
        Oledp = osMailCAlloc(Mail,osWaitForever);//��Mail�ڴ���з���һ����Ϣ�ṹ��
        Oledp->OledResCd=PLAY;//������Ϣ����ΪPLAY
        Oledp->Music_Num=MusicNumber;//����Music��Ŀ
        osMailPut(Mail,Oledp);//����Ϣ����Mail��Ϣ������
}

/* CD��OLED������һ����Ϣ */
void CD_Send_Next(void)
{
        MusicNumber++;
        if(MusicNumber >100)
        {
                MusicNumber=001;
        }
        Oledp = osMailCAlloc(Mail,osWaitForever);//��Mail�ڴ���з���һ����Ϣ�ṹ��
        Oledp->OledResCd=PLAY;//������Ϣ����ΪPLAY
        Oledp->Music_Num=MusicNumber;//����Music��Ŀ
        osMailPut(Mail,Oledp);//����Ϣ����Mail��Ϣ������
}

/* ״̬Ǩ�Ʊ� */
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
 {  Pause,            CD_Previous,        Previous,   CD_Send_FastPreviousing},
 {  Pause,            CD_Next,            Next,       CD_Send_FastNexting    },

 {  Previous,         CD_Power_OFF,       Disc,              CD_Respond_OFF  },
 {  Previous,         CD_Load_Eject,      Eject,             CD_Send_Eject   },
 {  Previous,         CD_Play_Pause ,     Play,              CD_Send_Play    },
 {  Previous,         CD_Previous ,       Previous,   CD_Send_FastPreviousing},
 {  Previous,         CD_Next ,           Next,       CD_Send_FastNexting    },

 {  Next,             CD_Power_OFF,       Disc,               CD_Respond_OFF },
 {  Next,             CD_Load_Eject,      Eject,              CD_Send_Eject  },
 {  Next,             CD_Play_Pause ,     Play,               CD_Send_Play   },
 {  Next,             CD_Previous ,       Previous,   CD_Send_FastPreviousing},
 {  Next,             CD_Next ,           Next,       CD_Send_FastNexting    },

 };

 /* ��鵱ǰ״̬ �� ���յ�KEY����Ϣ */
int CheckTransition(CD_State s, CD_Msg m)  
{
    int ret = -1; // ����ֵ��ʼ��Ϊ-1 ��ʾδ�ҵ�
    int flag = -1; // ��־������ʼ��Ϊ-1 ��ʾδ�ҵ�
    int i;

    for (i = 0; i < sizeof(Arr) / sizeof(Arr[0]); i++)
    {
        //�����е�״̬�봫���״̬�Ƿ���� ���� //�����е���Ϣ�봫�����Ϣ�Ƿ����
        if (Arr[i][0].Current == s && Arr[i][0].Msg == m) 
        {
            flag = i;
            ret = flag;
            break;
        }
    }
    return ret;
}

void CdStateChange(CD_Msg m)
{
    int index = CheckTransition(StateInit, m);// ��鵱ǰ״̬����Ϣ��ת�����

    if (index != -1)
    {
         // ִ��״̬ת�ƺ���
        Arr[index][0].fun();
        // ���µ�ǰ״̬Ϊ��һ��״̬ 
        StateInit = Arr[index][0].Next;
    }
    else
    {
        index = 0;
    }
}

