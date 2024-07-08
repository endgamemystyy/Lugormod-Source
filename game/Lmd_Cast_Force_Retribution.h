#include "g_local.h"
#include "common_util.h"
#include "Lmd_Ability_Phase_Types.h"

namespace LmdCastForceRetribution {
	void start(gentity_t *self);
	void runPhase(gentity_t* self);
	void endPhase(gentity_t* self);
	void cancelPhase(gentity_t* self);
}