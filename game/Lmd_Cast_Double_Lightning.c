#include "Lmd_Cast_Double_Lightning.h"

void LmdCastDoubleLightning::start(gentity_t *self) {
	self->client->Lmd.CastAbility.currentPhaseNum = 0;
	self->client->Lmd.CastAbility.currentPhaseEndTime = level.time + 2000;
	self->client->Lmd.CastAbility.currentPhaseType = LmdAbilityPhaseType::PRESS_USE;
}

void LmdCastDoubleLightning::runPhase(gentity_t *self) {
	switch (self->client->Lmd.CastAbility.currentPhaseNum) {
	case 0:
		if (self->client->Lmd.CastAbility.indicatorDebounceTime < level.time) {
			self->client->Lmd.CastAbility.indicatorDebounceTime = level.time + 100;
			G_AddEvent(self, EV_PLAY_EFFECT_ID, G_EffectIndex("force/rage2"));
		}

		break;
	case 1:
		if (self->client->Lmd.CastAbility.indicatorDebounceTime < level.time) {
			self->client->Lmd.CastAbility.indicatorDebounceTime = level.time + 100;
			G_AddEvent(self, EV_PLAY_EFFECT_ID, G_EffectIndex("force/rage2"));
			G_SetAnim(self, SETANIM_TORSO, BOTH_FORCE_RAGE, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD | SETANIM_FLAG_RESTART, 0);
			self->client->ps.saberHolstered = 2;
			self->client->ps.saberMove = LS_NONE;
			self->client->ps.saberBlocked = 0;
			self->client->ps.saberBlocking = 0;
		}

		break;
	case 2:
		if (self->client->ps.fd.forcePower < 1) {
			endPhase(self);
			return;
		}

		bool showIndicator = false;

		if (self->client->Lmd.CastAbility.indicatorDebounceTime < level.time) {
			self->client->Lmd.CastAbility.indicatorDebounceTime = level.time + 40;
			showIndicator = true;
		}

		if (showIndicator) {
			if (self->client->ps.activeForcePass == 3) {
				self->client->ps.activeForcePass = 6;
			}
			else {
				self->client->ps.activeForcePass = 3;
			}
		}

		G_SetAnim(self, SETANIM_TORSO, BOTH_FORCE_2HANDEDLIGHTNING_HOLD, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD | SETANIM_FLAG_RESTART, 0);
		self->client->ps.saberHolstered = 2;
		self->client->ps.saberMove = LS_NONE;
		self->client->ps.saberBlocked = 0;
		self->client->ps.saberBlocking = 0;
		self->client->ps.fd.forcePower -= 2;

		trace_t	tr;
		vec3_t	end, forward;
		gentity_t	*traceEnt;

		AngleVectors(self->client->ps.viewangles, forward, NULL, NULL);
		vec3_t	center, mins, maxs;
		int			iEntityList[MAX_GENTITIES];
		int		e, numListedEntities, i;

		VectorCopy(self->client->ps.origin, center);
		int range = 768;

		for (i = 0; i < 3; i++) {
			mins[i] = center[i] - range;
			maxs[i] = center[i] + range;
		}

		numListedEntities = trap_EntitiesInBox(mins, maxs, iEntityList, MAX_GENTITIES);

		for (e = 0; e < numListedEntities; e++) {
			gentity_t *traceEnt = &g_entities[iEntityList[e]];

			if (traceEnt == self)
				continue;

			if (!traceEnt->takedamage)
				continue;

			if (traceEnt->health <= 0) //no torturing corpses
				continue;

			if (!g_friendlyFire.integer && OnSameTeam(self, traceEnt))
				continue;

			if (CommonUtil::isWithinFrontConeOf(traceEnt, self, range, forward, center, 0.5)) {
				Force_Lightning_Damage(self, traceEnt, forward, false, 2, 1);
			}
		}

		break;
	}
}

void LmdCastDoubleLightning::endPhase(gentity_t *self) {
	switch (self->client->Lmd.CastAbility.currentPhaseNum) {
	case 0:
		self->client->Lmd.CastAbility.currentPhaseNum = 1;
		self->client->Lmd.CastAbility.currentPhaseEndTime = level.time + 500;
		self->client->Lmd.CastAbility.currentPhaseType = LmdAbilityPhaseType::HOLD_USE;
		G_EntitySound(self, CHAN_VOICE, G_SoundIndex("*falling1.wav"));
		break;
	case 1: {
		self->client->Lmd.CastAbility.currentPhaseNum = 2;
		self->client->Lmd.CastAbility.currentPhaseEndTime = level.time + Q3_INFINITE;
		self->client->Lmd.CastAbility.currentPhaseType = LmdAbilityPhaseType::HOLD_USE;
		self->client->Lmd.CastAbility.dealtDamage = 0;
		break;
	}
	case 2:
		self->client->ps.activeForcePass = 0;
		self->client->ps.torsoTimer = 0;
		self->client->ps.legsTimer = 0;
		self->client->Lmd.CastAbility.type = 0;
		self->client->Lmd.CastAbility.dealtDamage = 0;
		break;
	}
}

void LmdCastDoubleLightning::cancelPhase(gentity_t *self) {
	if (self->client->Lmd.CastAbility.currentPhaseNum > 0) {
		self->client->Lmd.CastAbility.dealtDamage = 0;
	}

	self->client->ps.activeForcePass = 0;
	self->client->ps.torsoTimer = 0;
	self->client->ps.legsTimer = 0;
	self->client->Lmd.CastAbility.type = 0;
}

float LmdCastDoubleLightning::Force_Lightning_AbsorbPower(gentity_t *self, gentity_t *targ, unsigned int forcePower) {
	int powerLevel;
	powerLevel = self->client->ps.fd.forcePowerLevel[FP_LIGHTNING];
	int modPowerLevel = WP_AbsorbConversion(targ, targ->client->ps.fd.forcePowerLevel[FP_ABSORB], self,
		FP_LIGHTNING, powerLevel, forcePower);
	if (modPowerLevel == -1)
		modPowerLevel = powerLevel;
	return ((float)modPowerLevel) / ((float)powerLevel);
}

void LmdCastDoubleLightning::Force_Lightning_Damage(gentity_t *self, gentity_t *target, vec3_t dir, bool twohanded, int dmg, unsigned int forcePower) {
	if (!target->client && target->s.eType == ET_NPC){
		//g2animent
		if (target->s.genericenemyindex < level.time)
			target->s.genericenemyindex = level.time + 2000;
	}
	if (target->client){
		//an enemy or object
		if (target->client->noLightningTime >= level.time){
			//give them power and don't hurt them.
			target->client->ps.fd.forcePower++;
			if (target->client->ps.fd.forcePower > 100)
				target->client->ps.fd.forcePower = 100;
			return;
		}
		if (ForcePowerUsableOn(self, target, FP_LIGHTNING)){
			//Ufo: default damage is too high
			//int	dmg = 1; //Q_irand(1, 2);

			float absorb = Force_Lightning_AbsorbPower(self, target, forcePower);

			if (absorb == 0)
				return;

			if (absorb != 1) {
				absorb = 1.0f - absorb;
				target->client->noLightningTime = level.time + 100 + (absorb * 300.0f);
			}

			if (self->client->ps.weapon == WP_MELEE && twohanded) {
				//2-handed lightning
				//jackin' 'em up, Palpatine-style
				dmg *= 2;
			}

			if (dmg) {
				if (!self->client->ps.duelInProgress
					&& !target->NPC
					&& target->s.eType != ET_NPC) {

					self->client->Lmd.CastAbility.dealtDamage += dmg;
				}

				if (target->client->ps.stats[STAT_ARMOR] > 0) {
					int armorDmg = dmg - target->client->ps.stats[STAT_ARMOR];

					if (armorDmg > 0) {
						target->client->ps.stats[STAT_ARMOR] = 0;
						dmg -= armorDmg;
						target->client->ps.stats[STAT_HEALTH] -= dmg;
						target->health -= dmg;
					}
					else {
						target->client->ps.stats[STAT_ARMOR] -= dmg;
					}
				}
				else {
					target->client->ps.stats[STAT_HEALTH] -= dmg;
					target->health -= dmg;
					target->client->damage_blood += dmg;
				}

				if (target->health < 1) {
					player_die(target, self, self, 1, MOD_FORCE_DARK);
				}
				//rww - Shields can now absorb lightning too.
				//	G_Damage( target, self, self, dir, self->client->ps.origin, dmg, 0, MOD_FORCE_DARK );
			}

			if (target->client) {
				if (!Q_irand(0, 2)){
					G_Sound(target, CHAN_BODY, G_SoundIndex(va("sound/weapons/force/lightninghit%i", Q_irand(1, 3))));
				}

				if (target->client->ps.electrifyTime < (level.time + 400)){
					//only update every 400ms to reduce bandwidth usage (as it is passing a 32-bit time value)
					target->client->ps.electrifyTime = level.time + 800;
				}

				if (target->client->ps.powerups[PW_CLOAKED]){
					//disable cloak temporarily
					Jedi_Decloak(target);
					target->client->cloakToggleTime = level.time + Q_irand(3000, 10000);
				}
			}

		}
	}
}

void LmdCastDoubleLightning::Jedi_Decloak(gentity_t *self)
{
	if (self)
	{
		self->flags &= ~FL_NOTARGET;
		if (self->client)
		{
			//RoboPhred
			gentity_t *targ;
			qboolean doSound = qfalse;
			//Dont check the cvar here, in case cvar is turned off while someone is cloaking.
			if (self->client->ps.m_iVehicleNum) {
				targ = &g_entities[self->client->ps.m_iVehicleNum];
				//Only the pilot can control our cloaking
				//Ufo: sanity check
				if (targ && targ->m_pVehicle && self == (gentity_t *)targ->m_pVehicle->m_pPilot) {
					if (self->client->ps.powerups[PW_CLOAKED]) {
						targ->client->ps.powerups[PW_CLOAKED] = 0;
						doSound = qtrue;
					}
				}
			}
			if (self->client->ps.powerups[PW_CLOAKED])
			{//Uncloak
				self->client->ps.powerups[PW_CLOAKED] = 0;
				doSound = qtrue;
			}
			if (doSound) {
				G_Sound(self, CHAN_ITEM, G_SoundIndex("sound/chars/shadowtrooper/decloak.wav"));
			}
		}
	}
}

int LmdCastDoubleLightning::WP_AbsorbConversion(gentity_t *attacked, int atdAbsLevel, gentity_t *attacker, int atPower, int atPowerLevel, int atForceSpent)
{
	int getLevel = 0;
	int addTot = 0;
	gentity_t *abSound;

	if (atPower != FP_LIGHTNING &&
		atPower != FP_DRAIN &&
		atPower != FP_GRIP &&
		atPower != FP_PUSH &&
		atPower != FP_PULL)
	{ //Only these powers can be absorbed
		return -1;
	}


	if (!atdAbsLevel)
	{ //looks like attacker doesn't have any absorb power
		return -1;
	}

	// RoboPhred: Passive absorb if lvl 4 or 5.
	qboolean absorbActive = (attacked->client->ps.fd.forcePowersActive & (1 << FP_ABSORB));
	// RoboPhred: Not now
	//if (!absorbActive && atdAbsLevel < 4)
	//{ //absorb is not active
	//	return -1;
	//}
	// else
	if (!absorbActive) {
		// Passive, use lvl 1
		//atdAbsLevel = 1;
		return -1;
	}

	//Subtract absorb power level from the offensive force power
	getLevel = atPowerLevel;
	getLevel -= atdAbsLevel;

	if (getLevel < 0)
	{
		getLevel = 0;
	}

	//let the attacker absorb an amount of force used in this attack based on his level of absorb
	addTot = (atForceSpent / 3)*attacked->client->ps.fd.forcePowerLevel[FP_ABSORB];

	if (addTot < 1 && atForceSpent >= 1)
	{
		addTot = 1;
	}

	attacked->client->ps.fd.forcePower += addTot;

	if (attacked->client->ps.fd.forcePower > 100)
	{
		attacked->client->ps.fd.forcePower = 100;
	}

	//play sound indicating that attack was absorbed
	if (attacked->client->forcePowerSoundDebounce < level.time)
	{
		abSound = G_PreDefSound(attacked->client->ps.origin, PDSOUND_ABSORBHIT);
		abSound->s.trickedentindex = attacked->s.number;

		attacked->client->forcePowerSoundDebounce = level.time + 400;
	}

	return getLevel;
}