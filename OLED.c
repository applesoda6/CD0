#include "OLED.h"
#include "POWER.h"
#include "CD.h"
#include "stdio.h"
#include <string.h>

/* ---------------- RTOS ��Դ��OLED �������� + �߳� ---------------- */
osMailQDef(OLED_MAILQ, 16, MsgMail_t);
osMailQId g_mailq_oled = NULL;

static osThreadId OledId;

/* ---------------- ��ʾ���� ---------------- */
static void make_connect_str(const MsgMail_t* m, char buf[17])
{
    uint32_t trk = m->option[0];
    uint32_t mm = m->option[1];
    uint32_t ss = m->option[2];
    snprintf(buf, 17, "TRK:%02lu %02lu:%02lu",
        (unsigned long)trk, (unsigned long)mm, (unsigned long)ss);
}

void Show_PowerOn(void)                                  // ��ʾ��Դ������Ϣ�ĺ���
{
	OLED_ShowRow1((uint8_t *)"Power On            ");
	OLED_ShowRow2((uint8_t *)"NO DISC             ");
	OLED_ShowRow3((uint8_t *)"                    ");
}




void Show_PowerOff(void)                                // ��ʾ��Դ�ر���Ϣ�ĺ���
{
	OLED_ShowRow1((uint8_t *)"  Power Off        ");
	OLED_ShowRow2((uint8_t *)"                   ");
	OLED_ShowRow3((uint8_t *)"                   ");
}



void Show_PowerError(void)
{
	OLED_ShowRow1((uint8_t *)"  Power Error      ");
	OLED_ShowRow2((uint8_t *)"                   ");
	OLED_ShowRow3((uint8_t *)"                   ");
}

void Show_NODISC(void)                                 // ��ʾû��CD��Ϣ�ĺ���
{
  OLED_ShowRow1((uint8_t *)"Power On            ");
	OLED_ShowRow2((uint8_t *)"NO DISC             ");
	OLED_ShowRow3((uint8_t *)"                    ");
}



void Show_LOADING(void)                                 // ��ʾ������Ϣ�ĺ���
{
	OLED_ShowRow1((uint8_t *)"PowerOn CDSource    ");
	OLED_ShowRow2((uint8_t *)"LOADING             ");
	OLED_ShowRow3((uint8_t *)"                    ");
}



void Show_EJECTING(void)                                // ��ʾ������Ϣ�ĺ���
{
	OLED_ShowRow1((uint8_t *)"Power On            ");
	OLED_ShowRow2((uint8_t *)"EJECTING            ");
	OLED_ShowRow3((uint8_t *)"                    ");
}



void Show_PLAY(void)                                    // ��ʾ������Ϣ�ĺ���
{
	OLED_ShowRow1((uint8_t *)"PowerOn CDSource      ");
	OLED_ShowRow2((uint8_t *)"PLAY                  ");
	OLED_ShowRow3((uint8_t *)"                      ");
}



void Show_PAUSE(void)                                   // ��ʾ��ͣ��Ϣ�ĺ���
{
	OLED_ShowRow1((uint8_t *)"PowerOn CDSource     ");
	OLED_ShowRow2((uint8_t *)"PAUSE                ");
	OLED_ShowRow3((uint8_t *)"                     ");
}



void Show_STOP(void)                                    // ��ʾֹͣ������Ϣ�ĺ���
{
	OLED_ShowRow1((uint8_t *)"PowerOn CDSource    ");
	OLED_ShowRow2((uint8_t *)"STOP                ");
	OLED_ShowRow3((uint8_t *)"                    ");
}



void Show_MusicNum_And_Time(const char *connectchar)                          // ��ʾ����˳����Ϣ�ĺ���
{
	OLED_ShowRow1((uint8_t *)"PowerOn CDSource");
	OLED_ShowRow3((uint8_t *)connectchar);

}



void FastPreviousing(void)                              // ��ʾ������һ����Ϣ�ĺ���
{
	OLED_ShowRow1((uint8_t *)"PowerOn CDSource     ");
	OLED_ShowRow2((uint8_t *)"Fast Previousing     ");
	OLED_ShowRow3((uint8_t *)"                     ");
}



void FatsNexting(void)                                  // ��ʾ������һ����Ϣ�ĺ���
{
OLED_ShowRow1((uint8_t *)"PowerOn CDSource      ");
OLED_ShowRow2((uint8_t *)"Fast Nexting          ");
OLED_ShowRow3((uint8_t *)"                      ");
}


static void OledShowMsg(const MsgMail_t* msg)
{
    uint8_t evt = msg->eventID;

    /* POWER��OLED ���� + �� ACK �� POWER���� POWER.h �Ľ����¼��� */
    if (evt == OLED_TURN_ON) {
        OLED_CLS();
        Show_PowerOn();
        SendToPower(MOD_OLED, MOD_POWER, OLED_ON_RECEIVE_OK, NULL);
        return;
    }
    else if (evt == OLED_TURN_OFF) {
        Show_PowerOff();
        SendToPower(MOD_OLED, MOD_POWER, OLED_OFF_RECEIVE_OK, NULL);
        return;
    }
    else if (evt == OLED_TURNER_ERROR) {
        Show_PowerError();
        return;
    }

    /* CD��OLED ��������ʾ */
    {
        char conn[17] = { 0 };
        make_connect_str(msg, conn);

        switch (evt) {
        case NO_DISC:   Show_NODISC(); break;
        case LOADING:   Show_LOADING(); break;
        case EJECTING:  Show_EJECTING(); break;
        case PLAY:      Show_PLAY();  Show_MusicNum_And_Time(conn); break;
        case PAUSE:     Show_PAUSE(); Show_MusicNum_And_Time(conn); break;
        case PREVIOUS:  FastPreviousing(); Show_MusicNum_And_Time(conn); break;
        case NEXT:      FatsNexting();    Show_MusicNum_And_Time(conn); break;
        case STOP:      Show_STOP(); break;
        default:        break;
        }
    }
}

/* ---------------- �߳�/��ʼ�� ---------------- */
static void THREAD_OLED(void const* arg)
{
    (void)arg;
    for (;;) {
        osEvent e = osMailGet(g_mailq_oled, osWaitForever);
        if (e.status != osEventMail) continue;
        {
            MsgMail_t* m = (MsgMail_t*)e.value.p;
            if (m && m->targetID == MOD_OLED) {
                OledShowMsg(m);
            }
            osMailFree(g_mailq_oled, m);
        }
    }
}

void OledThreadInit(void)
{
    g_mailq_oled = osMailCreate(osMailQ(OLED_MAILQ), NULL);
    osThreadDef(THREAD_OLED, osPriorityBelowNormal, 1,300);
    OledId = osThreadCreate(osThread(THREAD_OLED), NULL);
    (void)OledId;
}

