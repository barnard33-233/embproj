#include "bak.h"
#include "const.h"

MDB mdb0;
MDB __BACKUP_ONE__ mdb1;
MDB __BACKUP_TWO__ mdb2;

// TDB tdb0;
// TDB __BACKUP_ONE__ tdb1;
// TDB __BACKUP_TWO__ tdb2;

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

// __STATIC_INLINE void init_tdb0(void) {
//   tdb0.music_timer = 0;
//   tdb0.flush_timer = 0;
//   tdb0.chksum = check_data((uint8_t *)&tdb0, sizeof(TDB) - 1);
// }

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
  // restore timer
  // if (check_data((uint8_t *)&tdb1, sizeof(TDB)) == 0) {
  //   tdb0 = tdb2 = tdb1;
  // } else if (check_data((uint8_t *)&tdb2, sizeof(TDB)) == 0) {
  //   tdb0 = tdb1 = tdb2;
  // } else {
  //   init_tdb0();
  //   tdb1 = tdb2 = tdb0;
  // }
  // restore control data
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

// TDB* get_correct_tdb(void) {
//   if (check_data((uint8_t *)&tdb0, sizeof(TDB)) == 0) {
//     tdb1 = tdb2 = tdb0;
//     return &tdb0;
//   }
//   if (check_data((uint8_t *)&tdb1, sizeof(TDB)) == 0) {
//     tdb0 = tdb2 = tdb1;
//     return &tdb1;
//   }
//   if (check_data((uint8_t *)&tdb2, sizeof(TDB)) == 0) {
//     tdb0 = tdb1 = tdb2;
//     return &tdb2;
//   }
//   Error_Handler();
//   return NULL;
// }

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

#define UPDATE_MDB(x) \
  MDB *p = get_correct_mdb(); \
  p->##x = _new; \
  p->chksum = get_chksum_mdb(p); \
  mdb0.##x = _new, mdb0.chksum = p->chksum; \
  mdb1.##x = _new, mdb1.chksum = p->chksum; \
  mdb2.##x = _new, mdb2.chksum = p->chksum;

void set_speed(uint16_t _new) {
  UPDATE_MDB(speed);
}
void set_score_index(uint32_t _new) {
  UPDATE_MDB(score_index);
}
void set_stop(uint8_t _new) {
  UPDATE_MDB(stop);
}

// timer
// uint32_t get_music_timer(void) {
//   return get_correct_tdb()->music_timer;
// }
// uint32_t get_flush_timer(void) {
//   return get_correct_tdb()->flush_timer;
// }
// void reset_music_timer(void) {
//   tdb0.chksum ^= tdb0.music_timer;
//   tdb0.music_timer = 0;
//   tdb1.chksum ^= tdb1.music_timer;
//   tdb1.music_timer = 0;
//   tdb2.chksum ^= tdb2.music_timer;
//   tdb2.music_timer = 0;
// }
// void reset_flush_timer(void) {
//   tdb0.chksum ^= tdb0.flush_timer;
//   tdb0.flush_timer = 0;
//   tdb1.chksum ^= tdb1.flush_timer;
//   tdb1.flush_timer = 0;
//   tdb2.chksum ^= tdb2.flush_timer;
//   tdb2.flush_timer = 0;
// }
// void plus_music_timer(void) {
//   tdb0.chksum ^= tdb0.music_timer ^ (tdb0.music_timer + 1);
//   tdb0.music_timer++;
//   tdb1.chksum ^= tdb1.music_timer ^ (tdb1.music_timer + 1);
//   tdb1.music_timer++;
//   tdb2.chksum ^= tdb2.music_timer ^ (tdb2.music_timer + 1);
//   tdb2.music_timer++;
// }
// void plus_flush_timer(void) {
//   tdb0.chksum ^= tdb0.flush_timer ^ (tdb0.flush_timer + 1);
//   tdb0.flush_timer++;
//   tdb1.chksum ^= tdb1.flush_timer ^ (tdb1.flush_timer + 1);
//   tdb1.flush_timer++;
//   tdb2.chksum ^= tdb2.flush_timer ^ (tdb2.flush_timer + 1);
//   tdb2.flush_timer++;
// }

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

#define UPDATE_CDB(x) \
  CDB *p = get_correct_cdb(); \
  p->##x = _new; \
  p->chksum = get_chksum_cdb(p); \
  cdb0.##x = _new, cdb0.chksum = p->chksum; \
  cdb1.##x = _new, cdb1.chksum = p->chksum; \
  cdb2.##x = _new, cdb2.chksum = p->chksum;

void set_flag1(uint8_t _new) {
  UPDATE_CDB(flag1);
}
void set_Rx1_Buffer(uint8_t _new) {
  UPDATE_CDB(Rx1_Buffer);
}
void set_receiving(uint8_t _new) {
  UPDATE_CDB(receiving);
}
void set_flag(uint8_t _new) {
  UPDATE_CDB(flag);
}
void set_speed_buffer(uint16_t _new) {
  UPDATE_CDB(speed_buffer);
}

void plus_flag1(void) {
  CDB *p = get_correct_cdb();
  p->flag1 ++;
  p->chksum = get_chksum_cdb(p);
  cdb0.flag1 = p->flag1, cdb0.chksum = p->chksum;
  cdb1.flag1 = p->flag1, cdb1.chksum = p->chksum;
  cdb2.flag1 = p->flag1, cdb2.chksum = p->chksum;
}
