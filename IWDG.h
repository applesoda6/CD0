#ifndef _IWDG_H
#define _IWDG_H

#include "stm32f10x.h"
#include "cmsis_os.h"

/* 初始化独立看门狗 */
#ifdef __cplusplus
extern "C" {
#endif

	void IWDG_Init(void);

#ifdef __cplusplus
}
#endif

#endif
