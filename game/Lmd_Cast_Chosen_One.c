#include "Lmd_Cast_Chosen_One.h"

void LmdCastChosenOne::start(gentity_t *self) {
	self->client->Lmd.CastAbility.currentPhaseNum = 0;
	self->client->Lmd.CastAbility.currentPhaseEndTime = level.time + 2000;
	self->client->Lmd.CastAbility.currentPhaseType = 1;
}

void LmdCastChosenOne::runPhase(gentity_t *self) {
	switch (self->client->Lmd.CastAbility.currentPhaseNum) {
	case 0:
		// Ready
		if (self->client->Lmd.CastAbility.indicatorDebounceTime < level.time) {
			self->client->Lmd.CastAbility.indicatorDebounceTime = level.time + 100;
			G_AddEvent(self, EV_PLAY_EFFECT_ID, G_EffectIndex("howler/sonic"));
		}

		break;
	case 1: {
		// Charging
		if (self->client->Lmd.CastAbility.indicatorDebounceTime < level.time) {
			self->client->Lmd.CastAbility.indicatorDebounceTime = level.time + 1200;
			G_AddEvent(self, EV_PLAY_EFFECT_ID, G_EffectIndex("misc/possession"));
		}

		self->client->Lmd.customSpeed.value = 0;
		self->client->Lmd.customSpeed.time = level.time + 100;
		self->client->ps.saberHolstered = 2;
		self->client->ps.saberMove = LS_NONE;
		self->client->ps.saberBlocked = 0;
		self->client->ps.saberBlocking = 0;
		self->client->ps.forceHandExtend = HANDEXTEND_NONE;
		self->client->ps.forceHandExtendTime = level.time + 100;
		self->client->ps.fd.forcePower = 0;
		self->client->ps.velocity[0] = 0;
		self->client->ps.velocity[1] = 0;
		
		if (self->client->ps.velocity[2] > 0) {
			self->client->ps.velocity[2] = -self->client->ps.velocity[2];
		}

		self->client->ps.velocity[2] = 80;

		G_SetAnim(self, SETANIM_BOTH, BOTH_FORCE_ABSORB_START, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD | SETANIM_FLAG_RESTART, 0);

		vec3_t center;
		VectorCopy(self->client->ps.origin, center);
		int	nearbyEnts[MAX_GENTITIES];
		int nearbyEntsCount = CommonUtil::getNearbyEnts(center, 2048, nearbyEnts);

		for (int i = 0; i < nearbyEntsCount; i++) {
			gentity_t *ent = GetEnt(nearbyEnts[i]);

			if (ent == self) {
				continue;
			}

			if (!ent || !ent->client || !ent->inuse || ent->health < 1) {
				continue;
			}

			vec3_t dir;
			VectorSubtract(ent->client->ps.origin, center, dir);
			VectorNormalize(dir);
			vec3_t angles;
			vectoangles(dir, angles);
			vec3_t newViewangles;
			newViewangles[0] = -angles[0];
			newViewangles[1] = angles[1] - 180;
			newViewangles[2] = 0;
			SetClientViewAngle(ent, newViewangles);
			G_SetAnim(ent, SETANIM_BOTH, BOTH_COWER1, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD | SETANIM_FLAG_RESTART, 0);
		}

		break;
	}
	case 2: {
		G_SetAnim(self, SETANIM_BOTH, BOTH_FORCE_ABSORB_END, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD | SETANIM_FLAG_RESTART, 0);
		self->client->ps.velocity[0] = 0;
		self->client->ps.velocity[1] = 0;

		if (self->client->ps.velocity[2] > 0) {
			self->client->ps.velocity[2] = -self->client->ps.velocity[2];
		}

		self->client->ps.velocity[2] = -120;

		vec3_t center;
		VectorCopy(self->client->ps.origin, center);
		int	nearbyEnts[MAX_GENTITIES];
		int nearbyEntsCount = CommonUtil::getNearbyEnts(center, 2048, nearbyEnts);

		for (int i = 0; i < nearbyEntsCount; i++) {
			gentity_t *ent = GetEnt(nearbyEnts[i]);

			if (ent == self) {
				continue;
			}

			if (!ent || !ent->client || !ent->inuse || ent->health < 1) {
				continue;
			}

			ent->client->ps.velocity[0] = 0;
			ent->client->ps.velocity[1] = 0;

			if (ent->client->ps.velocity[2] > 0) {
				ent->client->ps.velocity[2] = -ent->client->ps.velocity[2];
			}

			G_SetAnim(ent, SETANIM_BOTH, BOTH_COWER1_STOP, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD | SETANIM_FLAG_RESTART, 0);
		}
	}
	}
}

void LmdCastChosenOne::endPhase(gentity_t *self) {
	switch (self->client->Lmd.CastAbility.currentPhaseNum) {
	case 0: {
		// Ready end.
		self->client->Lmd.CastAbility.currentPhaseNum = 1;
		self->client->Lmd.CastAbility.currentPhaseEndTime = level.time + 5000;
		self->client->Lmd.CastAbility.currentPhaseType = 3;

		int soundNum = Q_irand(0, 15);
		char *soundPath;

		switch (soundNum) {
		case 0: soundPath = "sound/chars/kyle/18kyk017.mp3"; break;
		case 1: soundPath = "sound/chars/kyle/19kyk012.mp3"; break;
		case 2: soundPath = "sound/chars/kyle/21kyk006.mp3"; break;
		case 3: soundPath = "sound/chars/kyle/21kyk007.mp3"; break;
		case 4: soundPath = "sound/chars/kyle/misc/mkyk_05.mp3"; break;
		case 5: soundPath = "sound/chars/kyle/misc/taunt1.mp3"; break;
		case 6: soundPath = "sound/chars/kyle/misc/taunt2.mp3"; break;
		case 7: soundPath = "sound/chars/kyle/misc/taunt3.mp3"; break;
		case 8: soundPath = "sound/chars/kyle/misc/victory1.mp3"; break;
		case 9: soundPath = "sound/chars/kyle/misc/victiory3.mp3"; break;
		case 10: soundPath = "sound/chars/kyle_boss/misc/anger3.mp3"; break;
		case 11: soundPath = "sound/chars/kyle_boss/misc/gloat3.mp3"; break;
		case 12: soundPath = "sound/chars/kyle_boss/misc/taunt3.mp3"; break;
		case 13: soundPath = "sound/chars/kyle_boss/misc/victory3.mp3"; break;
		case 14: soundPath = "chars/jaden_male/29jak003.mp3"; break;
		case 15: soundPath = "chars/jaden_male/29jak016.mp3"; break;
		}

		G_Sound(self, CHAN_VOICE, G_SoundIndex(soundPath));

		break;
	}
	case 1: {
		// Charging end. You are now the chosen one!
		self->client->Lmd.CastAbility.currentPhaseNum = 2;
		self->client->Lmd.CastAbility.currentPhaseEndTime = level.time + 2500;
		self->client->Lmd.CastAbility.currentPhaseType = 3;
		self->client->ps.fd.forcePowerMax = 500;
		self->client->ps.fd.forcePower = 500;
		self->client->Lmd.CastAbility.states |= LmdAbilityStates::THE_CHOSEN_ONE;
		self->client->Lmd.CastAbility.dealtDamage = 0;

		int soundNum = Q_irand(0, 15);
		char *soundPath;

		switch (soundNum) {
		case 0: soundPath = "sound/chars/kyle/18kyk017.mp3"; break;
		case 1: soundPath = "sound/chars/kyle/19kyk012.mp3"; break;
		case 2: soundPath = "sound/chars/kyle/21kyk006.mp3"; break;
		case 3: soundPath = "sound/chars/kyle/21kyk007.mp3"; break;
		case 4: soundPath = "sound/chars/kyle/misc/mkyk_05.mp3"; break;
		case 5: soundPath = "sound/chars/kyle/misc/taunt1.mp3"; break;
		case 6: soundPath = "sound/chars/kyle/misc/taunt2.mp3"; break;
		case 7: soundPath = "sound/chars/kyle/misc/taunt3.mp3"; break;
		case 8: soundPath = "sound/chars/kyle/misc/victory1.mp3"; break;
		case 9: soundPath = "sound/chars/kyle/misc/victiory3.mp3"; break;
		case 10: soundPath = "sound/chars/kyle_boss/misc/anger3.mp3"; break;
		case 11: soundPath = "sound/chars/kyle_boss/misc/gloat3.mp3"; break;
		case 12: soundPath = "sound/chars/kyle_boss/misc/taunt3.mp3"; break;
		case 13: soundPath = "sound/chars/kyle_boss/misc/victory3.mp3"; break;
		case 14: soundPath = "chars/jaden_male/29jak003.mp3"; break;
		case 15: soundPath = "chars/jaden_male/29jak016.mp3"; break;
		}

		G_Sound(self, CHAN_VOICE, G_SoundIndex(soundPath));

		// Let everyone else get up XD

		/*
		for (int i = 0; i < self->client->Lmd.castForceAffectedEntsCount; i++) {
		if (self->client->Lmd.castForceAffectedEnts[i] == -1) {
		continue;
		}

		gentity_t *ent = &g_entities[self->client->Lmd.castForceAffectedEnts[i]];

		if (ent->health > 0 && ent->inuse) {
		//G_EntitySound(ent, CHAN_VOICE, G_SoundIndex("*gasp.wav"));
		}

		if (ent->inuse) {
		ent->client->ps.forceGripChangeMovetype = PM_NORMAL;
		}

		G_Damage(ent, self, self, NULL, NULL, 100000, DAMAGE_NO_PROTECTION, MOD_FORCE_DARK);
		}

		self->client->Lmd.castForceAffectedEntsCount = 0;
		self->client->ps.torsoTimer = 0;
		self->client->ps.legsTimer = 0;
		*/

		break;
	}
	case 2: {
		self->client->Lmd.CastAbility.type = 0;
		self->client->ps.torsoTimer = 0;
		self->client->ps.legsTimer = 0;
		self->client->ps.velocity[2] = 0;
		vec3_t center;
		VectorCopy(self->client->ps.origin, center);
		int	nearbyEnts[MAX_GENTITIES];
		int nearbyEntsCount = CommonUtil::getNearbyEnts(center, 2048, nearbyEnts);

		for (int i = 0; i < nearbyEntsCount; i++) {
			gentity_t *ent = GetEnt(nearbyEnts[i]);

			if (ent == self) {
				continue;
			}

			if (!ent || !ent->client || !ent->inuse || ent->health < 1) {
				continue;
			}

			ent->client->ps.torsoTimer = 0;
			ent->client->ps.legsTimer = 0;
		}
	}
	}
}

void LmdCastChosenOne::cancelPhase(gentity_t *self) {
	self->client->ps.torsoTimer = 0;
	self->client->ps.legsTimer = 0;
	self->client->Lmd.CastAbility.type = 0;
}