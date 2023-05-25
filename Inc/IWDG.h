#ifndef __IWDG_H
#define __IWDG_H

#include "stm32f4xx_hal.h"

// IWDG global variables
extern IWDG_HandleTypeDef h_iwdg;

extern void Error_Handler(void);

void IWDG_Init(void);

__STATIC_INLINE void IWDG_Feed(void){
    HAL_IWDG_Refresh(&h_iwdg);
}

#endif