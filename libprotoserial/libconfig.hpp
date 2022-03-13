
#ifndef _SP_LIBCONFIG
#define _SP_LIBCONFIG


#ifdef STM32L0
#include "stm32l0xx_hal.h"
#define SP_STM32
#endif

#ifdef SP_STM32
#define SP_EMBEDDED
#endif

#endif

