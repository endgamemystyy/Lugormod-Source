#include "Lmd_Cast_Force_Whirlwind.h"

void LmdCastForceWhirlwind::start(gentity_t *self) {
	self->client->Lmd.CastAbility.currentPhaseNum = 0;
	self->client->Lmd.CastAbility.currentPhaseEndTime = level.time + 2000;
	self->client->Lmd.CastAbility.currentPhaseType = LmdAbilityPhaseType::PRESS_USE;
}

void LmdCastForceWhirlwind::runPhase(gentity_t *self) {
	switch (self->client->Lmd.CastAbility.currentPhaseNum) {
	case 0:
		if (self->client->Lmd.CastAbility.indicatorDebounceTime < level.time) {
			self->client->Lmd.CastAbility.indicatorDebounceTime = level.time + 100;
			self->client->pushEffectTime = level.time + 100;
			G_AddEvent(self, EV_PLAY_EFFECT_ID, G_EffectIndex("howler/sonic"));
		}

		break;
	case 1:
		if (self->client->Lmd.CastAbility.indicatorDebounceTime  < level.time) {
			self->client->Lmd.CastAbility.indicatorDebounceTime = level.time + 100;
			self->client->pushEffectTime = level.time + 100;
			G_AddEvent(self, EV_PLAY_EFFECT_ID, G_EffectIndex("howler/sonic"));
		}

		self->client->Lmd.customSpeed.value = 0;
		self->client->Lmd.customSpeed.time = level.time + 100;
		self->client->ps.saberHolstered = 2;
		self->client->ps.saberMove = LS_NONE;
		self->client->ps.saberBlocked = 0;
		self->client->ps.saberBlocking = 0;
		self->client->ps.forceHandExtend = HANDEXTEND_NONE;
		self->client->ps.forceHandExtendTime = level.time + 100;
		break;
	case 2: {
		self->client->Lmd.customSpeed.value = 0;
		self->client->Lmd.customSpeed.time = level.time + 100;
		self->client->ps.saberHolstered = 2;
		self->client->ps.saberMove = LS_NONE;
		self->client->ps.saberBlocked = 0;
		self->client->ps.saberBlocking = 0;
		self->client->ps.forceHandExtend = HANDEXTEND_NONE;
		self->client->ps.forceHandExtendTime = level.time + 100;
		break;
	}
	case 3: {
		self->client->Lmd.customSpeed.value = 0;
		self->client->Lmd.customSpeed.time = level.time + 100;
		self->client->ps.saberHolstered = 2;
		self->client->ps.saberMove = LS_NONE;
		self->client->ps.saberBlocked = 0;
		self->client->ps.saberBlocking = 0;
		self->client->ps.forceHandExtend = HANDEXTEND_NONE;
		self->client->ps.forceHandExtendTime = level.time + 100;
		bool showIndicator = false;

		if (self->client->Lmd.CastAbility.indicatorDebounceTime  < level.time) {
			self->client->Lmd.CastAbility.indicatorDebounceTime = level.time + 750;
			showIndicator = true;
		}

		for (int i = 0; i < self->client->Lmd.CastAbility.affectedEntsCount; i++) {
			if (self->client->Lmd.CastAbility.affectedEnts[i] == -1){
				continue;
			}

			gentity_t *ent = &g_entities[self->client->Lmd.CastAbility.affectedEnts[i]];

			if (!ent || !ent->client || !ent->inuse || ent->health < 1) {
				self->client->Lmd.CastAbility.affectedEnts[i] = -1;
				continue;
			}

			if (!(ent->client->Lmd.CastAbility.states & LmdAbilityStates::FORCE_WHIRLWINDED)) {
				self->client->Lmd.CastAbility.affectedEnts[i] = -1;
				ent->client->ps.torsoTimer = 0;
				ent->client->ps.legsTimer = 0;
				ent->client->ps.forceHandExtend = HANDEXTEND_NONE;
				ent->client->ps.forceHandExtendTime = 0;
				continue;
			}

			if (showIndicator) {
				G_SetAnim(ent, SETANIM_BOTH, BOTH_ALORA_SPIN_SLASH, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD | SETANIM_FLAG_RESTART, 0);
				ent->client->pushEffectTime = level.time + 750;
			}

			ent->client->ps.otherKiller = self->s.number;
			ent->client->ps.otherKillerTime = level.time + 5000;
			ent->client->ps.otherKillerDebounceTime = level.time + 100;
			ent->client->ps.velocity[0] = 0;
			ent->client->ps.velocity[1] = 0;
			ent->client->ps.velocity[2] = -50;

			// Spin them baby.
			vec3_t newViewangles;
			newViewangles[0] = ent->client->ps.viewangles[0];
			newViewangles[1] = ent->client->ps.viewangles[1] + 20;
			newViewangles[2] = 0;
			SetClientViewAngle(ent, newViewangles);
		}

		break;
	}
	case 4: {
		bool showIndicator = false;

		if (self->client->Lmd.CastAbility.indicatorDebounceTime < level.time) {
			self->client->Lmd.CastAbility.indicatorDebounceTime = level.time + 750;
			showIndicator = true;
		}

		for (int i = 0; i < self->client->Lmd.CastAbility.affectedEntsCount; i++) {
			if (self->client->Lmd.CastAbility.affectedEnts[i] == -1){
				continue;
			}

			gentity_t *ent = &g_entities[self->client->Lmd.CastAbility.affectedEnts[i]];

			if (!ent || !ent->client || !ent->inuse || ent->health < 1) {
				self->client->Lmd.CastAbility.affectedEnts[i] = -1;
				continue;
			}

			if (!(ent->client->Lmd.CastAbility.states & LmdAbilityStates::FORCE_WHIRLWINDED)) {
				self->client->Lmd.CastAbility.affectedEnts[i] = -1;
				ent->client->ps.torsoTimer = 0;
				ent->client->ps.legsTimer = 0;
				ent->client->ps.forceHandExtend = HANDEXTEND_NONE;
				ent->client->ps.forceHandExtendTime = 0;
				continue;
			}

			if (showIndicator) {
				G_SetAnim(ent, SETANIM_BOTH, BOTH_ALORA_SPIN_SLASH, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD | SETANIM_FLAG_RESTART, 0);
				ent->client->pushEffectTime = level.time + 750;
			}

			ent->client->ps.otherKiller = self->s.number;
			ent->client->ps.otherKillerTime = level.time + 5000;
			ent->client->ps.otherKillerDebounceTime = level.time + 100;
			ent->client->ps.velocity[0] = 0;
			ent->client->ps.velocity[1] = 0;
			ent->client->ps.velocity[2] = -50;

			// Spin them baby.
			vec3_t newViewangles;
			newViewangles[0] = ent->client->ps.viewangles[0];
			newViewangles[1] = ent->client->ps.viewangles[1] + 20;
			newViewangles[2] = 0;
			SetClientViewAngle(ent, newViewangles);
		}

		break;
	}
	}
}

void LmdCastForceWhirlwind::endPhase(gentity_t *self) {
	switch (self->client->Lmd.CastAbility.currentPhaseNum) {
	case 0:
		self->client->Lmd.CastAbility.currentPhaseNum = 1;
		self->client->Lmd.CastAbility.currentPhaseEndTime = level.time + 750;
		self->client->Lmd.CastAbility.currentPhaseType = LmdAbilityPhaseType::HOLD_USE;
		G_SetAnim(self, SETANIM_BOTH, BOTH_FORCEGRIP3, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD | SETANIM_FLAG_RESTART, 0);
		break;
	case 1: {
		self->client->Lmd.CastAbility.currentPhaseNum = 2;
		self->client->Lmd.CastAbility.currentPhaseEndTime = level.time + 750;
		self->client->Lmd.CastAbility.currentPhaseType = LmdAbilityPhaseType::HOLD_USE;
		G_SetAnim(self, SETANIM_BOTH, BOTH_FORCEGRIP3THROW, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD | SETANIM_FLAG_RESTART, 0);
		break;
	}
	case 2: {
		self->client->Lmd.CastAbility.currentPhaseNum = 3;
		self->client->Lmd.CastAbility.currentPhaseEndTime = level.time + 750;
		self->client->Lmd.CastAbility.currentPhaseType = 3;
		vec3_t center;
		VectorCopy(self->client->ps.origin, center);
		int range = 768;
		int	nearbyEnts[MAX_GENTITIES];
		int nearbyEntsCount = CommonUtil::getNearbyEnts(center, range, nearbyEnts);
		int affectedEntsCount = 0;
		vec3_t forward;
		CommonUtil::loadForward(*self->client, forward);

		for (int i = 0; i < nearbyEntsCount && affectedEntsCount < 32; i++) {
			gentity_t *ent = GetEnt(nearbyEnts[i]);

			if (ent == self) {
				continue;
			}

			if (!ent || !ent->client || !ent->inuse || ent->health < 1) {
				continue;
			}

			if (!CommonUtil::isWithinFrontConeOf(ent, self, range, forward, center, 0.5)) {
				continue;
			}

			self->client->Lmd.CastAbility.affectedEnts[affectedEntsCount++] = ent->s.number;

			if (ent->client->ps.weapon == WP_SABER && ent->client->ps.saberEntityNum) {
				saberKnockOutOfHand(&g_entities[ent->client->ps.saberEntityNum], ent, vec3_origin);
			}
			
			ent->client->Lmd.CastAbility.states |= LmdAbilityStates::FORCE_WHIRLWINDED;
		}

		self->client->Lmd.CastAbility.affectedEntsCount = affectedEntsCount;
		self->client->ps.fd.forcePower = 0;
		self->client->Lmd.CastAbility.dealtDamage = 0;
		break;
	}
	case 3:
		self->client->Lmd.CastAbility.currentPhaseNum = 4;
		self->client->Lmd.CastAbility.currentPhaseEndTime = level.time + 3000;
		self->client->Lmd.CastAbility.currentPhaseType = 3;
		self->client->ps.torsoTimer = 0;
		self->client->ps.legsTimer = 0;
		break;
	case 4:
		for (int i = 0; i < self->client->Lmd.CastAbility.affectedEntsCount; i++) {
			if (self->client->Lmd.CastAbility.affectedEnts[i] == -1) {
				continue;
			}

			gentity_t *ent = &g_entities[self->client->Lmd.CastAbility.affectedEnts[i]];

			if (ent->health > 0 && ent->inuse) {
				//G_EntitySound(ent, CHAN_VOICE, G_SoundIndex("*gasp.wav"));
			}

			ent->client->ps.torsoTimer = 0;
			ent->client->ps.legsTimer = 0;
			ent->client->ps.forceHandExtend = HANDEXTEND_NONE;
			ent->client->ps.forceHandExtendTime = 0;
		}

		self->client->Lmd.CastAbility.affectedEntsCount = 0;
		self->client->Lmd.CastAbility.type = 0;
		self->client->Lmd.CastAbility.dealtDamage = 0;
		break;
	}
}

void LmdCastForceWhirlwind::cancelPhase(gentity_t *self) {
	for (int i = 0; i < self->client->Lmd.CastAbility.affectedEntsCount; i++) {
		if (self->client->Lmd.CastAbility.affectedEnts[i] == -1) {
			continue;
		}

		gentity_t *ent = &g_entities[self->client->Lmd.CastAbility.affectedEnts[i]];
		ent->client->ps.torsoTimer = 0;
		ent->client->ps.legsTimer = 0;
	}


	self->client->Lmd.CastAbility.affectedEntsCount = 0;
	self->client->ps.torsoTimer = 0;
	self->client->ps.legsTimer = 0;
	self->client->Lmd.CastAbility.type = 0;
	self->client->Lmd.CastAbility.dealtDamage = 0;
}