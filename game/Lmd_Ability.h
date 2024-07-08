#ifndef LMD_ABILITY_H
#define LMD_ABILITY_H

#include "g_local.h"
#include "Lmd_Cast_Force_Power_Value.h"
#include "Lmd_Cast_Force_Retribution.h"
#include "Lmd_Cast_Chosen_One.h"
#include "Lmd_Cast_Force_Whirlwind.h"
#include "Lmd_Cast_Double_Lightning.h"
#include "Lmd_Ability_States.h"

namespace LmdAbility {
	enum Type {
		NONE,
		FORCE_RETRIBUTION,
		FORCE_WHIRLWIND,
		CHOSEN_ONE,
		DOUBLE_LIGHTNING,
		MAX
	};

	bool update(gentity_t *self, usercmd_t *ucmd);
	void g_damage(gentity_t *target, gentity_t *attacker, int mod);
}

#endif