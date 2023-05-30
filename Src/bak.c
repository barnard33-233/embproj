#include "bak.h"
#include "const.h"

MDB mdb0;
MDB __BACKUP_ONE__ mdb1;
MDB __BACKUP_TWO__ mdb2;

CDB cdb0;
CDB __BACKUP_ONE__ cdb1;
CDB __BACKUP_TWO__ cdb2;

MDB *pmdb = NULL;
CDB *pcdb = NULL;

extern void Error_Handler(int err);

uint32_t get_chksum_mdb(MDB *p) {
  return (uint32_t)(p->score_index << 24lu) + (p->speed << 16lu) + (p->stop << 8lu) + 233lu;
}
uint32_t get_chksum_cdb(CDB *p) {
  return (uint32_t)13lu + (p->flag1 << 4lu) \
  + (p->flag << 8lu) + (p->Rx1_Buffer << 12lu) \
  + (p->receiving << 20lu) + (p->speed_buffer << 24lu);
}

__STATIC_INLINE void init_mdb0(void) {
  mdb0.speed = 120;
  mdb0.score_index = 0;
  mdb0.stop = 0;
  mdb0.chksum = get_chksum_mdb(&mdb0);
}

__STATIC_INLINE void init_cdb0(void) {
  cdb0.flag1 = 0;
  cdb0.Rx1_Buffer = 0;
  cdb0.receiving = 0;
  cdb0.flag = 0xff;
  cdb0.speed_buffer = 0;
  cdb0.chksum = get_chksum_cdb(&cdb0);
}

int restore_data(void) {
  int hot = 0;
  // restore music data
  if (get_chksum_mdb(&mdb1) == mdb1.chksum) {
    mdb0 = mdb2 = mdb1;
    hot = 1;
  } else if (get_chksum_mdb(&mdb2) == mdb2.chksum) {
    mdb0 = mdb1 = mdb2;
    hot = 1;
  } else {
    init_mdb0();
    mdb1 = mdb2 = mdb0;
  }
  pmdb = &mdb0;
  if (get_chksum_cdb(&cdb1) == cdb1.chksum) {
    cdb0 = cdb2 = cdb1;
    hot = 1;
  } else if (get_chksum_cdb(&cdb2) == cdb2.chksum) {
    cdb0 = cdb1 = cdb2;
    hot = 1;
  } else {
    init_cdb0();
    cdb1 = cdb2 = cdb0;
  }
  pcdb = &cdb0;
  return hot;
}

void recover_backups(void) {
  if (get_chksum_mdb(&mdb0) == mdb0.chksum) {
    mdb1 = mdb2 = mdb0;
  } else if (get_chksum_mdb(&mdb1) == mdb1.chksum) {
    mdb0 = mdb2 = mdb1;
  } else if (get_chksum_mdb(&mdb2) == mdb2.chksum) {
    mdb0 = mdb1 = mdb2;
  } else {
    Error_Handler(1);
  }
  pmdb = &mdb0;
  if (get_chksum_cdb(&cdb0) == cdb0.chksum) {
    cdb1 = cdb2 = cdb0;
  } else if (get_chksum_cdb(&cdb1) == cdb1.chksum) {
    cdb0 = cdb2 = cdb1;
  } else if (get_chksum_cdb(&cdb2) == cdb2.chksum) {
    cdb0 = cdb1 = cdb2;
  } else {
    Error_Handler(2);
  }
  pcdb = &cdb0;
}

MDB* get_correct_mdb(void) {
  if (pmdb != NULL && get_chksum_mdb(pmdb) == pmdb->chksum)
    return pmdb;
  if (get_chksum_mdb(&mdb0) == mdb0.chksum) 
    return pmdb = &mdb0;
  if (get_chksum_mdb(&mdb1) == mdb1.chksum)
    return pmdb = &mdb1;
  if (get_chksum_mdb(&mdb2) == mdb2.chksum) 
    return pmdb = &mdb2;
  Error_Handler(1);
  return NULL;
}

CDB* get_correct_cdb(void) {
  if (pcdb != NULL && get_chksum_cdb(pcdb) == pcdb->chksum)
    return pcdb;
  if (get_chksum_cdb(&cdb0) == cdb0.chksum)
    return pcdb = &cdb0;
  if (get_chksum_cdb(&cdb1) == cdb1.chksum)
    return pcdb = &cdb1;
  if (get_chksum_cdb(&cdb2) == cdb2.chksum)
    return pcdb = &cdb2;
  Error_Handler(2);
  return NULL;
}

// music data
uint16_t get_speed(void) {
  return get_correct_mdb()->speed;
}
uint32_t get_score_index(void) {
  return get_correct_mdb()->score_index;
}
uint8_t get_stop(void) {
  return get_correct_mdb()->stop;
}

#define MDB_UPD_VALUE(var_name, new_value) \
  MDB *p = get_correct_mdb(); \
  p->##var_name = new_value; \
  p->chksum = get_chksum_mdb(p);

#define MDB_PLUS_VALUE(var_name, plus_value) \
  MDB *p = get_correct_mdb(); \
  p->##var_name += plus_value; \
  p->chksum = get_chksum_mdb(p);

#define MDB_UPD_ALL(var_name, new_value) \
  mdb0.##var_name = new_value, mdb0.chksum = p->chksum; \
  mdb1.##var_name = new_value, mdb1.chksum = p->chksum; \
  mdb2.##var_name = new_value, mdb2.chksum = p->chksum;

void set_speed(uint16_t _new) {
  MDB_UPD_VALUE(speed, _new);
  MDB_UPD_ALL(speed, p->speed);
}
void set_score_index(uint32_t _new) {
  MDB_UPD_VALUE(score_index, _new);
  MDB_UPD_ALL(score_index, p->score_index);
}
void set_stop(uint8_t _new) {
  MDB_UPD_VALUE(stop, _new);
  MDB_UPD_ALL(stop, p->stop);
}

// control
uint8_t get_flag1(void) {
  return get_correct_cdb()->flag1;
}
uint8_t get_Rx1_Buffer(void) {
  return get_correct_cdb()->Rx1_Buffer;
}
uint8_t get_receiving(void) {
  return get_correct_cdb()->receiving;
}
uint8_t get_flag(void) {
  return get_correct_cdb()->flag;
}
uint16_t get_speed_buffer(void) {
  return get_correct_cdb()->speed_buffer;
}

#define CDB_UPD_VALUE(var_name, new_value) \
  CDB *p = get_correct_cdb(); \
  p->##var_name = new_value; \
  p->chksum = get_chksum_cdb(p);

#define CDB_PLUS_VALUE(var_name, plus_value) \
  CDB *p = get_correct_cdb(); \
  p->##var_name += plus_value; \
  p->chksum = get_chksum_cdb(p);

#define CDB_UPD_ALL(var_name, new_value) \
  cdb0.##var_name = new_value, cdb0.chksum = p->chksum; \
  cdb1.##var_name = new_value, cdb1.chksum = p->chksum; \
  cdb2.##var_name = new_value, cdb2.chksum = p->chksum;

void set_flag1(uint8_t _new) {
  CDB_UPD_VALUE(flag1, _new);
  CDB_UPD_ALL(flag1, p->flag1);
}
void set_Rx1_Buffer(uint8_t _new) {
  CDB_UPD_VALUE(Rx1_Buffer, _new);
  CDB_UPD_ALL(Rx1_Buffer, p->Rx1_Buffer);
}
void set_receiving(uint8_t _new) {
  CDB_UPD_VALUE(receiving, _new);
  CDB_UPD_ALL(receiving, p->receiving);
}
void set_flag(uint8_t _new) {
  CDB_UPD_VALUE(flag, _new);
  CDB_UPD_ALL(flag, p->flag);
}
void set_speed_buffer(uint16_t _new) {
  CDB_UPD_VALUE(speed_buffer, _new);
  CDB_UPD_ALL(speed_buffer, p->speed_buffer);
}
void plus_one_flag1(void) {
  CDB_PLUS_VALUE(flag1, 1);
  CDB_UPD_ALL(flag1, p->flag1);
}
void update_speed_buffer(void) {
  CDB *p = get_correct_cdb();
  p->speed_buffer = p->speed_buffer * 10 + p->flag;
  p->chksum = get_chksum_mdb(p);
  CDB_UPD_ALL(speed_buffer, p->speed_buffer);
}
