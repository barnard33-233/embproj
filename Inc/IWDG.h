#ifndef __IWDG_H
#define __IWDG_H

#include "stm32f4xx_hal.h"

// IWDG global variables
extern IWDG_HandleTypeDef h_iwdg;

extern void Error_Handler(int);

void IWDG_Init(void);
void IWDG_Start(void);
void IWDG_Feed(void);

#endif
