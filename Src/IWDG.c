#include "IWDG.h"

IWDG_HandleTypeDef h_iwdg;

void IWDG_Init(void){
    h_iwdg.Instance = IWDG;
    h_iwdg.Init.Prescaler = IWDG_PRESCALER_64;
    h_iwdg.Init.Reload = 625;
    if(HAL_IWDG_Init(&h_iwdg) != HAL_OK){
        Error_Handler();
    }
}
