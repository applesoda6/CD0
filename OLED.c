#include "OLED.h"
#include "POWER.h"
#include "CD.h"
#include "stdio.h"
#include <string.h>

/* ---------------- RTOS 资源：OLED 接收邮箱 + 线程 ---------------- */
osMailQDef(OLED_MAILQ, 16, MsgMail_t);
osMailQId g_mailq_oled = NULL;

static osThreadId OledId;

/* ---------------- 显示工具 ---------------- */
static void make_connect_str(const MsgMail_t* m, char buf[17])
{
    uint32_t trk = m->option[0];
    uint32_t mm = m->option[1];
    uint32_t ss = m->option[2];
    snprintf(buf, 17, "TRK:%02lu %02lu:%02lu",
        (unsigned long)trk, (unsigned long)mm, (unsigned long)ss);
}

void Show_PowerOn(void)                                  // 显示电源开启信息的函数
{
	OLED_ShowRow1((uint8_t *)"Power On            ");
	OLED_ShowRow2((uint8_t *)"NO DISC             ");
	OLED_ShowRow3((uint8_t *)"                    ");
}




void Show_PowerOff(void)                                // 显示电源关闭信息的函数
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

void Show_NODISC(void)                                 // 显示没有CD信息的函数
{
  OLED_ShowRow1((uint8_t *)"Power On            ");
	OLED_ShowRow2((uint8_t *)"NO DISC             ");
	OLED_ShowRow3((uint8_t *)"                    ");
}



void Show_LOADING(void)                                 // 显示加载信息的函数
{
	OLED_ShowRow1((uint8_t *)"PowerOn CDSource    ");
	OLED_ShowRow2((uint8_t *)"LOADING             ");
	OLED_ShowRow3((uint8_t *)"                    ");
}



void Show_EJECTING(void)                                // 显示弹出信息的函数
{
	OLED_ShowRow1((uint8_t *)"Power On            ");
	OLED_ShowRow2((uint8_t *)"EJECTING            ");
	OLED_ShowRow3((uint8_t *)"                    ");
}



void Show_PLAY(void)                                    // 显示播放信息的函数
{
	OLED_ShowRow1((uint8_t *)"PowerOn CDSource      ");
	OLED_ShowRow2((uint8_t *)"PLAY                  ");
	OLED_ShowRow3((uint8_t *)"                      ");
}



void Show_PAUSE(void)                                   // 显示暂停信息的函数
{
	OLED_ShowRow1((uint8_t *)"PowerOn CDSource     ");
	OLED_ShowRow2((uint8_t *)"PAUSE                ");
	OLED_ShowRow3((uint8_t *)"                     ");
}



void Show_STOP(void)                                    // 显示停止播放信息的函数
{
	OLED_ShowRow1((uint8_t *)"PowerOn CDSource    ");
	OLED_ShowRow2((uint8_t *)"STOP                ");
	OLED_ShowRow3((uint8_t *)"                    ");
}



void Show_MusicNum_And_Time(const char *connectchar)                          // 显示音乐顺序信息的函数
{
	OLED_ShowRow1((uint8_t *)"PowerOn CDSource");
	OLED_ShowRow3((uint8_t *)connectchar);

}



void FastPreviousing(void)                              // 显示快速上一曲信息的函数
{
	OLED_ShowRow1((uint8_t *)"PowerOn CDSource     ");
	OLED_ShowRow2((uint8_t *)"Fast Previousing     ");
	OLED_ShowRow3((uint8_t *)"                     ");
}



void FatsNexting(void)                                  // 显示快速下一曲信息的函数
{
OLED_ShowRow1((uint8_t *)"PowerOn CDSource      ");
OLED_ShowRow2((uint8_t *)"Fast Nexting          ");
OLED_ShowRow3((uint8_t *)"                      ");
}


static void OledShowMsg(const MsgMail_t* msg)
{
    uint8_t evt = msg->eventID;

    /* POWER→OLED 命令 + 回 ACK 给 POWER（用 POWER.h 的接收事件） */
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

    /* CD→OLED 的内容显示 */
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

/* ---------------- 线程/初始化 ---------------- */
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

