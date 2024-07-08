

#include "g_local.h"
#include "Lmd_KeyPairs.h"
#include "Lmd_Accounts_Data.h"
#include "Lmd_Accounts_Core.h"
#include "Lmd_Arrays.h"
#include "Lmd_Inventory.h"
#include "Lmd_PlayerActions.h"
#include "Lmd_Confirm.h"
#include "Lmd_Ability.h"
#include "Lmd_Professions.h"

int AccInventoryDataDataIndex = -1;
#define INVDATA(acc) (iObjectList_t *)Lmd_Accounts_GetAccountCategoryData(acc, AccInventoryDataDataIndex)
qboolean Inventory_Quest_DownCount_CheckAccess(iObject_t *obj, char *prop);
void Inventory_Quest_DownCount_Decrement(gentity_t *player, iObject_t *obj, int decrementCount);
void hurl(gentity_t *ent, gentity_t *dropped);
void Inventory_DownCount_Add_InfoString(gentity_t *ent, iObject_t *obj, int maxEntries);
int Inventory_DownCount_GetCount(iObject_t *obj);

extern void NPC_SetAnim(gentity_t *ent, int setAnimParts, int anim, int setAnimFlags);
gentity_t *trySpawn(char *string);
void G_Knockdown(gentity_t *victim, int duration);

/*

IMPORTANT:
Make sure all items run a player modified event when they are modified.

*/

int ForceDash(gentity_t* ent);
int BattleMeditation(gentity_t* ent);
void DestructionMissile(gentity_t* ent);
int ForceDestruction(gentity_t* ent);
int SummonBacta(gentity_t* ent);
void BombThink(gentity_t* self);
void TimeBombThink(gentity_t* self);
void cliAimTrace(trace_t* tr, gentity_t* ent, int distance = 8192, vec3_t mins = 0, vec3_t maxs = 0);
int TimeBomb(gentity_t* ent);
int SkillDuration(gentity_t* ent, int* var, int duration);
int SlowRegen(gentity_t* ent);
int DisableJP(gentity_t* ent);
int ForceSever(gentity_t* ent);
void CloakStop(gentity_t* to);
int CloakSkill(gentity_t* ent);
int ForceSlow(gentity_t* ent);
int MercSprint(gentity_t* ent);
int PoisonDart(gentity_t* ent);
void Repulse(gentity_t* ent, qboolean pull, float knockback, float distance);
int RepulseBegin(gentity_t* ent);
void ForceScreamDo(gentity_t* ent, float distance);

int ForceScream(gentity_t* ent);

BG_field_t Item_Fields[] = {
	{"name", FOFFSET(iObjectFields_t, name), F_GSTRING},
	{"noCombine", FOFFSET(iObjectFields_t, noCombine), F_INT},
	{"noDelete", FOFFSET(iObjectFields_t, noDelete), F_INT},
	{"noGive", FOFFSET(iObjectFields_t, noGive), F_INT},
	{NULL, 0}
};

extern iObjectDef_t *Inventory_Items_General[];
extern iObjectDef_t *Inventory_Items_Quest[];
iObjectCatagory_t catagories[] = {
	{"General", Inventory_Items_General},
	{"Quest", Inventory_Items_Quest},
	{NULL, NULL}
};

iObjectCatagory_t *Inventory_GetCategory(char *name){
	iObjectCatagory_t *category = catagories;
	while(category->name){
		if(Q_stricmp(category->name, name) == 0)
			return category;
		category++;
	}
	return NULL;
}

iObjectDef_t *Inventory_GetObjectDef(char *name, iObjectCatagory_t **catagory){
	iObjectCatagory_t *search = catagories;
	iObjectDef_t **def;
	while(search->name){
		def = search->objects;
		while(*def){
			if(Q_stricmp((*def)->name, name) == 0) {
				if(catagory)
					(*catagory) = search;
				return (*def);
			}
			def++;
		}
		search++;
	}
	return NULL;
}


iObject_t *Inventory_LoadObject(iObjectDef_t *def, char *data);
qboolean Inventory_AddObject(iObjectList_t *inventory, iObject_t *obj);

qboolean Accounts_Inventory_Parse(char *key, char *value, void *target, void *args)
{
	iObjectList_t *inventory = (iObjectList_t *)target;

	char name[MAX_STRING_CHARS];
	char keys[MAX_STRING_CHARS];

	if(sscanf((const char *)value, "\"%1023[^\"]\" {%1023[^}]}",  name, keys) != 2) {
		int creator = 0;
		if(sscanf((const char *)value, "\"%1023[^\"]\" %i {%1023[^}]}", name, &creator, keys) < 3)
			return qtrue; //was ment for us, but invalid.
	}

	iObjectDef_t *def = Inventory_GetObjectDef(name, NULL);
	if(!def)
		return qfalse;

	iObject_t *obj = Inventory_LoadObject(def, keys);
	Inventory_AddObject(inventory, obj);
	return qtrue;
}

void Inventory_WriteObject(iObject_t *obj, char *data, unsigned int sze);



typedef struct InventoryWriteState_s {
	int index;
} InventoryWriteState_t;

DataWriteResult_t Accounts_Inventory_Write(void *target, char key[], int keySize, char value[], int valueSize, void **writeState, void *args)
{
	iObjectList_t *inventory = (iObjectList_t *)target;

	void* statePtr = *writeState;
	if (statePtr == NULL) {
		statePtr = *writeState = G_Alloc(sizeof(InventoryWriteState_t));
	}

	InventoryWriteState_t *state = (InventoryWriteState_t *)statePtr;


	if (state->index >= inventory->count) {
		G_Free(state);
		return DWR_NODATA;
	}

	char objstr[MAX_STRING_CHARS] = "";
	Inventory_WriteObject(inventory->objects[state->index], objstr, sizeof(objstr));
	Q_strncpyz(value, va("\"%s\" {%s}", inventory->objects[state->index]->def->name, objstr), valueSize);

	state->index++;

	if (state->index >= inventory->count) {
		G_Free(state);
		return DWR_COMPLETE;
	}

	return DWR_CONTINUE;
}

#define InventoryFields_Base(_m) \
	_m##_FUNC(inventory, Accounts_Inventory_Parse, Accounts_Inventory_Write, NULL)

InventoryFields_Base(DEFINE_FIELD_PRE)

DATAFIELDS_BEGIN(InventoryFields)
InventoryFields_Base(DEFINE_FIELD_LIST)
DATAFIELDS_END

const int InventoryFields_Count = DATAFIELDS_COUNT(InventoryFields);

void Inventory_FreeObject(iObject_t *obj);
void Accounts_Inventory_Free(void *target){
	iObjectList_t *inventory = (iObjectList_t *)target;
	int i;
	for(i = 0; i < inventory->count; i++) {
		Inventory_FreeObject(inventory->objects[i]);
	}
	Lmd_Arrays_RemoveAllElements((void **)&inventory->objects);
}

accDataModule_t Accounts_Inventory = {
	// dataFields
	InventoryFields,

	// numDataFields
	InventoryFields_Count,

	// dataSize
	sizeof(iObjectList_t),

	// allocData
	NULL,

	// freeData
	Accounts_Inventory_Free,
};


void Accounts_Inventory_Register() {
	AccInventoryDataDataIndex = Lmd_Accounts_AddDataCategory(&Accounts_Inventory);
}

iObjectList_t *Inventory_Player_GetInventory(gentity_t *player) {
	if(!player->client->pers.Lmd.account)
		return NULL;
	return INVDATA(player->client->pers.Lmd.account);
}

iObject_t *Inventory_NewObject(iObjectDef_t *objDef) {
	iObject_t *obj = (iObject_t *)G_Alloc(sizeof(iObject_t));

	obj->def = objDef;
	obj->data = G_Alloc(objDef->dataSize);
	memset(obj->data, 0, objDef->dataSize);
	//Init local name
	obj->fields.name = G_NewString2(objDef->name);
	return obj;
}

void Inventory_SpawnObject(iObject_t *obj) {
	if(obj->def->spawnObj)
		obj->def->spawnObj(obj);
}

iObject_t *Inventory_LoadObject(iObjectDef_t *def, char *data) {
	iObject_t *obj = Inventory_NewObject(def);
	char *str = data;
	Lmd_Data_ParseDatastring(&str, NULL, Item_Fields, (byte *)&obj->fields);
	str = data;
	Lmd_Data_ParseDatastring(&str, NULL, def->fields, (byte *)obj->data);
	Inventory_SpawnObject(obj); //FIXME: should probably be done on player spawn, not on item spawn.  Or a new callback needs to be made.
	return obj;
}

qboolean Lmd_Data_WriteDatastringField( BG_field_t *f, char *value, unsigned int sze, byte *b );
void Inventory_WriteObject(iObject_t *obj, char *data, unsigned int sze) {
	char buf[MAX_STRING_CHARS] = "";
	BG_field_t *field = Item_Fields;
	while(field->name) {
		if(Lmd_Data_WriteDatastringField(field, buf, sizeof(buf), (byte *)&obj->fields))
			Q_strcat(data, sze, buf);
		field++;
	}

	field = obj->def->fields;
	while(field->name) {
		if(Lmd_Data_WriteDatastringField(field, buf, sizeof(buf), (byte *)obj->data))
			Q_strcat(data, sze, buf);
		field++;
	}
}


void Inventory_FreeObject(iObject_t *obj) {
	if(obj->def->freeObj)
		obj->def->freeObj(obj);
	BG_FreeFields(obj->def->fields, (byte *)obj->data);
	BG_FreeFields(Item_Fields, (byte *)&obj->fields);
	G_Free(obj->data);
	G_Free(obj);
}

void Inventory_Holder_RemoveObject(iObject_t *obj);
qboolean Inventory_DestroyObject(iObject_t *obj){
	Inventory_Holder_RemoveObject(obj);
	if(obj->del)
		obj->del(obj);

	Inventory_FreeObject(obj);

	return qtrue;
}

qboolean Inventory_DeleteObject(iObject_t *obj){
	if(obj->del && !obj->del(obj))
		return qfalse;
	return Inventory_DestroyObject(obj);
}

qboolean Inventory_TryCombine(iObject_t *obj, iObject_t *targ){
	BG_field_t *field = obj->def->fields;
	qboolean canMerge = qfalse;
	qboolean combined = qfalse;

	//Check merge requirements
	if(Q_stricmp(obj->fields.name, targ->fields.name) != 0)
		return qfalse;
	if(obj->fields.noCombine || targ->fields.noCombine)
		return qfalse;

	while(field->name){
		if(field->flags != COMBINE_IGNORE)
			canMerge = qtrue;
		if(field->flags == COMBINE_MATCH){
			if(!BG_CompareFields(field, (byte *)obj, (byte *)targ))
				return qfalse;
		}
		field++;
	}
	//We can combine, check aquire on target
	if(targ->combine){
		if(!targ->combine(targ, obj))
			return qfalse;
		combined = qtrue;
	}
	if(canMerge) {
		field = obj->def->fields;
		byte *targD = (byte *)targ->data, *objD = (byte *)obj->data;
		while(field->name){
			if(field->flags == COMBINE_ADD){
				switch(field->type){
				case F_INT:
					*(int *)(targD+field->ofs) = *(int *)(targD+field->ofs) + *(int *)(objD+field->ofs);
					break;
				case F_FLOAT:
					*(float *)(targD+field->ofs) = *(float *)(targD+field->ofs) + *(float *)(objD+field->ofs);
					break;
				default:
					assert(!"Unhandled F_ type in COMBINE_ADD");
				}
			}
			else if(field->flags == COMBINE_REPLACE){
				BG_CopyField(field, targD, objD);
			}
			field++;
		}
	}
	//Can only merge if we have at least one non-ignore flag.
	return canMerge || combined;
}

qboolean Inventory_CheckCombine(iObjectList_t *inventory, iObject_t *obj){
	int i;
	for(i = 0; i < inventory->count; i++){
		if(inventory->objects[i]->def != obj->def)
			continue;
		if(Inventory_TryCombine(obj, inventory->objects[i]))
			return qtrue;
	}
	return qfalse;
}

qboolean Inventory_AddObject(iObjectList_t *inventory, iObject_t *obj){
	if(Inventory_CheckCombine(inventory, obj)){
		//We were combined, remove the item.
		Inventory_DestroyObject(obj);
		return qfalse;
	}
	else {
		int index = Lmd_Arrays_AddArrayElement((void **)&inventory->objects, sizeof(iObject_t *), &inventory->count);
		inventory->objects[index] = obj;
		return qtrue;
	}
}

void Inventory_RemoveObject(iObject_t *obj){
	int index = -1, i;
	iObjectList_t *inventory = INVDATA(obj->holder->client->pers.Lmd.account);

	for(i = 0; i < inventory->count; i++){
		if(inventory->objects[i] == obj){
			index = i;
			break;
		}
	}
	assert(index != -1);
	if(index == -1)
		return; //should never come up
	Lmd_Arrays_RemoveArrayElement((void **)&inventory->objects,	index, sizeof(iObject_t *), &inventory->count);
	//Ufo:
	Lmd_Accounts_Modify(obj->holder->client->pers.Lmd.account);
	obj->holder = NULL;
}

void Inventory_Holder_RemoveObject(iObject_t *obj) {
	if(!obj->holder)
		return;
	if(obj->holder->client)
		Inventory_RemoveObject(obj);
	else {
		//remove container.
	}
}

qboolean Inventory_Holder_LooseObject(iObject_t *obj){
	if(!obj->holder)
		return qtrue;
	if(obj->loose && !obj->loose(obj))
		return qfalse;
	if(obj->holder->client){
		Inventory_RemoveObject(obj);
	}
	else{
		//remove container
	}
	return qtrue;
}

void Inventory_Player_Modify(gentity_t *player) {
	PlayerAcc_Modify(player);
}

void Inventory_Player_Think(gentity_t *player) {
	iObjectList_t *inventory = INVDATA(player->client->pers.Lmd.account);
	if(!inventory)
		return;
	iObject_t *obj;
	int i;
	for(i = 0; i < inventory->count; i++) {
		obj = inventory->objects[i];
		if(obj->nextThink > 0 && obj->nextThink <= level.time){
			obj->nextThink = 0;
			obj->think(obj);
		}	
	}
}

void Inventory_Player_Login(gentity_t *player) {
	iObjectList_t *inventory = INVDATA(player->client->pers.Lmd.account);
	int i;
	for(i = 0; i < inventory->count; i++) {
		inventory->objects[i]->holder = player;
	}
}

void Inventory_Player_Logout(gentity_t *player) {
	iObjectList_t *inventory = INVDATA(player->client->pers.Lmd.account);
	int i;
	for(i = 0; i < inventory->count; i++) {
		inventory->objects[i]->holder = NULL;
	}
}

qboolean Inventory_Player_AquireItem(gentity_t* player, iObject_t *obj){
	iObjectList_t *inventory = INVDATA(player->client->pers.Lmd.account);

	//What if we have a holder already?  Run LooseItem?  Assume it is taken care of?
	assert(!obj->holder);

	//Check for aquire before combining
	if(obj->aquire && obj->aquire(obj, player) == qfalse)
		return qfalse;

	///Returns true if a new object was added, false if the object was combined (which means our obj is now invalid).
	if(Inventory_AddObject(inventory, obj))
		obj->holder = player;
	
	//Ufo:
	Lmd_Accounts_Modify(player->client->pers.Lmd.account);
	return qtrue;
}

qboolean Inventory_RunSpawner(gentity_t *spawner, gentity_t *activator) {
	 if(!activator->client->pers.Lmd.account)
		 return qfalse;
	iObject_t *obj = Inventory_LoadObject((iObjectDef_t *)spawner->genericValue1, spawner->GenericStrings[0]);
	if(!Inventory_Player_AquireItem(activator, obj)) {
		Inventory_DestroyObject(obj);
		return qfalse;
	}
	return qtrue;
}

qboolean Inventory_SetupSpawner(gentity_t *ent){
	char infostring[MAX_STRING_CHARS] = "";
	char *val;
	BG_field_t *field;
	iObjectDef_t *def = Inventory_GetObjectDef(ent->fullName, NULL);
	if(!def)
		return qfalse;
	field = Item_Fields;
	while(field->name){
		G_SpawnString(field->name, "", &val);
		if(val && val[0])
			Q_strcat(infostring, sizeof(infostring), va("\"%s\",\"%s\",", field->name, val));
		field++;
	}
	field = def->fields;
	while(field->name){
		G_SpawnString(field->name, "", &val);
		if(val && val[0])
			Q_strcat(infostring, sizeof(infostring), va("\"%s\",\"%s\",", field->name, val));
		field++;
	}
	ent->genericValue1 = (int)def;
	ent->GenericStrings[0] = G_NewString2(infostring);
	return qtrue;
}

void Cmd_Inventory_ListCatagory(gentity_t *ent, iObjectList_t *inventory, iObjectCatagory_t *category, char *name, int *offset, int *count){
	int i;
	iObject_t *obj;
	char descr[MAX_STRING_CHARS];
	char compare[MAX_STRING_CHARS];
	for(i = 0; i < inventory->count; i++){
		obj = inventory->objects[i];
		if(name && name[0]) {
			Q_strncpyz(compare, obj->fields.name, sizeof(compare));
			Q_CleanStr(compare);
			if(Q_stricmp(name, compare) != 0)
				continue;
		}
		if(category) {
			iObjectCatagory_t *c = NULL;
			Inventory_GetObjectDef(obj->def->name, &c);
			if(c != category)
				continue;
		}
		if((*offset) > 0){
			(*offset)--;
			continue;
		}
		if(obj->describe)
			obj->describe(obj, descr, sizeof(descr));
		else
			descr[0] = 0;
		if((*count) > 0) {
			Disp(ent, va("^5%-5i    ^2%-25s ^3%s", i, obj->fields.name, descr));
		}
		(*count)--;
	}
}

void Cmd_Inventory_List(gentity_t *ent, iObjectList_t *inventory, int argnum) {
	char arg[MAX_STRING_CHARS];
	char name[MAX_STRING_CHARS] = "";
	int count = 25;
	int offset = 0;
	iObjectCatagory_t *category;

	trap_Argv(argnum, arg, sizeof(arg));
	if(!arg[0]) {
		Disp(ent, "^3/inventory ^2[category or item name] [offset]\n"
			"^3Catagories:");
		category = catagories;
		while(category->name){
			Disp(ent, va("^2%s", category->name));
			category++;
		}
	}
	category = Inventory_GetCategory(arg);
	if(category == NULL) {
		if(atoi(arg) != 0) {
			// This is an offset, subtract the arg count to read it as such.
			argnum -= 1;
		}
		else {
			Q_strncpyz(name, arg, sizeof(name));
			Q_CleanStr(name);
		}
	}
	trap_Argv(argnum + 1, arg, sizeof(arg));
	offset = atoi(arg);
	if(offset < 0)
		offset = 0;
	Disp(ent, "^5Index    ^2Name                      ^3Info");
	Cmd_Inventory_ListCatagory(ent, inventory, category, name, &offset, &count);
	if(count >= 0)
		Disp(ent, va("^2%i^3 item%s displayed.", 25 - count, (count != 24)?"s":""));
	if(count < 0) {
		Disp(ent, va("^2%i^3 out of ^2%i^3 items displayed\n"
			"Specify a number at the end of the command to start listing from that number.",
			25, 25 + (-count)));
	}
}

void Cmd_Inventory_Use(gentity_t *ent, iObjectList_t *inventory, int argnum){
	char arg[MAX_STRING_CHARS];
	if(ent->client->sess.sessionTeam == TEAM_SPECTATOR) {
		Disp(ent, "^3You must be in game to use inventory.");
		return;
	}
	else if(ent->health <= 0) {
		Disp(ent, "^3You must be alive to use inventory.");
		return;
	}
	//Ufo: no more medpacks in duel
	else if(ent->client->ps.duelInProgress) {
		Disp(ent, "^3You cannot use items at this time.");
		return;
	}
	trap_Argv(argnum, arg, sizeof(arg));
	int num = atoi(arg);
	iObject_t *obj = NULL;
	if(num != 0 || (arg[0] == '0' && arg[1] == 0)){
		if(num < 0 || num >= inventory->count){
			Disp(ent, "^3Invalid index.");
			return;
		}
		obj = inventory->objects[num];
	}
	else{
		int i;
		for(i = 0; i < inventory->count; i++){
			if(Q_stricmp(inventory->objects[i]->fields.name, arg) == 0){
				obj = inventory->objects[i];
			}
		}
	}

	if(!obj)
		Disp(ent, "^3Invalid item.");
	else {
		qboolean preUsed = false;

		if (Inventory_Quest_DownCount_CheckAccess(obj, "force_retribution")) {
			ent->client->Lmd.CastAbility.type = LmdAbility::FORCE_RETRIBUTION;
			ent->client->Lmd.CastAbility.invoked = true;
			Disp(ent, "^3Hold your ^2USE ^3button to activate.");
			preUsed = true;
		}
		else if (Inventory_Quest_DownCount_CheckAccess(obj, "force_the_chosen_one")) {
			ent->client->Lmd.CastAbility.type = LmdAbility::CHOSEN_ONE;
			ent->client->Lmd.CastAbility.invoked = true;
			Disp(ent, "^3Press your ^2USE ^3button to activate.");
			preUsed = true;
		}
		else if (Inventory_Quest_DownCount_CheckAccess(obj, "double_lightning")) {
			ent->client->Lmd.CastAbility.type = LmdAbility::DOUBLE_LIGHTNING;
			ent->client->Lmd.CastAbility.invoked = true;
			Disp(ent, "^3Hold your ^2USE ^3button to activate.");
			preUsed = true;
		}
		else if (Inventory_Quest_DownCount_CheckAccess(obj, "force_whirlwind")) {
			ent->client->Lmd.CastAbility.type = LmdAbility::FORCE_WHIRLWIND;
			ent->client->Lmd.CastAbility.invoked = true;
			Disp(ent, "^3Hold your ^2USE ^3button to activate.");
			preUsed = true;
		}
		else if (Inventory_Quest_DownCount_CheckAccess(obj, "desann_mod")) {
			ent->client->Lmd.addDesannToCycle = true;
			Disp(ent, "^3Desann saber style activated.");
			preUsed = true;
		}
		else if (Inventory_Quest_DownCount_CheckAccess(obj, "tavion_mod")) {
			ent->client->Lmd.addTavionToCycle = true;
			Disp(ent, "^3Tavion saber style activated.");
			preUsed = true;
		}
		else if (Inventory_Quest_DownCount_CheckAccess(obj, "force_dash")) {
			ForceDash(ent);
			preUsed = true;
		}
		else if (Inventory_Quest_DownCount_CheckAccess(obj, "battle_meditation")) {
			BattleMeditation(ent);
			preUsed = true;
		}
		else if (Inventory_Quest_DownCount_CheckAccess(obj, "destruction_missile")) {
			DestructionMissile(ent);
			preUsed = true;
		}
		else if (Inventory_Quest_DownCount_CheckAccess(obj, "summon_bacta")) {
			SummonBacta(ent);
			preUsed = true;
		}
		else if (Inventory_Quest_DownCount_CheckAccess(obj, "time_bomb")) {
			TimeBomb(ent);
			preUsed = true;
		}
		else if (Inventory_Quest_DownCount_CheckAccess(obj, "disable_jp")) {
			DisableJP(ent);
			preUsed = true;
		}
		else if (Inventory_Quest_DownCount_CheckAccess(obj, "force_slow")) {
			ForceSlow(ent);
			preUsed = true;
		}
		else if (Inventory_Quest_DownCount_CheckAccess(obj, "sprint")) {
			MercSprint(ent);
			preUsed = true;
		}
		else if (Inventory_Quest_DownCount_CheckAccess(obj, "force_scream")) {
			ForceScream(ent);
			preUsed = true;
		}
		else if (Inventory_Quest_DownCount_CheckAccess(obj, "cloak")) {
			CloakSkill(ent);
			preUsed = true;
		}
		else if (Inventory_Quest_DownCount_CheckAccess(obj, "repulse")) {
			RepulseBegin(ent);
			preUsed = true;
		}
		else if (Inventory_Quest_DownCount_CheckAccess(obj, "poison_dart")) {
			PoisonDart(ent);
			preUsed = true;
		}


		if (preUsed) {
			Inventory_Quest_DownCount_Decrement(ent, obj, 1);
		}
		else {
			if (!obj->use)
				Disp(ent, "^3This item is currently not usable.");
			else
				obj->use(obj);
		}
	}
}

/*
typedef struct InventoryGiveItemConfirmData_s {
	iObject_t *obj;
	gentity_t *targ;
	Account_t *acc;
}InventoryGiveItemConfirmData_t;

void Cmd_Inventory_Give_Confirm(gentity_t *ent, void *data){
	InventoryGiveItemConfirmData_t *iData = (InventoryGiveItemConfirmData_t *)data;
	if(iData->obj->holder != ent){
		Disp(ent, "^3You no longer have that item.");
		return;
	}
	if(iData->obj->fields.noGive) {
		Disp(ent, "^3You cannot give this item away.");
		return;
	}
	if(!iData->obj->give(iData, iData->targ)) {
		Disp(ent, "^3You cannot give this item to that player.");
		return;
	}
	if(!Inventory_Holder_LooseObject(iData->obj)) {
		Disp(ent, "^3You cannot give this item right now");
		return;
	}
	if(!Inventory_Player_AquireItem(iData->targ, iData->obj)) {
		Disp(ent, "^3That player cannot pick up this item.");
		Inventory_Player_AquireItem(ent, iData->obj);
	}
}

void Cmd_Inventory_Give(gentity_t *ent, iObjectList_t *inventory, int argnum){
	char arg[MAX_STRING_CHARS];
	trap_Argv(argnum, arg, sizeof(arg));
	gentity_t *target = AimAnyTarget(ent, 64);
	int num = atoi(arg);
	if(num < 0 || num >= inventory->count){
		Disp(ent, "^3Invalid index.");
		return;
	}
	if(iData->obj->fields.noGive) {
		Disp(ent, "^3You cannot give this item away.");
		return;
	}
	if(!target || !target->client) {
		Disp(ent, "^3Invalid player.");
		return;
	}
	if(!target->client->pers.Lmd.account) {
		Disp(ent, "^3This player is not logged in.");
		return;
	}
	if(!iData->obj->give(iData, target)) {
		Disp(ent, "^3You cannot give this item to that player.");
		return;
	}
	Disp(ent, va("^3Give inventory index ^2%i^3 (^2%s^3) to ^7^s", num, inventory->objects[num]->fields.name, target->client->pers.netname));
	InventoryGiveItemConfirmData_t *data = (InventoryGiveItemConfirmData_t *)G_Alloc(sizeof(InventoryGiveItemConfirmData_t));
	data->obj = inventory->objects[num];
	data->targ = target;
	data->acc = target->client->pers.Lmd.account;
	Confirm_Set(ent, Cmd_Inventory_Give_Confirm, data);
}
*/

//Need to have a struct, since data is G_Free'd.
typedef struct InventoryDeleteItemConfirmData_s {
	iObject_t *obj;
}InventoryDeleteConfirmData_t;

void Cmd_Inventory_Delete_Item_Confirm(gentity_t *ent, void *data){
	InventoryDeleteConfirmData_t *iData = (InventoryDeleteConfirmData_t *)data;
	if(iData->obj->holder != ent){
		Disp(ent, "^3You no longer have that item.  Perhapse it was traded or deleted?");
		return;
	}
	if(Inventory_DeleteObject(iData->obj))
		Disp(ent, "^2Item deleted.");
	else
		Disp(ent, "^3Item could not be deleted.");
}

typedef struct InventoryDeleteTypeConfirmData_s {
	iObjectList_t *inventory;
	char type[MAX_STRING_CHARS];
}InventoryDeleteTypeConfirmData_t;

void Cmd_Inventory_Delete_Type_Confirm(gentity_t *ent, void *data){
	InventoryDeleteTypeConfirmData_t *iData = (InventoryDeleteTypeConfirmData_t *)data;
	int i, c = 0;
	char *name = (char *)data;
	for(i = 0; i < iData->inventory->count; i++){
		if(Q_stricmp(iData->inventory->objects[i]->fields.name, name) == 0){
			if(Inventory_DeleteObject(iData->inventory->objects[i])) {
				c++;
				i--;
			}
		}
	}
	Disp(ent, va("^3%i ^2item%s deleted.", c, (c != 1)?"s":""));
}


void Cmd_Inventory_Delete(gentity_t *ent, iObjectList_t *inventory, int argnum){
	char arg[MAX_STRING_CHARS];
	trap_Argv(argnum, arg, sizeof(arg));
	int num = atoi(arg);
	if(num != 0 || (arg[0] == '0' && arg[1] == 0)){
		if(num < 0 || num >= inventory->count){
			Disp(ent, "^3Invalid index.");
			return;
		}
		if(inventory->objects[num]->fields.noDelete) {
			Disp(ent, "^3You cannot delete this item.");
			return;
		}
		Disp(ent, va("^3Delete inventory index ^2%i^3 (^2%s^3)", num, inventory->objects[num]->fields.name));
		InventoryDeleteConfirmData_t *data = (InventoryDeleteConfirmData_t *)G_Alloc(sizeof(InventoryDeleteConfirmData_t));
		data->obj = inventory->objects[num];
		Confirm_Set(ent, Cmd_Inventory_Delete_Item_Confirm, data);
	}
	else{
		int i, c = 0;
		char name[MAX_STRING_CHARS];
		for(i = 0; i < inventory->count; i++){
			Q_strncpyz(name, inventory->objects[i]->fields.name, sizeof(name));
			Q_CleanStr(name);
			if(Q_stricmp(name, arg) == 0){
				c++;
			}
		}
		if(c == 0){
			Disp(ent, "^3You have no items of that name.");
			return;
		}
		Disp(ent, va("^3Delete all items of name ^2%s^3 (^2%i^3 item%s)", inventory->objects[num]->fields.name, c, (c != 1)?"s":""));
		InventoryDeleteTypeConfirmData_t *data = (InventoryDeleteTypeConfirmData_t *)G_Alloc(sizeof(InventoryDeleteTypeConfirmData_t));
		data->inventory = inventory;
		Q_strncpyz(data->type, arg, sizeof(data->type));
		Confirm_Set(ent, Cmd_Inventory_Delete_Item_Confirm, data);
	}
}

void Touch_Iobject(gentity_t *ent, gentity_t *other, trace_t *trace) {
	if (ent->pain_debounce_time > level.time)
		return;
	if (!other || !other->client || other->NPC || other->r.svFlags & SVF_BOT)
		return;
	if (other->health < 1)
		return;
	if (other->client->pers.connected != CON_CONNECTED)
		return;
	if (duelInProgress(&other->client->ps))
		return;
	if (other->client->ps.pm_type == PM_SPECTATOR)
		return;
	if (ent->genericValue11 == other->s.number && ent->genericValue10 > level.time)
		return;

	ent->pain_debounce_time = level.time + 500;

	if (!other->client->pers.Lmd.account){
		return;
	}

	G_Sound(other, CHAN_AUTO, G_SoundIndex("sound/interface/secret_area.wav"));
	Inventory_RunSpawner(ent, other);
	G_FreeEntity(ent);
}

void Cmd_Inventory_Drop(gentity_t *ent, iObjectList_t *inventory, int argnum, int dropCount){
	char arg[MAX_STRING_CHARS];
	trap_Argv(argnum, arg, sizeof(arg));
	int num = atoi(arg);
	if (num != 0 || (arg[0] == '0' && arg[1] == 0)){
		if (num < 0 || num >= inventory->count){
			Disp(ent, "^3Invalid index.");
			return;
		}
		if (inventory->objects[num]->fields.noDelete) {
			Disp(ent, "^3You cannot drop this item.");
			return;
		}

		InventoryDeleteConfirmData_t *data = (InventoryDeleteConfirmData_t *)G_Alloc(sizeof(InventoryDeleteConfirmData_t));
		data->obj = inventory->objects[num];
		gentity_t *dropped;

		if (!(dropped = G_Spawn())) {
			return;
		}

		VectorSet(dropped->r.mins, -4, -4, -1);
		VectorSet(dropped->r.maxs, 4, 4, 2);
		dropped->flags = FL_BOUNCE_HALF;
		dropped->s.eFlags &= ~(EF_NODRAW);
		dropped->s.modelindex = G_ModelIndex("models/items/datapad.glm");
		dropped->s.modelGhoul2 = 1;
		dropped->s.eType = ET_GENERAL;
		dropped->r.contents = CONTENTS_TRIGGER;
		dropped->clipmask = MASK_PLAYERSOLID;
		dropped->bounceCount = -5;
		dropped->physicsBounce = 0.50;		// items are bouncy
		dropped->parent = ent;
		dropped->classname = "dropped_iobject";
		dropped->touch = Touch_Iobject;
		dropped->flags |= FL_DROPPED_ITEM;
		dropped->count = 1;
		dropped->fullName = "keycard";
		iObjectDef_t *def = Inventory_GetObjectDef(dropped->fullName, NULL);

		if (!def)
			return;

		dropped->genericValue1 = (int)def;
		int currentItemCount = Inventory_DownCount_GetCount(data->obj);

		if (dropCount > 0) {
			if (dropCount > currentItemCount) {
				Inventory_DownCount_Add_InfoString(dropped, data->obj, currentItemCount);
			}
			else {
				Inventory_DownCount_Add_InfoString(dropped, data->obj, dropCount);
			}
		}
		else {
			Inventory_DownCount_Add_InfoString(dropped, data->obj, 0);
		}

		hurl(ent, dropped);
		Disp(ent, "^2Item dropped.");

		if (dropCount < currentItemCount) {
			Inventory_Quest_DownCount_Decrement(ent, data->obj, dropCount);
		}
		else {
			Inventory_DeleteObject(data->obj);
		}
	}
}

void Cmd_Inventory_f(gentity_t *ent, int iArg){
	iObjectList_t *inventory = INVDATA(ent->client->pers.Lmd.account);
	if(!inventory) {
		Disp(ent, "^3You must be logged in to use this.");
		return;
	}
	int argc = trap_Argc();
	char arg[MAX_STRING_CHARS];
	if(argc < 2){
		Disp(ent, "^3Usage: Inventory {list ^5[catagory/name] [offset]^3} {use ^2<index/name>^3} {destroy ^2<index/name>^3}");
		return;
	}
	trap_Argv(1, arg, sizeof(arg));
	if(Q_stricmp(arg, "list") == 0){
		Cmd_Inventory_List(ent, inventory, 2);
		return;
	}
	else if(Q_stricmp(arg, "use") == 0){
		Cmd_Inventory_Use(ent, inventory, 2);
		return;
	}
	else if(Q_stricmp(arg, "destroy") == 0){
		if(argc < 2){
			Disp(ent, "^3Usage: /inventory destroy ^2<index or type>");
			return;
		}
		Cmd_Inventory_Delete(ent, inventory, 2);
		return;
	}
	else if (Q_stricmp(arg, "drop") == 0){
		int dropCount = 0;

		if (argc > 3) {
			trap_Argv(3, arg, sizeof(arg));
			int num = atoi(arg);

			if (num != 0 || (arg[0] == '0' && arg[1] == 0)) {
				if (num > 0) {
					dropCount = num;
				}
			}
		}

		Cmd_Inventory_Drop(ent, inventory, 2, dropCount);
		return;
	}
	else
		Disp(ent, "^3Unknown argument");
}

void Inventory_ClearAccounts() {
	int i, j, max = Accounts_Count();
	iObjectList_t *inventory;
	Account_t *acc;
	for(i = 0; i < max; i++) {
		acc = Accounts_Get(i);
		inventory = INVDATA(acc);
		for(j = 0; j < inventory->count; j++) {
			Inventory_FreeObject(inventory->objects[j]);
		}
		inventory->count = 0;
		Lmd_Accounts_Modify(acc); //Ufo: was missing
	}
}

#ifdef LMD_EXPERIMENTAL

extern char *fieldNames[];

void GenerateInvDocs() {
	fileHandle_t f;
	trap_FS_FOpenFile("docs/inven.txt", &f, FS_WRITE);
	iObjectCatagory_t *catagory = catagories;
	iObjectDef_t **def;
	BG_field_t *field;
	char *str;
	str = "[b]Generic keys[/b]\n";
	trap_FS_Write(str, strlen(str), f);
	field = Item_Fields;
	while(field->name) {
		str = va("\t%s (%s)\n", field->name, fieldNames[field->type]);
		trap_FS_Write(str, strlen(str), f);
		field++;
	}
	trap_FS_Write("\n", 1, f);
	while(catagory->name) {
		str = va("[b]%s[/b]\n", catagory->name);
		trap_FS_Write(str, strlen(str), f);
		def = catagory->objects;
		while(*def) {
			str = va("\t[u]%s[/u]\n", (*def)->name);
			trap_FS_Write(str, strlen(str), f);
			field = (*def)->fields;
			while(field->name) {
				str = va("\t\t%s (%s)\n", field->name, fieldNames[field->type]);
				trap_FS_Write(str, strlen(str), f);
				field++;
			}
			trap_FS_Write("\n", 1, f);
			def++;
		}
		catagory++;
	}
	trap_FS_FCloseFile(f);
}

#endif
