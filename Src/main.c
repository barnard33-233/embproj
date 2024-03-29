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

// #define DEBUG_TEST_DURING

#define DURING_TIME_EVENT 59
#define DURING_REFRESH_RAM 39
#define DURING_REFRESH_TUBE 449
#define DURING_I2C_REINIT 75
#define DURING_EXPAND_REINIT (DURING_REFRESH_TUBE-DURING_I2C_REINIT)
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

int comp_flag = 0;

uint32_t flush_timer = 0;
uint32_t music_timer = 0;
uint32_t i2c_timer = 0;

int suc_delay = 0;

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
uint32_t Du_to_us(enum DURATION du);
void Error_Handler(int err);
void HAL_Delay(__IO uint32_t delay);
void print_data(void);
void HAL_SYSTICK_Callback(void);
void init_device(void);
void loop_delay(int time);
void init_keyboard(void);
void init_beep(void);
void init_uart(void);
void reinit_i2c(void);
int chk_speed_valid(uint16_t speed);

void module_TimeEvent(void);
void module_Music(void);
extern void module_Input(void);
void do_I2C_regular(void);
//__STATIC_INLINE void disable_SysTick(void); 
//__STATIC_INLINE void enable_SysTick(void);
/* Private function prototypes -----------------------------------------------*/

// 定期事件模块
void module_TimeEvent(void) {
  // 控制 music_timer 是否计数
  if (get_stop() == 1) enable_music = 0;
  else enable_music = 1;
  // 定时事件 每 100 ms
  if (flush_timer >= 100000) {
#ifdef DEBUG_TEST_DURING
    int t = HAL_GetTick(), e;
#endif
    flush_timer = 0;
    // 重新刷新蜂鸣器的引脚和中断标志位
    // 串口的刷新和 I2C 的刷新不在这里
    init_keyboard();
    init_beep();
    // 检查备份
    recover_backups();
    // 设置时间中断为开
    __HAL_RCC_PWR_CLK_ENABLE();
#ifdef DEBUG_TEST_DURING
    e = HAL_GetTick();
    printf("flush devices: %d\r\n", e - t);
#endif
  } else {
    // 模块后的 Delay 都是为了稳定波形
    // rand() 是因为上面的代码执行时间一定存在波动
    do_HAL_Delay(DURING_TIME_EVENT + rand() % 10);
  }
  comp_flag |= 1;
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
      // 理论上，速度值不合法只有一种可能性:
      // 在将 speed_buffer 写入 speed 时的判断因为不明原因被跳过
      printf("Bad speed value. You may under an attack.\r\n");
      printf("Reset speed to 120.\r\n");
      set_speed(120);
    }
    // 刷新音符时间
    note_time = Du_to_us(present_du);
    // 将下一个音符写入备份中
    set_score_index((score_index + 1) % SCORE_LENGTH);
    // 重置时间计数器
    music_timer = 0;
  }
  if(present_pitch != pause){
    // 波形模拟
    HAL_GPIO_WritePin(GPIOG,GPIO_PIN_6,GPIO_PIN_SET);
    do_HAL_Delay(present_pitch);
    HAL_GPIO_WritePin(GPIOG,GPIO_PIN_6,GPIO_PIN_RESET);
    do_HAL_Delay(present_pitch - 850); // 修音
  }
	IWDG_Feed();
}

void refresh_Display(void) {
  // 刷新需要显示的值 (保存在备份中的 buf)
  static int last_fresh = -1;
  // 每 10ms 刷新一次显示值
  if (flush_timer / 10000 != last_fresh) {
#ifdef DEBUG_TEST_DURING
    int t = HAL_GetTick(), e;
#endif
    // 左侧显示速度
    update_disp_left();
    // 右边显示输入状态
    update_disp_right();
    // 中间显示开关状态
    update_disp_mid();
    last_fresh = flush_timer / 10000;
#ifdef DEBUG_TEST_DURING
    e = HAL_GetTick();
    printf("refresh display: %d\r\n", e - t);
#endif
  } else {
    do_HAL_Delay(DURING_REFRESH_RAM + rand() % 10);
  }
  comp_flag |= 2;
}

void do_I2C_regular(void) {
  // 往数码管的显示缓冲区写一个字节
  static int badc = 0;
  static uint8_t cnt = 0;
  // 每 50ms 进行一次
  if (i2c_timer > 50000) {
    cnt++;
    // 每刷新 7 次数码管就重刷一次 I2C
    if (cnt & 7) {
#ifdef DEBUG_TEST_DURING
      int t = HAL_GetTick(), e;
#endif
      // 获取当前需要显示的是第几个数码管
      uint8_t i = get_disp_i();
      if (I2C_ZLG7290_WriteOneByte(&hi2c1, 0x70, 0x10 + i, get_disp_buf(i)) == 0) {
        // 成功写入 自加
        plus_one_disp_i();
        badc = 0;
      } else {
        badc++;
        // 如果已经有多次写入失败，调用 Error_Handler 进行 I2C 重置
        if (badc > 2) Error_Handler(I2C_BADSTATE);
      }
#ifdef DEBUG_TEST_DURING
      e = HAL_GetTick();
      printf("do display: %d\r\n", e - t);
#endif
    } else {
#ifdef DEBUG_TEST_DURING
      int t = HAL_GetTick(), e;
#endif
      IWDG_Feed();
      reinit_i2c(); // 重新初始化 i2c 和它的两个引脚
      do_HAL_Delay(DURING_EXPAND_REINIT + rand() % 10);
#ifdef DEBUG_TEST_DURING
      e = HAL_GetTick();
      printf("reinit I2C: %d\r\n", e - t);
#endif
    }
    i2c_timer = 0;
  } else {
    do_HAL_Delay(DURING_REFRESH_TUBE + rand() % 10);
  }
  comp_flag |= 4;
}

void fix_pre_runing(void) {
  // 代码前序检查
  // 如果发现前面有模块被跳过 就进行补齐
  // 此处的目的是维护程序的可靠性: 
  // 因为如果前面的模块没被执行，music 的波形会出现失真
  if (comp_flag != 7) Error_Handler(-1);
  comp_flag = 0;
}

int main(void)
{
  // 如果是热启动，会从内存中恢复数据
  int hot = restore_data();
  // 如果是冷启动，会进入一段 loop 来等待设备上电
  if (hot == 0) loop_delay(1000);

  // 初始化一切并开始看门狗计时
  init_device();
  IWDG_Start();
  
  // 调试信息
  IWDG_Feed();
  if (hot != 0) printf("A hot booting... \r\n");
  printf("-------------------------------------------------\r\n");
  printf(" Muti speed music player! \r\n");
  printf("-------------------------------------------------\r\n");
  print_data();
	printf("-------------------------------------------------\r\n");

  // 刷新一次待显示内容
  refresh_Display();

  srand(HAL_GetTick());
  
  /* Infinite loop */
  while (1)
  {
    // 随机乱序执行各个模块
    // 音乐模块总是放在最后，是为了利用相对固定前面的事件时延，确保模拟波形稳定
		int t = rand() % 3;
    if (t == 0) {
      refresh_Display();
      module_TimeEvent();
      do_I2C_regular();
      module_Input();
    } else if (t == 1) {
      module_Input();
      do_I2C_regular();
      refresh_Display();
      module_TimeEvent();
    } else {
      module_Input();
      refresh_Display();
      do_I2C_regular();
      module_TimeEvent();
    }
    // 前序检查并修复 以此保证波形稳定
    fix_pre_runing();
    module_Music();
  }
}

// 配置系统时钟
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
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/2000000); // 1us
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

void reinit_i2c(void) {
  HAL_I2C_DeInit(&hi2c1);
  MX_I2C1_Init();
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
  flush_timer ++; i2c_timer ++;
  if (enable_music != 0) music_timer ++;
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
  // 10000 对应 timeout = 10ms
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

// HAL_Delay 重写
void HAL_Delay(__IO uint32_t delay) {
  if (delay > 0x4FFFFFFF) return; // delay is bad
  uint32_t start = 0, end = 0;
  uint32_t now = 0, past = 0;
  uint32_t count = 0;
  now = past = start = HAL_GetTick();
  end = start + delay;
  // 处理了 HAL_GetTick 翻转的可能
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
  suc_delay = 1;
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
      printf("@ It looks like all backups of mdb broken! I will reset it.\r\n");
      init_mdb(); // 重置 mdb
      return;
    }
    case CDB_DESTORY: {
      printf("@ It looks like all backups of cdb broken!\r\n");
      init_cdb(); // 重置 cdb
      return;
    }
    case DELAY_TIMEOUT: {
      printf("@ HAL_Delay timeout!\r\n");
      break; // break 到死循环由看门狗复位
    }
    case IWDG_INIT_ERROR: {
      printf("@ HAL_IWDG_Init failed!\r\n");
      init_device(); // 从头再来
      return;
    }
    case DDB_DESTORY: {
      printf("@ It looks like all backups of ddb broken!\r\n");
      init_ddb(); // 重置 ddb
      return;
    }
    case I2C_BADSTATE: {
      printf("@ Maybe something wrong with I2C!\r\n");
      IWDG_Feed();
      GPIO_Init_Keyboard();
      reinit_i2c();
      do_HAL_Delay(5000);
      return;
    }
    /*case BAD_READ_INPUT: {
      printf("@ Read three times differ!\r\n");
      break;
    }*/
    case TOO_MANY_FLAG1: {
      printf("@ Flag1 too large!\r\n");
      IWDG_Feed();
      init_keyboard();
      set_zero_flag1();
      return;
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
