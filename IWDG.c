#include "IWDG.h"

/* 简化的看门狗初始化（按需调整预分频和重装值） */
void IWDG_Init(void)
{
    /* LSI 40k 假设：IWDG timeout ≈ (Reload * Prescaler)/40kHz */
    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
    IWDG_SetPrescaler(IWDG_Prescaler_64);
    IWDG_SetReload(1250); /* ~2s */
    IWDG_ReloadCounter();
    IWDG_Enable();
}
