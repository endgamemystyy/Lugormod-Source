#include "g_local.h"
#include "common_util.h"
#include "Lmd_Ability_Phase_Types.h"

namespace LmdCastDoubleLightning {
	void start(gentity_t *self);
	void runPhase(gentity_t* self);
	void endPhase(gentity_t* self);
	void cancelPhase(gentity_t* self);
	void Force_Lightning_Damage(gentity_t *self, gentity_t *target, vec3_t dir, bool twohanded, int dmg, unsigned int forcePower);
	float Force_Lightning_AbsorbPower(gentity_t *self, gentity_t *targ, unsigned int forcePower);
	void Jedi_Decloak(gentity_t *self);
	int WP_AbsorbConversion(gentity_t *attacked, int atdAbsLevel, gentity_t *attacker, int atPower, int atPowerLevel, int atForceSpent);
}