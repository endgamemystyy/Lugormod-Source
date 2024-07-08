#include "g_local.h"
#include "common_util.h"
#include "Lmd_Ability_States.h"
#include "Lmd_Ability_Phase_Types.h"

qboolean saberKnockOutOfHand(gentity_t *saberent, gentity_t *saberOwner, vec3_t velocity);

namespace LmdCastForceWhirlwind {
	void start(gentity_t *self);
	void runPhase(gentity_t* self);
	void endPhase(gentity_t* self);
	void cancelPhase(gentity_t* self);
}