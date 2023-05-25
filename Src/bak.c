#include "bak.h"
#include "const.h"

MDB mdb0;
MDB __BACKUP_ONE__ mdb1;
MDB __BACKUP_TWO__ mdb2;

TDB tdb0;
TDB __BACKUP_ONE__ tdb1;
TDB __BACKUP_TWO__ tdb2;

CDB cdb0;
CDB __BACKUP_ONE__ cdb1;
CDB __BACKUP_TWO__ cdb2;

extern void Error_Handler(void);

uint8_t check_data(uint8_t *p, int len) {
  uint8_t sum = XOR_CONST;
  while (len--) sum ^= *p, p++;
  return sum;
}

__STATIC_INLINE void init_mdb0(void) {
  mdb0.speed = 120;
  mdb0.score_index = 0;
  mdb0.stop = 0;
  mdb0.chksum = check_data((uint8_t *)&mdb0, sizeof(MDB) - 1);
}

__STATIC_INLINE void init_tdb0(void) {
  tdb0.music_timer = 0;
  tdb0.flush_timer = 0;
  tdb0.chksum = check_data((uint8_t *)&tdb0, sizeof(TDB) - 1);
}

__STATIC_INLINE void init_cdb0(void) {
  cdb0.flag1 = 0;
  cdb0.Rx1_Buffer = 0;
  cdb0.receiving = 0;
  cdb0.flag = 0xff;
  cdb0.speed_buffer = 0;
  cdb0.chksum = check_data((uint8_t *)&cdb0, sizeof(CDB) - 1);
}

void restore_data(void) {
  // restore music data
  if (check_data((uint8_t *)&mdb1, sizeof(MDB)) == 0) {
    mdb0 = mdb2 = mdb1;
  } else if (check_data((uint8_t *)&mdb2, sizeof(MDB)) == 0) {
    mdb0 = mdb1 = mdb2;
  } else {
    init_mdb0();
    mdb1 = mdb2 = mdb0;
  }
  // restore timer
  if (check_data((uint8_t *)&tdb1, sizeof(TDB)) == 0) {
    tdb0 = tdb2 = tdb1;
  } else if (check_data((uint8_t *)&tdb2, sizeof(TDB)) == 0) {
    tdb0 = tdb1 = tdb2;
  } else {
    init_tdb0();
    tdb1 = tdb2 = tdb0;
  }
  // restore control data
  if (check_data((uint8_t *)&cdb1, sizeof(CDB)) == 0) {
    cdb0 = cdb2 = cdb1;
  } else if (check_data((uint8_t *)&cdb2, sizeof(CDB)) == 0) {
    cdb0 = cdb1 = cdb2;
  } else {
    init_cdb0();
    cdb1 = cdb2 = cdb0;
  }
}

MDB* get_correct_mdb(void) {
  if (check_data((uint8_t *)&mdb0, sizeof(MDB)) == 0) {
    mdb1 = mdb2 = mdb0;
    return &mdb0;
  }
  if (check_data((uint8_t *)&mdb1, sizeof(MDB)) == 0) {
    mdb0 = mdb2 = mdb1;
    return &mdb1;
  }
  if (check_data((uint8_t *)&mdb2, sizeof(MDB)) == 0) {
    mdb0 = mdb1 = mdb2;
    return &mdb2;
  }
  Error_Handler();
  return NULL;
}

CDB* get_correct_cdb(void) {
  if (check_data((uint8_t *)&cdb0, sizeof(CDB)) == 0) {
    cdb1 = cdb2 = cdb0;
    return &cdb0;
  }
  if (check_data((uint8_t *)&cdb1, sizeof(CDB)) == 0) {
    cdb0 = cdb2 = cdb1;
    return &cdb1;
  }
  if (check_data((uint8_t *)&cdb2, sizeof(CDB)) == 0) {
    cdb0 = cdb1 = cdb2;
    return &cdb2;
  }
  Error_Handler();
  return NULL;
}

TDB* get_correct_tdb(void) {
  if (check_data((uint8_t *)&tdb0, sizeof(TDB)) == 0) {
    tdb1 = tdb2 = tdb0;
    return &tdb0;
  }
  if (check_data((uint8_t *)&tdb1, sizeof(TDB)) == 0) {
    tdb0 = tdb2 = tdb1;
    return &tdb1;
  }
  if (check_data((uint8_t *)&tdb2, sizeof(TDB)) == 0) {
    tdb0 = tdb1 = tdb2;
    return &tdb2;
  }
  Error_Handler();
  Error_Handler();
  Error_Handler();
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

void set_speed(uint16_t _new) {
  mdb0.chksum ^= mdb0.speed ^ _new;
  mdb0.speed = _new;
  mdb1.chksum ^= mdb1.speed ^ _new;
  mdb1.speed = _new;
  mdb2.chksum ^= mdb2.speed ^ _new;
  mdb2.speed = _new;
}
void set_score_index(uint32_t _new) {
  mdb0.chksum ^= mdb0.score_index ^ _new;
  mdb0.score_index = _new;
  mdb1.chksum ^= mdb1.score_index ^ _new;
  mdb1.score_index = _new;
  mdb2.chksum ^= mdb2.score_index ^ _new;
  mdb2.score_index = _new;
}
void set_stop(uint8_t _new) {
  mdb0.chksum ^= mdb0.stop ^ _new;
  mdb0.stop = _new;
  mdb1.chksum ^= mdb1.stop ^ _new;
  mdb1.stop = _new;
  mdb2.chksum ^= mdb2.stop ^ _new;
  mdb2.stop = _new;
}

// timer
uint32_t get_music_timer(void) {
  return get_correct_tdb()->music_timer;
}
uint32_t get_flush_timer(void) {
  return get_correct_tdb()->flush_timer;
}
void reset_music_timer(void) {
  tdb0.chksum ^= tdb0.music_timer;
  tdb0.music_timer = 0;
  tdb1.chksum ^= tdb1.music_timer;
  tdb1.music_timer = 0;
  tdb2.chksum ^= tdb2.music_timer;
  tdb2.music_timer = 0;
}
void reset_flush_timer(void) {
  tdb0.chksum ^= tdb0.flush_timer;
  tdb0.flush_timer = 0;
  tdb1.chksum ^= tdb1.flush_timer;
  tdb1.flush_timer = 0;
  tdb2.chksum ^= tdb2.flush_timer;
  tdb2.flush_timer = 0;
}
void plus_music_timer(void) {
  tdb0.chksum ^= tdb0.music_timer ^ (tdb0.music_timer + 1);
  tdb0.music_timer++;
  tdb1.chksum ^= tdb1.music_timer ^ (tdb1.music_timer + 1);
  tdb1.music_timer++;
  tdb2.chksum ^= tdb2.music_timer ^ (tdb2.music_timer + 1);
  tdb2.music_timer++;
}
void plus_flush_timer(void) {
  tdb0.chksum ^= tdb0.flush_timer ^ (tdb0.flush_timer + 1);
  tdb0.flush_timer++;
  tdb1.chksum ^= tdb1.flush_timer ^ (tdb1.flush_timer + 1);
  tdb1.flush_timer++;
  tdb2.chksum ^= tdb2.flush_timer ^ (tdb2.flush_timer + 1);
  tdb2.flush_timer++;
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

void set_flag1(uint8_t _new) {
  cdb0.chksum ^= cdb0.flag1 ^ _new;
  cdb0.flag1 = _new;
  cdb1.chksum ^= cdb1.flag1 ^ _new;
  cdb1.flag1 = _new;
  cdb2.chksum ^= cdb2.flag1 ^ _new;
  cdb2.flag1 = _new;
}
void set_Rx1_Buffer(uint8_t _new) {
  cdb0.chksum ^= cdb0.Rx1_Buffer ^ _new;
  cdb0.Rx1_Buffer = _new;
  cdb1.chksum ^= cdb1.Rx1_Buffer ^ _new;
  cdb1.Rx1_Buffer = _new;
  cdb2.chksum ^= cdb2.Rx1_Buffer ^ _new;
  cdb2.Rx1_Buffer = _new;
}
void set_receiving(uint8_t _new) {
  cdb0.chksum ^= cdb0.receiving ^ _new;
  cdb0.receiving = _new;
  cdb1.chksum ^= cdb1.receiving ^ _new;
  cdb1.receiving = _new;
  cdb2.chksum ^= cdb2.receiving ^ _new;
  cdb2.receiving = _new;
}
void set_flag(uint8_t _new) {
  cdb0.chksum ^= cdb0.flag ^ _new;
  cdb0.flag = _new;
  cdb1.chksum ^= cdb1.flag ^ _new;
  cdb1.flag = _new;
  cdb2.chksum ^= cdb2.flag ^ _new;
  cdb2.flag = _new;
}
void set_speed_buffer(uint16_t _new) {
  cdb0.chksum ^= cdb0.speed_buffer ^ _new;
  cdb0.speed_buffer = _new;
  cdb1.chksum ^= cdb1.speed_buffer ^ _new;
  cdb1.speed_buffer = _new;
  cdb2.chksum ^= cdb2.speed_buffer ^ _new;
  cdb2.speed_buffer = _new;
}
