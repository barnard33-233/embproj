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
extern void HAL_Delay(__IO uint32_t delay);

int keyerror_cnt = 0;

void switch_key(void) { // ɨ����ת����
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

int read_key(uint8_t *tmp) {
  int c = 0;
  *tmp = 0;
  while (*tmp > 0x1e || *tmp == 0) {
    if (c++ > 2) {
      *tmp = 0;
      return 1; // �Ŷ����󣬷�������
    }
    IWDG_Feed();
    I2C_ZLG7290_Read(&hi2c1, 0x71, 0x01, tmp, 1);
  }
  return 0;
}

uint8_t input_filter(void) {
  uint8_t tmp_rx1 = 0, tmp_rx2 = 0, tmp_rx3 = 0;
  int comp_flag = 0, bad_c = 0; // ���������Ա�־
  // ��һ�ζ���
  if (read_key(&tmp_rx1) != 0) bad_c++;
  comp_flag ++;
  // �ڶ��ζ���
  if (read_key(&tmp_rx2) != 0) bad_c++;
  comp_flag ++;
  // ʧ�ܴ������� 2�������������벢�ۼƴ������
  if (bad_c >= 2) return keyerror_cnt ++, 0;
  // �����ζ���
  if (read_key(&tmp_rx3) != 0) bad_c++;
  comp_flag ++;
  if (bad_c >= 2) return keyerror_cnt ++, 0;
  // ����ǰ����
  if (comp_flag != 3) {
    printf("You may under an attack! Input abort!\r\n");
    return 0;
  }
  // ȡ����
  uint8_t res = 0;
  if (tmp_rx1 == tmp_rx2) res = tmp_rx1;
  else if (tmp_rx1 == tmp_rx3) res = tmp_rx1;
  else if (tmp_rx2 == tmp_rx3) res = tmp_rx2;
  // ���ζ��벻ͬʱ�ۼƴ������
  keyerror_cnt += (res == 0 ? 1 : 0); 
  // �����˲����
  return res;
}

// ��������ģ��
void module_Input(void) {
  // �����Ϊ���󱻺��Ե��¼����� ���� I2C �ͼ�������
  if (keyerror_cnt > 3) {
		Error_Handler(I2C_BADSTATE);
		return;
	}
  if (get_flag1() != 0) {
    // �ۼƵ��жϹ���
    if(get_flag1() >= MAX_FLAG1) {
			Error_Handler(TOO_MANY_FLAG1);
			return;
		}
    // ˯�� 2ms �������Ŷ�
    HAL_Delay(2000);
    set_flag1(0);
    // ��ȡ����
    uint8_t tmp = input_filter();
    if (tmp == 0) return; // ��Ч����ֱ�Ӻ���
    // ������д�뱸����
    IWDG_Feed();
    set_Rx1_Buffer(tmp);
    // �������봦���������ʱ��
    loop_delay(rand() % 100);
    // ����ȡ��ɨ���������ֶ�Ӧ����
    IWDG_Feed();
    switch_key();
    printf("Get keyvalue = %#x => flag = %d\r\n", get_Rx1_Buffer(), get_flag());
    // ��Բ�ͬ�� flag ������ͬ�Ĵ���
    IWDG_Feed();
    switch_flag();
  }
}
