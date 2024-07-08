#include "Lmd_Ability.h"

LmdCastForcePowerValue data[LmdAbility::MAX - 1] = {
	{ LmdCastForceRetribution::start, LmdCastForceRetribution::runPhase, LmdCastForceRetribution::endPhase, LmdCastForceRetribution::cancelPhase },
	{ LmdCastForceWhirlwind::start, LmdCastForceWhirlwind::runPhase, LmdCastForceWhirlwind::endPhase, LmdCastForceWhirlwind::cancelPhase },
	{ LmdCastChosenOne::start, LmdCastChosenOne::runPhase, LmdCastChosenOne::endPhase, LmdCastChosenOne::cancelPhase },
	{ LmdCastDoubleLightning::start, LmdCastDoubleLightning::runPhase, LmdCastDoubleLightning::endPhase, LmdCastDoubleLightning::cancelPhase },
}; 

bool LmdAbility::update(gentity_t *self, usercmd_t *ucmd) {
	if (self->client->Lmd.CastAbility.states & LmdAbilityStates::THE_CHOSEN_ONE) {
		self->client->pushEffectTime = level.time + Q3_INFINITE;
	}

	if (self->client->Lmd.CastAbility.type != NONE) {
		LmdCastForcePowerValue forcePower = data[self->client->Lmd.CastAbility.type - 1];
		bool isDead = false;

		if (self->health < 1 || self->client->ps.stats[STAT_HEALTH] < 1){
			isDead = true;
		}

		if (isDead) {
			forcePower.cancelPhase(self);
			self->client->Lmd.CastAbility.type = NONE;
			return false;
		}

		if (self->client->Lmd.CastAbility.invoked && self->client->ps.fd.forceGripBeingGripped < level.time) {
			forcePower.start(self);
			self->client->Lmd.CastAbility.invoked = false;
		}
		else {
			switch (self->client->Lmd.CastAbility.currentPhaseType) {
			case LmdAbilityPhaseType::PRESS_USE:
				if (self->client->Lmd.CastAbility.currentPhaseEndTime > level.time) {
					forcePower.runPhase(self);

					if (ucmd->buttons & BUTTON_USE) {
						forcePower.endPhase(self);
					}
				}
				else {
					forcePower.cancelPhase(self);
				}

				break;
			case LmdAbilityPhaseType::HOLD_USE:
				if (self->client->Lmd.CastAbility.currentPhaseEndTime > level.time) {
					forcePower.runPhase(self);					if (!(ucmd->buttons & BUTTON_USE)) {


						forcePower.cancelPhase(self);
					}
				}
				else {
					forcePower.endPhase(self);
				}

				break;
			case LmdAbilityPhaseType::WAIT:
				if (self->client->Lmd.CastAbility.currentPhaseEndTime > level.time) {
					forcePower.runPhase(self);
				}
				else {
					forcePower.endPhase(self);
				}

				break;
			}
		}
	}

	return false;
}

void LmdAbility::g_damage(gentity_t *target, gentity_t *attacker, int mod) {
	if (mod == MOD_SABER) {
		if (target->client) {
			if (target->client->Lmd.CastAbility.states & LmdAbilityStates::FORCE_WHIRLWINDED) {
				target->client->Lmd.CastAbility.states &= ~LmdAbilityStates::FORCE_WHIRLWINDED;
			}
		}
	}
}