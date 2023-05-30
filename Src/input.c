#include "stdio.h"
#include "stdlib.h"
#include "IWDG.h"
#include "bak.h"
#include "i2c.h"
#include "usart.h"
#include "zlg7290.h"

#define MAX_FLAG1 3

extern int chk_speed_valid(uint16_t speed);
extern void loop_delay(int time);
extern void init_keyboard(void);
void print_data(void);


void switch_key(void) { // 扫描码转数字
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
    if (flag <= 9) {
      update_speed_buffer();
      printf("Receiving... (%d)\r\n", get_speed_buffer());
    } else if (flag == 14) { // commit
      if(chk_speed_valid(get_speed_buffer())){ // valid value
        set_speed(get_speed_buffer());
        printf("Commit! Change speed to %d.\r\n", get_speed());
      } else{
        printf("Invalid value: %d.\r\n", get_speed_buffer());
      }
      IWDG_Feed();
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
      case 14: set_receiving(1); printf("Start receiving...\r\n"); break;
      case 10: set_stop(1); printf("Pause music...\r\n"); break;
      case 11: set_stop(0); printf("Continue music...\r\n"); break;
      case 15: {
        IWDG_Feed();
        printf("-------------------------------------------------\r\n");
        print_data();
        printf("-------------------------------------------------\r\n");
        break;
      }
      default: break;
    }
  }
}

// 按键处理模块
void module_Input(void) {
  if (get_flag1() >= 1) {
    if(get_flag1() >= MAX_FLAG1) {
      IWDG_Feed();
      init_keyboard();
			set_flag1(0);
			return;
    }
    uint8_t tmp_rx1 = 0, tmp_rx2 = 0, tmp_rx3 = 0;
    set_flag1(0);
    int comp_flag = 0; // 序列完整性标志
    IWDG_Feed();
    I2C_ZLG7290_Read(&hi2c1, 0x71, 0x01, &tmp_rx1, 1);
    comp_flag |= 1;
    IWDG_Feed();
    I2C_ZLG7290_Read(&hi2c1, 0x71, 0x01, &tmp_rx2, 1);
    comp_flag |= 2;
    if (tmp_rx2 != tmp_rx1) {
      IWDG_Feed();
      I2C_ZLG7290_Read(&hi2c1, 0x71, 0x01, &tmp_rx3, 1);
      if (tmp_rx1 == tmp_rx2) tmp_rx1 = tmp_rx1;
      else if (tmp_rx1 == tmp_rx3) tmp_rx1 = tmp_rx3;
      else if (tmp_rx2 == tmp_rx3) tmp_rx1 = tmp_rx2;
      else Error_Handler(5);
    }
    set_Rx1_Buffer(tmp_rx1);
    comp_flag |= 4;
    IWDG_Feed();
    // 在输入与处理间添加随机时延
    loop_delay(rand() % 100);
    if (comp_flag == 7) {
      IWDG_Feed();
      switch_key(); // 更新 flag 的值 
      printf("Get keyvalue = %#x => flag = %d\r\n", get_Rx1_Buffer(), get_flag());
      IWDG_Feed();
      switch_flag();
    } else {
      // 不等于 3 意味着攻击者跳过了前面的语句
      IWDG_Feed();
      printf("You may under an attack. Input Abort.\r\n");
    }
  }
}
