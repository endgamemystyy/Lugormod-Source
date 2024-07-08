#include "common_util.h"

bool CommonUtil::isWithinFrontConeOf(gentity_t *target, gentity_t *player, int range, vec3_t forward, vec3_t playerCenter, float angle) {
	vec3_t	dir, ent_org, size, v;
	trace_t tr;

	//this is all to see if we need to start a saber attack, if it's in flight, this doesn't matter
	// find the distance from the edge of the bounding box
	for (int i = 0; i < 3; i++) {
		if (playerCenter[i] < target->r.absmin[i])
			v[i] = target->r.absmin[i] - playerCenter[i];
		else if (playerCenter[i] > target->r.absmax[i])
			v[i] = playerCenter[i] - target->r.absmax[i];
		else
			v[i] = 0;
	}

	VectorSubtract(target->r.absmax, target->r.absmin, size);
	VectorMA(target->r.absmin, 0.5, size, ent_org);
	VectorSubtract(ent_org, playerCenter, dir);
	VectorNormalize(dir);

	if (DotProduct(dir, forward) < 1 - angle)
		return false;

	//must be close enough
	if (VectorLength(v) >= range) {
		return false;
	}

	//in PVS?
	if (!target->r.bmodel && !trap_InPVS(ent_org, player->client->ps.origin)) {
		return false;
	}

	trap_Trace(&tr, player->client->ps.origin, vec3_origin, vec3_origin, ent_org, player->s.number, MASK_SHOT);

	if (tr.fraction < 1.0f && tr.entityNum != target->s.number) {
		//must have clear line of sight
		return false;
	}

	return true;
}

void CommonUtil::loadForward(gclient_s &client, vec3_t &forward) {
	AngleVectors(client.ps.viewangles, forward, NULL, NULL);
	VectorNormalize(forward);
}

int CommonUtil::getNearbyEnts(vec3_t center, int range, int *outEnts) {
	vec3_t mins, maxs;

	for (int i = 0; i < 3; i++) {
		mins[i] = center[i] - range;
		maxs[i] = center[i] + range;
	}

	return trap_EntitiesInBox(mins, maxs, outEnts, MAX_GENTITIES);
}