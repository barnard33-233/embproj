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
#define DISPLAY_BUFFER_MAX 4

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

int comp_flag;

uint32_t flush_timer = 0;
uint32_t music_timer = 0;

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
uint32_t Du_to_us(enum DURATION du);
void Error_Handler(int err);
void HAL_delay(__IO uint32_t delay);
void print_data(void);
void HAL_SYSTICK_Callback(void);
void init_device(void);
void loop_delay(int time);
void init_keyboard(void);
void init_beep(void);
void init_uart(void);
int chk_speed_valid(uint16_t speed);

void module_TimeEvent(void);
void module_Music(void);
extern void module_Input(void);
//__STATIC_INLINE void disable_SysTick(void); 
//__STATIC_INLINE void enable_SysTick(void);
/* Private function prototypes -----------------------------------------------*/

// 定期事件模块
void module_TimeEvent(void) {
  // 控制 music_timer 是否计数
  if (get_stop() == 1) enable_music = 0;
  else enable_music = 1;
  if (flush_timer /*get_flush_timer()*/ >= 40000) {
    // 定时事件 每 40 ms
    flush_timer = 0; // reset_flush_timer();
    // 同步所有备份数据
    IWDG_Feed();
    recover_backups();
    // 初始化设备
    IWDG_Feed();
    init_beep();
    // init_keyboard();
    // init_uart();
    // printf("Fresh beep and backups...\r\n");
  } else {
    HAL_delay(50 + rand() % 10);
  }
}

void module_Music(void) {
  // 音符播放模块
  if (get_stop() == 1) return;
  uint32_t score_index = get_score_index();
  if(music_timer >= note_time - note_time/32){
    present_pitch = pause;
  }
  if(music_timer >= note_time){
    present_du = score[score_index].duration;
    present_pitch = score[score_index].pitch;
    if (!chk_speed_valid(get_speed())) {
      // 速度复原
      IWDG_Feed();
      printf("Bad speed value. You may under an attack.\r\n");
      printf("Reset speed to 120.\r\n");
      set_speed(120);
    }
    // 写入音符信息
    IWDG_Feed();
    note_time = Du_to_us(present_du);
    set_score_index((score_index + 1) % SCORE_LENGTH);
    music_timer = 0;
  } else {
    HAL_delay(10 + rand() % 10);
  }
  if(present_pitch != pause){
    IWDG_Feed();
    HAL_GPIO_WritePin(GPIOG,GPIO_PIN_6,GPIO_PIN_SET);
    HAL_Delay(present_pitch);
    IWDG_Feed();
    HAL_GPIO_WritePin(GPIOG,GPIO_PIN_6,GPIO_PIN_RESET);
    HAL_Delay(present_pitch);
  }
}

void refresh_Display(void) {
  // convert data to write buffer
  static int last_fresh = -1;
  if (flush_timer / 2333 != last_fresh) {
    IWDG_Feed();
    update_disp_left();
    update_disp_right();
    update_disp_mid();
    last_fresh = flush_timer / 2333;
  } else {
    HAL_delay(5 + rand() % 5);
  }
}

void do_Display(void) {
  // convert write buffer to i2c one by one
  static int last_fresh = -1, badc = 0;
  if (flush_timer / 5000 != last_fresh) {
    IWDG_Feed();
    uint8_t i = get_disp_i();
    if (I2C_ZLG7290_WriteOneByte(&hi2c1, 0x70, 0x10 + i, get_disp_buf(i)) == 0) {
      plus_one_disp_i();
      badc = 0;
    } else if (badc > 10) {
      printf("Bad I2C state. Reset it.\r\n");
      MX_I2C1_Init();
    }
    last_fresh = flush_timer / 5000;
  } else {
    HAL_delay(100 + rand() % 20);
  }
}

int main(void)
{
  /* MCU Configuration */
  int hot = restore_data();
  // delay when cold boot
  if (hot == 0) loop_delay(1000);

  init_device();
  IWDG_Start();
  
  IWDG_Feed();
  if (hot == 1) printf("A hot booting... \r\n");
  printf("-------------------------------------------------\r\n");
  printf(" Muti speed music player! \r\n");
  printf("-------------------------------------------------\r\n");
  IWDG_Feed();
  print_data();
	printf("-------------------------------------------------\r\n");

  refresh_Display();

  srand(HAL_GetTick());
  /* Infinite loop */
  while (1)
  {
		int t = rand() % 3;
    if (t == 0) {
      refresh_Display();
      module_TimeEvent();
      do_Display();
      module_Input();
      module_Music();
    } else if (t == 1) {
      module_Input();
      do_Display();
      refresh_Display();
      module_TimeEvent();
      module_Music();
    } else {
      module_Input();
      refresh_Display();
      do_Display();
      module_TimeEvent();
      module_Music();
    }
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

void init_keyboard(void) {
  GPIO_Init_Keyboard();
}

void init_beep(void) {
  GPIO_Init_Beep();
}

void init_uart(void) {
  __GPIOA_CLK_ENABLE();
  MX_USART1_UART_Init();
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

int chk_speed_valid(uint16_t speed) {
  return speed >= 50 && speed <= 170;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	plus_one_flag1();
}

int fputc(int ch, FILE *f)
{ 
  uint8_t tmp[1]={0}, c = 0;
	tmp[0] = (uint8_t)ch;
	while (HAL_OK != HAL_UART_Transmit(&huart1, tmp, 1, 10000)) {
    c++;
    if (c == 2) init_uart();
    else if (c >= 4) Error_Handler(0);
  }	
	return ch;
}

void print_data(void) {
	printf("Here is our datas: \r\n");
	printf("Current speed is %d \r\n", (int)get_speed());
  IWDG_Feed();
	printf("Score index is %d \r\n", (int)get_score_index());
	if (get_stop() != 0) {
		printf("Playing pause now \r\n");
	} else {
		printf("Playing will continue \r\n");
	}
  if (get_receiving() != 0) {
    printf("Receiving... (%d) \r\n", (int)get_speed_buffer());
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
    // feed IWDG
    IWDG_Feed();
    // 进入休眠模式
    HAL_PWR_EnterSLEEPMode(PWR_LOWPOWERREGULATOR_ON, PWR_SLEEPENTRY_WFI);
    now = HAL_GetTick();
    if (now == past) {
      // 如果 now 在 20 次循环后仍然没有变化
      // 说明定时器出错
      if (++count > 20) Error_Handler(DELAY_TIMEOUT);
    } else {
      past = now, count = 0;
    }
  }
}

void loop_delay(int time){
  while(time--);
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  None
  * @retval None
  */
void Error_Handler(int err)
{
  IWDG_Feed();
  printf("\n\r!! Error Handler !!\r\n");
  switch (err) {
    case MDB_DESTORY: {
      printf("@ It looks like all backups of mdb broken!\r\n"); 
      break;
    }
    case CDB_DESTORY: {
      printf("@ It looks like all backups of cdb broken!\r\n");
      break;
    }
    case DELAY_TIMEOUT: {
      printf("@ HAL_Delay timeout!\r\n");
      break;
    }
    case IWDG_INIT_ERROR: {
      printf("@ HAL_IWDG_Init failed!\r\n");
      break;
    }
    case DDB_DESTORY: {
      printf("@ It looks like all backups of ddb broken!\r\n");
      break;
    }
    case I2C_BADSTATE: {
      printf("@ Maybe something wrong with I2C!\r\n");
      break;
    }
    default: {
      printf("@ Receive an error code: %d\r\n", err);
      break;
    }
  }
  while(1); // loop to meet an hot boot
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
