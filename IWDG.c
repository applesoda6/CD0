#include "IWDG.h"

/* �򻯵Ŀ��Ź���ʼ�����������Ԥ��Ƶ����װֵ�� */
void IWDG_Init(void)
{
    /* LSI 40k ���裺IWDG timeout �� (Reload * Prescaler)/40kHz */
    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
    IWDG_SetPrescaler(IWDG_Prescaler_64);
    IWDG_SetReload(1250); /* ~2s */
    IWDG_ReloadCounter();
    IWDG_Enable();
}
