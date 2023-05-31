#ifndef BAK_H
#define BAK_H

#include "const.h"

// 音乐相关数据
typedef struct {
  uint16_t speed;
  uint32_t score_index;
  uint8_t stop;
  uint32_t chksum;
} BACKUP_MUSIC;

// 控制相关数据
typedef struct {
  uint8_t flag1;
  uint8_t Rx1_Buffer;
  uint8_t receiving;
  uint8_t flag; 
  uint16_t speed_buffer;
  uint8_t disp_i;
  uint32_t chksum;
} BACKUP_CONTROL;

// 数码管
typedef struct {
  union {
    uint8_t buf[8];
    uint32_t v[2];
  } dat;
  uint32_t chksum;
} BACKUP_DISPLAY;

typedef BACKUP_MUSIC MDB;
typedef BACKUP_CONTROL CDB;
typedef BACKUP_DISPLAY DDB;

#define __BACKUP_ONE__ __attribute__((section("BACKUP_ONE"), zero_init))
#define __BACKUP_TWO__ __attribute__((section("BACKUP_TWO"), zero_init))

void init_mdb(void);
void init_cdb(void);
void init_ddb(void);

int restore_data(void);

void recover_mdbs(void);
void recover_cdbs(void);
void recover_ddbs(void);
void recover_backups(void);

MDB* get_correct_mdb(void);
CDB* get_correct_cdb(void);
DDB* get_correct_ddb(void);

uint16_t get_speed(void);
uint32_t get_score_index(void);
uint8_t get_stop(void);
void set_speed(uint16_t _new);
void set_score_index(uint32_t _new);
void set_stop(uint8_t _new);

uint8_t get_flag1(void);
uint8_t get_Rx1_Buffer(void);
uint8_t get_receiving(void);
uint8_t get_flag(void);
uint16_t get_speed_buffer(void);
uint8_t get_disp_i(void);
void set_flag1(uint8_t _new);
void set_Rx1_Buffer(uint8_t _new);
void set_receiving(uint8_t _new);
void set_flag(uint8_t _new);
void set_speed_buffer(uint16_t _new);
void plus_one_flag1(void);
void update_speed_buffer(void);
void plus_one_disp_i(void);

uint8_t get_disp_buf(int i);
void update_disp_left(void);
void update_disp_right(void);
void update_disp_mid(void);

#endif
