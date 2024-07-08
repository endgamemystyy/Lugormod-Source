#ifndef COMMON_UTIL_H
#define COMMON_UTIL_H

#include "g_local.h"

namespace CommonUtil {
	bool isWithinFrontConeOf(gentity_t *target, gentity_t *player, int range, vec3_t forward, vec3_t playerCenter, float angle);
	void loadForward(gclient_s &client, vec3_t &forward);
	int getNearbyEnts(vec3_t center, int range, int *nearbyEnts);
}

#endif