#include "stdio.h"
#include "stdlib.h"
#include "IWDG.h"
#include "bak.h"
#include "i2c.h"
#include "usart.h"
#include "zlg7290.h"
#include "const.h"

#define MAX_FLAG1 3

extern int chk_speed_valid(uint16_t speed);
extern void loop_delay(int time);
extern void init_keyboard(void);
extern void print_data(void);

extern uint32_t i2c_timer;

int keyerror_cnt = 0;

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
  // receiving 标记位非 0 表示当前处于接收用户输入的状态
  if (get_receiving()) {
    if (flag <= 9) {
      // flag 为 0 到 9 意味着用户还在输入
      // 更新 speed_buffer = speed_buffer * 10 + flag
      // 在 bak 模块封装了专门的更新函数而不是 get 完算完再 set 回去
      update_speed_buffer();
      printf("Receiving... (%d)\r\n", get_speed_buffer());
    } else if (flag == 14) {
      // flag 为 # 意味着用户试图提交输入
      if(chk_speed_valid(get_speed_buffer())){
        // 不合法的输入不会被接收
        // 万一 if 被跳过，在 music 模块中也会进行检查和重置
        set_speed(get_speed_buffer());
        printf("Commit! Change speed to %d.\r\n", get_speed());
      } else {
        printf("Invalid value: %d.\r\n", get_speed_buffer());
      }
      // 重置 receiving 标志位和缓冲区
      IWDG_Feed();
      set_zero_speed_buffer();
      set_zero_receiving();
      printf("Finish receiving!\r\n");
    } else {
      // 输入非数字和 # 值则取消
      printf("Commit Cancel! Finish receiving.\r\n");
      set_zero_speed_buffer();
      set_zero_receiving();
    }
  } else {
    switch (flag) {
      case 14: set_one_receiving(); printf("Start receiving...\r\n"); break;
      case 10: set_one_stop(); printf("Pause music...\r\n"); break;
      case 11: set_zero_stop(); printf("Continue music...\r\n"); break;
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

int read_key(uint8_t *tmp) {
  int c = 0;
  *tmp = 0;
  while (*tmp > 0x1e || *tmp == 0) {
    if (c++ > 2) {
      *tmp = 0;
      return 1; // 扰动过大，放弃读入
    }
    IWDG_Feed();
    I2C_ZLG7290_Read(&hi2c1, 0x71, 0x01, tmp, 1);
  }
  return 0;
}

uint8_t input_filter(void) {
  uint8_t tmp_rx1 = 0, tmp_rx2 = 0, tmp_rx3 = 0;
  int comp_read = 0, bad_c = 0; // 序列完整性标志
  // 第一次读入
  if (read_key(&tmp_rx1) != 0) bad_c++;
  comp_read ++;
  // 第二次读入
  if (read_key(&tmp_rx2) != 0) bad_c++;
  comp_read ++;
  // 失败次数大于 2，放弃本次输入并累计错误次数
  if (bad_c >= 2) return keyerror_cnt ++, 0;
  // 第三次读入
  if (read_key(&tmp_rx3) != 0) bad_c++;
  comp_read ++;
  if (bad_c >= 2) return keyerror_cnt ++, 0;
  // 代码前序检查
  if (comp_read != 3) {
    printf("You may under an attack! Input abort!\r\n");
    return 0;
  }
  // 取众数
  uint8_t res = 0;
  if (tmp_rx1 == tmp_rx2) res = tmp_rx1;
  else if (tmp_rx1 == tmp_rx3) res = tmp_rx1;
  else if (tmp_rx2 == tmp_rx3) res = tmp_rx2;
  // 三次读入不同时累计错误次数
  keyerror_cnt += (res == 0 ? 1 : 0); 
  // 返回滤波结果
  return res;
}

// 按键处理模块
void module_Input(void) {
  // 如果因为错误被忽略的事件过多 重置 I2C 和键盘引脚
  if (keyerror_cnt > 3) {
		Error_Handler(I2C_BADSTATE);
		return;
	}
  if (get_flag1() != 0) {
    // 累计的中断过多
    if(get_flag1() >= MAX_FLAG1) {
			Error_Handler(TOO_MANY_FLAG1);
			return;
		}
    // 睡眠 2ms 来减少扰动
    do_HAL_Delay(2000);
    set_zero_flag1();
    // 获取输入
    uint8_t tmp = input_filter();
    // 无效输入直接忽略
    if (tmp == 0) return;
    // 将数据写入备份中
    IWDG_Feed();
    set_Rx1_Buffer(tmp);
    // 在输入与处理间添加随机时延
    // 数码管的写入本身与是否有输入是没有关系的，我们并不认为攻击者能通过数码管的写入来推断什么
    // 但是暂停位的设置与输入间的关系是确定的，它会明确反馈在蜂鸣器的电位变化上
    // 因此，这个随机时延是必要的，可以隐藏键盘中断到音乐暂停间的关系
    loop_delay(rand() % 100);
    // 将获取的扫描码与数字对应起来
    IWDG_Feed();
    switch_key();
    printf("Get keyvalue = %#x => flag = %d\r\n", get_Rx1_Buffer(), get_flag());
    // 针对不同的 flag 做出不同的处理
    IWDG_Feed();
    switch_flag();
  }
}
