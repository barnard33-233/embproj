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

// 定时事件计数器
// typedef struct {
//   uint32_t music_timer;
//   uint32_t flush_timer;
//   // uint32_t delay_timer;
//   uint8_t chksum;
// } BACKUP_TIMER;

// 控制相关数据
typedef struct {
  uint8_t flag1;
  uint8_t Rx1_Buffer;
  uint8_t receiving;
  uint8_t flag; 
  uint16_t speed_buffer;
  uint32_t chksum;
} BACKUP_CONTROL;

typedef BACKUP_MUSIC MDB;
// typedef BACKUP_TIMER TDB;
typedef BACKUP_CONTROL CDB;

#define __BACKUP_ONE__ __attribute__((section("BACKUP_ONE"), zero_init))
#define __BACKUP_TWO__ __attribute__((section("BACKUP_TWO"), zero_init))

int restore_data(void);
void recover_backups(void);

MDB* get_correct_mdb(void);
CDB* get_correct_cdb(void);
// TDB* get_correct_tdb(void);

uint16_t get_speed(void);
uint32_t get_score_index(void);
uint8_t get_stop(void);
void set_speed(uint16_t _new);
void set_score_index(uint32_t _new);
void set_stop(uint8_t _new);

// uint32_t get_music_timer(void);
// uint32_t get_flush_timer(void);
// void reset_music_timer(void);
// void reset_flush_timer(void);
// void plus_music_timer(void);
// void plus_flush_timer(void);

uint8_t get_flag1(void);
uint8_t get_Rx1_Buffer(void);
uint8_t get_receiving(void);
uint8_t get_flag(void);
uint16_t get_speed_buffer(void);
void set_flag1(uint8_t _new);
void set_Rx1_Buffer(uint8_t _new);
void set_receiving(uint8_t _new);
void set_flag(uint8_t _new);
void set_speed_buffer(uint16_t _new);

void plus_flag1(void);

#endif
