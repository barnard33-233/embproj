/**
  ******************************************************************************
  * File Name          : main.c
  * Description        : Main program body
  ******************************************************************************
  *
  * COPYRIGHT(c) 2015 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "i2c.h"
#include "usart.h"
#include "gpio.h"

#include "zlg7290.h"
#include "stdio.h"
#include "stdlib.h"

#include "const.h"
#include "bak.h"
#include "IWDG.h"

/* Private variables ---------------------------------------------------------*/

#define SCORE_LENGTH 14
#define SPEED_BUFFER_MAX 4

const struct Note score[SCORE_LENGTH] = {
	{C4, NOTE4},
	{C4, NOTE4},
	{G4, NOTE4},
	{G4, NOTE4},
	{A4, NOTE4},
	{A4, NOTE4},
	{G4, NOTE2},
	
	{F4, NOTE4},
	{F4, NOTE4},
	{E4, NOTE4},
	{E4, NOTE4},
	{D4, NOTE4},
	{D4, NOTE4},
	{C4, NOTE2},
	
};

enum PITCH present_pitch = pause;
enum DURATION present_du = NOTE1;
uint32_t note_time = 0;
uint8_t enable_music = 0;

uint32_t flush_timer = 0;
uint32_t music_timer = 0;

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
uint32_t Du_to_us(enum DURATION du);
void Error_Handler(int err);
void switch_key(void); // 将键值转化为对应的数字
void switch_flag(void);
void HAL_delay(__IO uint32_t delay);
void print_data(void);
void HAL_SYSTICK_Callback(void);
void init_device(void);
//__STATIC_INLINE void disable_SysTick(void); 
//__STATIC_INLINE void enable_SysTick(void);
/* Private function prototypes -----------------------------------------------*/

int main(void)
{

  /* MCU Configuration */
  restore_data();

  init_device();

  printf("-------------------------------------------------\r\n");
  printf(" Muti speed music player! \r\n");
  printf("-------------------------------------------------\r\n");
  print_data();
	printf("-------------------------------------------------\r\n");
  IWDG_Feed();

  /* Infinite loop */
  while (1)
  {
    if (get_stop() == 1) enable_music = 0;
    else enable_music = 1;
    if (flush_timer /*get_flush_timer()*/ >= 500000) {
      // 定时事件 每 500 ms
      flush_timer = 0; // reset_flush_timer();
      // 同步所有备份数据
      recover_backups();
      IWDG_Feed();
    }
		if (get_flag1() == 1) {
      uint8_t tmp_rx1 = 0;
			set_flag1(0);
      IWDG_Feed();
			I2C_ZLG7290_Read(&hi2c1, 0x71, 0x01, &tmp_rx1, 1);
      set_Rx1_Buffer(tmp_rx1);
      IWDG_Feed();

			switch_key(); // 更新 flag 的值
			printf("Get keyvalue = %#x => flag = %d\r\n", get_Rx1_Buffer(), get_flag());
      switch_flag();
      IWDG_Feed();
		}
    if (enable_music == 0) continue;
    // uint32_t music_timer = get_music_timer();
		uint32_t score_index = get_score_index();
		if(music_timer >= note_time - note_time/32){
      present_pitch = pause;
    }
    if(music_timer >= note_time){
      present_du = score[score_index].duration;
      present_pitch = score[score_index].pitch;
      note_time = Du_to_us(present_du);
      set_score_index((score_index + 1) % SCORE_LENGTH);
      music_timer = 0;
    }
    IWDG_Feed();
		if(present_pitch != pause){
			HAL_GPIO_WritePin(GPIOG,GPIO_PIN_6,GPIO_PIN_SET);
			HAL_Delay(present_pitch);
      IWDG_Feed();
			HAL_GPIO_WritePin(GPIOG,GPIO_PIN_6,GPIO_PIN_RESET);
			HAL_Delay(present_pitch);
		}
    IWDG_Feed();
  }
}

/** System Clock Configuration
*/
void SystemClock_Config(void)
{

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;

  __PWR_CLK_ENABLE();

  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK|RCC_CLOCKTYPE_PCLK1
                              |RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);

  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/2000000);

  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

}

void init_device(void) {
  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();
  /* Configure the system clock */
  SystemClock_Config();
  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();
  // Init watch dag
  IWDG_Init();
}

/*__STATIC_INLINE void disable_SysTick(void) {
  SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk |   
                   //SysTick_CTRL_TICKINT_Msk   |
                   0 |
                   SysTick_CTRL_ENABLE_Msk;    
}
__STATIC_INLINE void enable_SysTick(void) {
  SysTick->CTRL  = SysTick_CTRL_CLKSOURCE_Msk |   
                   SysTick_CTRL_TICKINT_Msk   |
                   SysTick_CTRL_ENABLE_Msk;    
}*/

uint32_t Du_to_us(enum DURATION du)
{
	return (1000000 * 60 * du) / (get_speed() * NOTE4);
}

void HAL_SYSTICK_Callback(void) {
  flush_timer ++; // plus_flush_timer();
  if (enable_music == 1) music_timer ++;
}

void switch_key(void) {
  switch (get_Rx1_Buffer()) {
    case 0x1c: set_flag(1); break; // numbers
		case 0x1b: set_flag(2); break;
		case 0x1a: set_flag(3); break;
		case 0x14: set_flag(4); break;
		case 0x13: set_flag(5); break;
		case 0x12: set_flag(6); break;
    case 0x0c: set_flag(7); break;
		case 0x0b: set_flag(8); break;
		case 0x0a: set_flag(9); break;
		case 0x03: set_flag(0); break; // 0
		case 0x19: set_flag(10); break; // A
		case 0x11: set_flag(11); break; // B
		case 0x09: set_flag(12); break; // C
		case 0x01: set_flag(13); break; // D
		case 0x02: set_flag(14); break; // #
		case 0x04: set_flag(15); break; // *
		default: break;
  }
}

void switch_flag(void){
  uint8_t flag = get_flag();
  if (get_receiving()) { // receiving user input.
    uint16_t speed_buffer = get_speed_buffer();
    if (flag <= 9) {
      set_speed_buffer(speed_buffer * 10 + flag);
      printf("Receiving... (%d)\r\n", speed_buffer * 10 + flag);
    } else if (flag == 14) { // commit
      if(speed_buffer <= 170 && speed_buffer >= 50){ // valid value
        set_speed(speed_buffer);
        printf("Commit! Change speed to %d.\r\n", get_speed());
      } else{
        printf("Invalid value: %d.\r\n", speed_buffer);
      }
			set_speed_buffer(0);
      set_receiving(0);
      printf("Finish receiving!\r\n");
    } else {
      printf("Commit Cancel! Finish receiving.\r\n");
			set_speed_buffer(0);
      set_receiving(0);
    }
  } else {
    switch (flag) {
      case 14: set_receiving(1); break;
      case 10: set_stop(1); break;
      case 11: set_stop(0); break;
      default: break;
    }
  }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	set_flag1(1);
}

int fputc(int ch, FILE *f)
{ 
  uint8_t tmp[1]={0};
	tmp[0] = (uint8_t)ch;
	HAL_UART_Transmit(&huart1,tmp,1,10000);	
	return ch;
}

void print_data(void) {
	printf("Here is our datas: \r\n");
	printf("Current speed is %d \r\n", (int)get_speed());
	printf("Score index is %d \r\n", (int)get_score_index());
	if (get_stop() != 0) {
		printf("Playing pause now \r\n");
	} else {
		printf("Playing will continue \r\n");
	}
}

// HAL_delay 重写
void HAL_delay(__IO uint32_t delay) {
  uint32_t start = 0, end = 0;
  uint32_t now = 0, past = 0;
  uint32_t count = 0;
  now = past = start = HAL_GetTick();
  end = start + delay;
  while( (end < start && (now >= start || now <= end)) || (start <= end && (now >= start && now <= end))) {
    now = HAL_GetTick();
    if (now == past) {
      // 如果 now 在 10000 次循环后仍然没有变化
      // 说明定时器出错
      if (++count > 10000) Error_Handler(3);
    } else {
      past = now, count = 0;
    }
  }
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
void Error_Handler(int err)
{
  /* USER CODE BEGIN Error_Handler */
  /* User can add his own implementation to report the HAL error return state */
  // TODO: 这里该写什么呢 ?
  printf("\n\r!! Error Handler !!\r\n");
  if (err == 1) {
    printf("@ It looks like all backups of mdb broken!\r\n");
  } else if (err == 2) {
    printf("@ It looks like all backups of cdb broken!\r\n");
  } else if (err == 3) {
    printf("@ HAL_Delay timeout!\r\n");
  } else if (err == 4) {
		printf("@ HAL_IWDG_Init failed!\r\n");
	} else {
    printf("@ Receive an error code: %d!\r\n", err);
  }
  while(1) 
  {
  }
  /* USER CODE END Error_Handler */ 
}

#ifdef USE_FULL_ASSERT

/**
   * @brief Reports the name of the source file and the source line number
   * where the assert_param error has occurred.
   * @param file: pointer to the source file name
   * @param line: assert_param error line source number
   * @retval None
   */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
    ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
  // TODO: 这里该加什么呢 ?
}

#endif


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
