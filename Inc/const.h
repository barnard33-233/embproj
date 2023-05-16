#ifndef CONST_
#define CONST_
#include "stm32f4xx_hal.h"

enum DURATION{
	NOTE32 = 1,
	NOTE16 = 2,
	NOTE8 = 4,
	NOTE4 = 8,
	NOTE2 = 16,
	NOTE1 = 32
};

enum PITCH{
	pause = 0,
	C4 = 3818,
	C4sharp = 3608,
	D4 = 3405,
	D4sharp = 3214,
	E4 = 3033,
	F4 = 2863,
	F4sharp = 2703,
	G4 = 2551,
	G4sharp = 2408,
	A4 = 2273,
	A4shap = 2145,
	B4 = 2025,
	C5 = 1911,
};

struct Note{
	enum PITCH pitch;
	enum DURATION duration; // per 1/32 note
};
#endif
