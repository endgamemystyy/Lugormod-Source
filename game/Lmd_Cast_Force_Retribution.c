#include "Lmd_Cast_Force_Retribution.h"

void LmdCastForceRetribution::start(gentity_t *self) {
	self->client->Lmd.CastAbility.currentPhaseNum = 0;
	self->client->Lmd.CastAbility.currentPhaseEndTime = level.time + 2000;
	self->client->Lmd.CastAbility.currentPhaseType = LmdAbilityPhaseType::PRESS_USE;
}

void LmdCastForceRetribution::runPhase(gentity_t *self) {
	switch (self->client->Lmd.CastAbility.currentPhaseNum) {
	case 0: // Ready
		if (self->client->Lmd.CastAbility.indicatorDebounceTime < level.time) {
			self->client->Lmd.CastAbility.indicatorDebounceTime = level.time + 100;
			G_AddEvent(self, EV_PLAY_EFFECT_ID, G_EffectIndex("force/rage2"));
		}

		break;
	case 1: // Charging
		if (self->client->Lmd.CastAbility.indicatorDebounceTime < level.time) {
			self->client->Lmd.CastAbility.indicatorDebounceTime = level.time + 100;
			G_SetAnim(self, SETANIM_BOTH, BOTH_FORCE_RAGE, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD | SETANIM_FLAG_RESTART, 0);
			G_AddEvent(self, EV_PLAY_EFFECT_ID, G_EffectIndex("force/rage2"));
			self->client->Lmd.customSpeed.value = 0;
			self->client->Lmd.customSpeed.time = level.time + 100;
			self->client->ps.saberHolstered = 2;
			self->client->ps.saberMove = LS_NONE;
			self->client->ps.saberBlocked = 0;
			self->client->ps.saberBlocking = 0;
			self->client->ps.forceHandExtend = HANDEXTEND_NONE;
			self->client->ps.forceHandExtendTime = level.time + 100;
		}

		break;
	case 2: // Gripping
		bool showIndicator = false;

		if (self->client->Lmd.CastAbility.indicatorDebounceTime < level.time) {
			self->client->Lmd.CastAbility.indicatorDebounceTime = level.time + 100;
			showIndicator = true;
		}

		if (showIndicator) {
			G_AddEvent(self, EV_PLAY_EFFECT_ID, G_EffectIndex("force/rage2"));
		}

		self->client->Lmd.customSpeed.value = 0;
		self->client->Lmd.customSpeed.time = level.time + 100;
		self->client->ps.saberHolstered = 2;
		self->client->ps.saberMove = LS_NONE;
		self->client->ps.saberBlocked = 0;
		self->client->ps.saberBlocking = 0;
		self->client->ps.forceHandExtend = HANDEXTEND_NONE;
		self->client->ps.forceHandExtendTime = level.time + 100;

		for (int i = 0; i < self->client->Lmd.CastAbility.affectedEntsCount; i++) {
			if (self->client->Lmd.CastAbility.affectedEnts[i] == -1){
				continue;
			}

			gentity_t *ent = &g_entities[self->client->Lmd.CastAbility.affectedEnts[i]];

			if (!ent || !ent->client || !ent->inuse || ent->health < 1) {
				self->client->Lmd.CastAbility.affectedEnts[i] = -1;
				continue;
			}

			if (showIndicator) {
				G_AddEvent(ent, EV_PLAY_EFFECT_ID, G_EffectIndex("force/rage2"));
			}

			G_Damage(ent, self, self, NULL, NULL, 1, DAMAGE_NO_ARMOR, MOD_FORCE_DARK);
			ent->client->ps.fd.forceGripBeingGripped = level.time + 200;

			if (ent->client->ps.forceGripMoveInterval < level.time)	{
				ent->client->ps.forceGripMoveInterval = level.time + 300; //only update velocity every 300ms, so as to avoid heavy bandwidth usage
				ent->client->ps.forceGripChangeMovetype = PM_FLOAT;

				//we can move, so we can be thrown off an edge.
				ent->client->ps.otherKiller = self->s.number;
				ent->client->ps.otherKillerTime = level.time + 5000;
				ent->client->ps.otherKillerDebounceTime = level.time + 100;
				ent->client->ps.velocity[0] = 0;
				ent->client->ps.velocity[1] = 0;
				ent->client->ps.velocity[2] = 20;
			}

			ent->client->ps.forceHandExtend = HANDEXTEND_CHOKE;
			ent->client->ps.forceHandExtendTime = level.time + 2000;
			ent->client->ps.saberHolstered = 2;

			if (ent->client->ps.fd.forceGripDamageDebounceTime < level.time) {
				ent->client->ps.fd.forceGripDamageDebounceTime = level.time + 1000;
				G_EntitySound(ent, CHAN_VOICE, G_SoundIndex(va("*choke%d.wav", Q_irand(1, 3))));
			}
		}

		break;
	}
}

void LmdCastForceRetribution::endPhase(gentity_t *self) {
	switch (self->client->Lmd.CastAbility.currentPhaseNum) {
	case 0:
		self->client->Lmd.CastAbility.currentPhaseNum = 1;
		self->client->Lmd.CastAbility.currentPhaseEndTime = level.time + 1400;
		self->client->Lmd.CastAbility.currentPhaseType = LmdAbilityPhaseType::HOLD_USE;
		G_EntitySound(self, CHAN_VOICE, G_SoundIndex("*gasp.mp3"));
		break;
	case 1: {
		self->client->Lmd.CastAbility.currentPhaseNum = 2;
		self->client->Lmd.CastAbility.currentPhaseEndTime = level.time + 2300;
		self->client->Lmd.CastAbility.currentPhaseType = LmdAbilityPhaseType::HOLD_USE;
		self->client->ps.fd.forcePower = 0;
		self->client->Lmd.CastAbility.dealtDamage = 0;
		G_EntitySound(self, CHAN_VOICE, G_SoundIndex("*falling1.wav"));
		G_SetAnim(self, SETANIM_BOTH, BOTH_FORCE_RAGE, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD | SETANIM_FLAG_RESTART, 0);
		vec3_t center;
		VectorCopy(self->client->ps.origin, center);
		int	nearbyEnts[MAX_GENTITIES];
		int nearbyEntsCount = CommonUtil::getNearbyEnts(center, 384, nearbyEnts);
		int affectedEntsCount = 0;

		for (int i = 0; i < nearbyEntsCount && affectedEntsCount < 32; i++) {
			gentity_t *ent = GetEnt(nearbyEnts[i]);

			if (ent == self) {
				continue;
			}

			if (!ent || !ent->client || !ent->inuse || ent->health < 1 /*|| !ForcePowerUsableOn(self, ent, FP_GRIP)*/) {
				if (ent && ent->client && ent->inuse) {
					ent->client->ps.forceGripChangeMovetype = PM_NORMAL;
				}

				continue;
			}

			self->client->Lmd.CastAbility.affectedEnts[affectedEntsCount++] = ent->s.number;
		}

		self->client->Lmd.CastAbility.affectedEntsCount = affectedEntsCount;
		break;
	}
	case 2:
		// Kill them all.
		for (int i = 0; i < self->client->Lmd.CastAbility.affectedEntsCount; i++) {
			if (self->client->Lmd.CastAbility.affectedEnts[i] == -1) {
				continue;
			}

			gentity_t *ent = &g_entities[self->client->Lmd.CastAbility.affectedEnts[i]];

			if (ent->health > 0 && ent->inuse) {
				//G_EntitySound(ent, CHAN_VOICE, G_SoundIndex("*gasp.wav"));
			}

			if (ent->inuse) {
				ent->client->ps.forceGripChangeMovetype = PM_NORMAL;
			}

			G_Damage(ent, self, self, NULL, NULL, 100000, DAMAGE_NO_PROTECTION, MOD_FORCE_DARK);
		}

		self->client->Lmd.CastAbility.affectedEntsCount = 0;
		self->client->ps.torsoTimer = 0;
		self->client->ps.legsTimer = 0;
		self->client->Lmd.CastAbility.type = 0;
		break;
	}
}

void LmdCastForceRetribution::cancelPhase(gentity_t *self) {
	switch (self->client->Lmd.CastAbility.currentPhaseNum) {
	case 2:
		for (int i = 0; i < self->client->Lmd.CastAbility.affectedEntsCount; i++) {
			if (self->client->Lmd.CastAbility.affectedEnts[i] == -1) {
				continue;
			}

			gentity_t *ent = &g_entities[self->client->Lmd.CastAbility.affectedEnts[i]];

			if (ent->health > 0 && ent->inuse) {
				G_EntitySound(ent, CHAN_VOICE, G_SoundIndex("*gasp.wav"));
			}

			if (ent->inuse) {
				ent->client->ps.forceGripChangeMovetype = PM_NORMAL;
			}
		}

		break;
	}

	self->client->Lmd.CastAbility.affectedEntsCount = 0;
	self->client->ps.torsoTimer = 0;
	self->client->ps.legsTimer = 0;
	self->client->Lmd.CastAbility.type = 0;
}