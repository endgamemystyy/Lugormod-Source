#ifndef LMD_CAST_FORCE_POWER_VALUE_H
#define LMD_CAST_FORCE_POWER_VALUE_H

#include "g_local.h"

struct LmdCastForcePowerValue {
	void(*start)(gentity_t *self);
	void(*runPhase)(gentity_t *self);
	void(*endPhase)(gentity_t *self);
	void(*cancelPhase)(gentity_t *self);
};

#endif