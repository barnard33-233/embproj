#include "IWDG.h"

IWDG_HandleTypeDef h_iwdg;

void IWDG_Init(void) {
    h_iwdg.Instance = IWDG;
    h_iwdg.Init.Prescaler = IWDG_PRESCALER_32;
    h_iwdg.Init.Reload = 125; // tout = 32 * 125 / 40k = 0.1
    if(HAL_IWDG_Init(&h_iwdg) != HAL_OK){
        Error_Handler(4);
    }
}

void IWDG_Start(void) {
    HAL_IWDG_Start(&h_iwdg);    
}

void IWDG_Feed(void) {
    HAL_IWDG_Refresh(&h_iwdg);
}
