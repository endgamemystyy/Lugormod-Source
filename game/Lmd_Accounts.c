
#include "g_local.h"

#include "Lmd_Accounts_Core.h"
#include "Lmd_Accounts_Stats.h"
#include "Lmd_Commands_Auths.h"
#include "Lmd_Professions.h"
#include "Lmd_Checksum.h"
#include "Lmd_Time.h"
#include "Lmd_IPs.h"
#include "Lmd_Commands_Core.h"
#include "Lmd_Console.h"
#include "Lmd_KeyPairs.h"
#include "Lmd_Accounts_CustomSkills.h"

Account_t *Accounts_New(char *username, char *name, char *password);
extern void NPC_SetAnim(gentity_t *ent, int setAnimParts, int anim, int setAnimFlags);
gentity_t *trySpawn(char *string);
void G_Knockdown(gentity_t *victim, int duration);

#define LEVEL_SCORE 11
#define MAX_LEVEL  40

void RenamePlayer( gentity_t *ent, char *Name) {
	char userinfo[MAX_INFO_STRING];
	int clientNum;
	if(!ent->client)
		return;
	if(!Name)
		return;
	if(strlen(Name) > MAX_NETNAME)
		return;
	clientNum = ent->s.number;
	trap_GetUserinfo(clientNum, userinfo, sizeof(userinfo));
	Info_SetValueForKey(userinfo, "name", Name);
	trap_SetUserinfo(clientNum, userinfo);	
	ent->client->pers.netnameTime = 0;//
	ClientUserinfoChanged( clientNum );
}

void checkLevelUp(gentity_t *ent){
	int myLevel = PlayerAcc_Prof_GetLevel(ent);
	int myTime = PlayerAcc_GetTime(ent);
	int levelUp = 0;
	if(myLevel && PlayerAcc_Prof_GetProfession(ent) != PROF_ADMIN){
		qboolean lvlup = qfalse;
		while(myLevel * LEVEL_SCORE <= PlayerAcc_GetScore(ent) && myLevel * myLevel * 3600 * 2 <= myTime && myLevel < MAX_LEVEL){
			levelUp++;
			myLevel++;
		}
		if(levelUp > 0){
			int flags = PlayerAcc_GetFlags(ent);
			if(flags & ACCFLAGS_NOPROFCRLOSS){
				PlayerAcc_AddFlags(ent, -ACCFLAGS_NOPROFCRLOSS);
				Disp(ent, "^3Your free profession change has been canceled.");
			}
			Disp(ent, va("You have gained %i level%s, you are now level %i.", levelUp, (levelUp>1)?"s":"", myLevel));
			PlayerAcc_Prof_SetLevel(ent, myLevel);
			WP_InitForcePowers(ent);
		}
	}
}

void creditsByTime(gentity_t *ent, int playtime);
void updatePlayer(gentity_t *ent){
	if(ent->client->pers.Lmd.account) {
		int prof = PlayerAcc_Prof_GetProfession(ent);
		if(prof == PROF_NONE || prof == PROF_BOT){
			checkLevelUp(ent);
		}
	}

	//Ufo: admins often perform their tasks while spectating
	if (Auths_PlayerHasAdmin(ent) || (ent->client->sess.sessionTeam != TEAM_SPECTATOR && !(ent->client->pers.Lmd.jailTime > level.time))){
		int playtime = (int)((level.time - ent->client->pers.Lmd.playTime) / 1000);
		// Add money by time
		if ((g_cmdDisable.integer & 256) && !Auths_PlayerHasAdmin(ent)){
			creditsByTime(ent, playtime);
		}
		PlayerAcc_SetTime(ent, PlayerAcc_GetTime(ent) + playtime);
	}

	ent->client->pers.Lmd.playTime = level.time;
}


void Lmd_Accounts_LogAction(gentity_t *player, Account_t *acc, char* action) {
	char* username = Accounts_GetUsername(acc);
	G_LogPrintf("Player \"%s\" (%s) has %s.\n", player->client->pers.netname, username, action);
	Auths_DispAdmins(va(CT_B"Player \'"CT_N"%s"CT_B"\' ("CT_B_V"%s"CT_B") has %s.", player->client->pers.netname, username, action));
}


typedef struct hiscore_s{
	Account_t *acc;
	int val;
	//struct hiscore_s *next;
}hiscore_t;

void HiScore (gentity_t *ent, int field){
	hiscore_t list[10];
	memset(list, 0, sizeof(list));
	int num_accounts = Accounts_Count();
	int i, i2;
	int valNew;
	int shiftIndex;
	Account_t *acc;
	for(i = 0;i<10;i++){
		list[i].val = -Q3_INFINITE;
	}
	for(i = 0; i < num_accounts; i++){
		acc = Accounts_Get(i);
		if(Accounts_Prof_GetProfession(acc) == PROF_ADMIN)
			continue;
		switch(field){
			case 1:
				valNew = Accounts_GetTime(acc);
				break;
			case 2:
				valNew = Accounts_GetCredits(acc);
				break;
			case 3:
				valNew = Accounts_Prof_GetLevel(acc);
				break;
			case 4:
				valNew = Accounts_Stats_GetKills(acc);
				break;
			case 5:
				valNew = Accounts_Stats_GetStashes(acc);
				break;
			default:
				valNew = Accounts_GetScore(acc);
				break;
		}
		shiftIndex = -1;
		for(i2 = 0;i2 < 10;i2++){
			if(list[i2].val < valNew){
				shiftIndex = i2;
				break;
			}
		}
		if(shiftIndex < 0)
			continue;
		if(shiftIndex < 9){
			//shift list array members from shiftIndex to 9 down.
			memcpy((list + shiftIndex + 1), list + shiftIndex, ((10 - shiftIndex) - 1) * sizeof(list[0]));
		}
		list[shiftIndex].acc = acc;
		list[shiftIndex].val = valNew;
	}

	char dstr[MAX_STRING_CHARS];
	char name[MAX_NAME_LENGTH];
	char *ptr;

	for(i = 0;i<10;i++){
		if(!list[i].acc)
			break;
		ptr = Accounts_GetName(list[i].acc);
		if(!ptr)
			Q_strncpyz(name, "^3<^1ERROR^3>", sizeof(name));
		else
			Q_strncpyz(name, ptr, sizeof(name));
		Q_CleanStrC(name); //needed so credit amount lines up
		Q_strncpyz(dstr, va("^3%-32s", name), sizeof(dstr));
		switch(field){
			case 1:
				Q_strcat(dstr, sizeof(dstr), va(" ^2%6i^3:^2%02i", 
					Accounts_GetTime(list[i].acc)/3600 ,(list[i].val/60)%60));
				break;
			default:
				Q_strcat(dstr, sizeof(dstr), va("  ^2%8i", list[i].val));
				break;
		}
		Disp(ent, dstr);
	}

}

int Jedi_GetAccSide(Account_t *acc);
void GetStats(gentity_t *ent, Account_t *acc){
	int prof, time, lvl, authrank;
	char *c;
	char *authlist;
	char lastLogin[MAX_STRING_CHARS];

	char *secCode;

	if(!acc)
		return;

	prof = Accounts_Prof_GetProfession(acc);
	time = Accounts_GetTime(acc);
	lvl = Accounts_Prof_GetLevel(acc);
	authlist = Auths_QuickAuthList(acc);
	authrank = Auths_GetRank(acc);
	Time_ToHumanString(Accounts_GetLastLogin(acc), lastLogin, sizeof(lastLogin));

	secCode = Accounts_GetSeccode(acc);

	Disp(ent, va(
		"^3Id:            ^2%i\n"
		"^3Name:          ^7%s\n"
		"^3Username:      ^2%s\n"
		"^3Security code: ^2%s\n"
		"^3Credits:       ^2%i\n"
		"^3Time:          ^2%i^3:^2%02i\n"
		"^3Level:         ^2%i\n"
		"^3Score:         ^2%i",
		Accounts_GetId(acc),
		Accounts_GetName(acc),
		Accounts_GetUsername(acc),
		(secCode != NULL) ? secCode : "^1<none>",
		Accounts_GetCredits(acc), 
		time/3600, (time/60)%60, 
		lvl, Accounts_GetScore(acc)));
	if(prof == PROF_NONE)
		c = "^2None";
	else if(prof == PROF_ADMIN)
		c = "^6God";
	else if(prof == PROF_BOT)
		c = "^1Bot";
	else if(prof == PROF_JEDI)
	{
		switch(Jedi_GetAccSide(acc))
		{
		case 0:
		default:
			//Ufo:
			c = "^5Force user";
			break;
		case 1:
			c = "^4Jedi";
			break;
		case 2:
			c = "^1Sith";
			break;
		}
	}
	else if(prof == PROF_MERC)
		c = "^3Merc";
	if(prof == PROF_NONE)
		Disp(ent, va("^3Next level up: ^2%i^3 points and ^2%i^3 hours", (lvl * LEVEL_SCORE), (lvl * lvl * 2)));
	Disp(ent, va(
		"^3Logins:        ^2%i\n"
		"^3Last login:    ^2%s\n"
		"^3Kills:         ^2%i\n"
		"^3Deaths:        ^2%i\n"
		"^3Stashes:       ^2%i\n"
		"^3Total duels    ^2%i\n"
		"^3Duels won:     ^2%i\n"
		"^3Total shots:   ^2%i\n"
		"^3Shots hit:     ^2%i\n"
		"^3Profession:    ^2%s",
		Accounts_GetLogins(acc),
		lastLogin,
		Accounts_Stats_GetKills(acc),
		Accounts_Stats_GetDeaths(acc),
		Accounts_Stats_GetStashes(acc), Accounts_Stats_GetDuels(acc), Accounts_Stats_GetDuelsWon(acc),
		Accounts_Stats_GetShots(acc), Accounts_Stats_GetHits(acc), c));

	if(authlist[0])
		Disp(ent, va("^3Authfile(s):   ^2%s", authlist));
	if(authrank)
		Disp(ent, va("^3Authrank:      ^2%i", authrank));
	if(!ent) {
		IP_t ip;
		Accounts_GetLastIp(acc, ip);
		Disp(ent, va("^3Last IP:       ^2%s", Lmd_IPs_IPToString(ip)));
	}
}

void listAdmins(gentity_t *ent){
	int i, count = 0;
	int max = Accounts_Count();
	Account_t *acc;
	for(i = 0; i < max; i++) {
		acc = Accounts_Get(i);
		if(Auths_AccHasAdmin(acc)){
			Disp(ent, va("^3%-30s ^2%i %s", Accounts_GetUsername(acc), Auths_GetRank(acc), Auths_QuickAuthList(acc)));
			count++;
		}
	}
	Disp (ent, va(CT_V"%i"CT_B" nicks.", count));
}

void clearLevels(void){
	int i, max = Accounts_Count();
	Account_t *acc;
	for(i = 0; i < max; i++) {
		acc = Accounts_Get(i);
		if(Auths_AccHasAdmin(acc))
			continue;
		Accounts_Prof_SetLevel(acc, 0);
	}
}

void clearCash(void){
	int i, max = Accounts_Count();
	Account_t *acc;
	for(i = 0; i < max; i++) {
		acc = Accounts_Get(i);
		if(Auths_AccHasAdmin(acc))
			continue;
		Accounts_SetCredits(acc, 0);
	}
}

void clearScore(void){
	int i, max = Accounts_Count();
	Account_t *acc;
	for(i = 0; i < max; i++) {
		acc = Accounts_Get(i);
		if(Auths_AccHasAdmin(acc))
			continue;
		Accounts_SetScore(acc, 0);
	}
}

void clearTime(void){
	int i, max = Accounts_Count();
	Account_t *acc;
	for(i = 0; i < max; i++) {
		acc = Accounts_Get(i);
		if(Auths_AccHasAdmin(acc))
			continue;
		Accounts_SetTime(acc, 0);
	}
}

void clearAccounts(void){
	int i, max = Accounts_Count();
	Account_t *acc;
	for(i = max - 1; i >= 0; i--) {
		acc = Accounts_Get(i);
		if(Auths_AccHasAdmin(acc))
			continue;
		Accounts_Delete(acc);
	}
}

void clearSkills(void){
	int i, max = Accounts_Count();
	Account_t *acc;
	for(i = 0; i < max; i++) {
		acc = Accounts_Get(i);
		if(Auths_AccHasAdmin(acc))
			continue;
		Accounts_Prof_ClearData(acc);
	}
}

int recallDroppedCredits(gentity_t *ent);
void Inventory_Player_Logout(gentity_t *player);
void Lmd_Accounts_Player_Logout(gentity_t *ent){
	Account_t *acc = ent->client->pers.Lmd.account;

	if(ent->client->pers.Lmd.jailTime > level.time)
		return;
	
	if(!acc){
		if(Auths_PlayerHasTempAdmin(ent)){
			Auths_RemoveTempAdmin(ent, NULL);
			Disp(ent, "^3You have logged out of admin.");
		}
		return;
	}

	recallDroppedCredits(ent);
	Inventory_Player_Logout(ent);
	updatePlayer(ent);

	ent->client->pers.Lmd.account = 0;
	ent->client->sess.Lmd.id = 0;

	Lmd_Accounts_LogAction(ent, acc, "logged out");

	RenamePlayer(ent, "Padawan");

	SetTeam(ent, "s");
	Disp(ent, "^3You have logged out.");
}

extern vmCvar_t lmd_startingcr;
int accountLiveTime(int level);
void Lmd_IPs_AddName(IP_t ip, char *name);
void Lmd_IPs_AddAccount(IP_t ip, int id);
void Inventory_Player_Login(gentity_t *player);
void PlayerGuide_Player_Login(gentity_t *ent);
qboolean Lmd_Accounts_Player_Login(gentity_t *ent, Account_t *acc){
	char uinfo[MAX_INFO_STRING];
	int i;

	for (i = 0; i < MAX_CLIENTS;i++){
		if (g_entities[i].inuse && i != ent->s.number && g_entities[i].client->pers.Lmd.account == acc) {
			return qfalse;
		}
	}

	if(!Accounts_Prof_GetLevel(acc)){
		Accounts_Prof_SetProfession(acc, PROF_NONE);
		Accounts_Prof_SetLevel(acc, 1);
		Accounts_SetScore(acc, 10);
		if(lmd_startingcr.integer > 0){
			Accounts_SetCredits(acc, lmd_startingcr.integer);
			Accounts_AddFlags(acc, ACCFLAGS_NOPROFCRLOSS);
		}
	}

	

	Accounts_SetLogins(acc, Accounts_GetLogins(acc) + 1);

	ent->client->pers.Lmd.playTime = level.time;
	ent->client->pers.Lmd.account = acc;
	ent->client->sess.Lmd.id = Accounts_GetId(acc);

	Accounts_SetLastIp(acc, ent->client->sess.Lmd.ip);

	Accounts_SetLastLogin(acc, Time_Now());
	
	int prof = Accounts_Prof_GetProfession(acc);
	if (prof != PROF_NONE && prof != PROF_BOT){
		ent->flags &= ~FL_GODMODE;
		if (ent->client->sess.sessionTeam != TEAM_SPECTATOR){
			//ent->client->ps.persistant[PERS_SCORE]++;
			ent->client->ps.stats[STAT_HEALTH] = ent->health = 0;
			ClientSpawn(ent); //Ufo: why die if respawn is sufficient
			//player_die(ent, ent, ent, 100000, MOD_SUICIDE);
		}
	}

	WP_InitForcePowers(ent);

	char *name = Accounts_GetName(acc);
	trap_GetUserinfo(ent->s.number, uinfo, sizeof(uinfo));
	Q_strncpyz(ent->client->pers.netname, name, sizeof(ent->client->pers.netname));
	Lmd_IPs_AddName(ent->client->sess.Lmd.ip, ent->client->pers.netname);
	Lmd_IPs_AddAccount(ent->client->sess.Lmd.ip, Accounts_GetId(acc));
	Info_SetValueForKey(uinfo, "name", name);
	trap_SetUserinfo(ent->s.number, uinfo);
	ClientUserinfoChanged(ent->s.number);

	Inventory_Player_Login(ent);
	PlayerGuide_Player_Login(ent);


	//Ufo: we don't want admins without proper flag to start with admin chatmode
	if (Auths_AccHasAdmin(acc) && Auths_AccHasAuthFlag(acc, AUTH_ADMINCHAT)) {
		ent->client->pers.Lmd.chatMode[1] = SAY_ADMINS;
	}

	
	Lmd_Accounts_LogAction(ent, acc, "logged in");

	return qtrue;
}

extern vmCvar_t lmd_loginsecurity;

void Lmd_Accounts_Player_CreateSecurityCode(gentity_t *ent, Account_t *acc) {
	char *code = Accounts_NewSeccode(acc);
	int flags = Accounts_GetFlags(acc);
	Disp(ent, va(
		"\n"
		"^3----IMPORTANT----\n"
		"^3Your new security code is:\n"
		"\n"
		"^2%s\n"
		"\n"
		"^3If you have security codes enabled, you will be asked for this code if you log in from a new ip.\n"
		"^3------------------\n"
		"\n",
		code));
	Disp(ent, va(
		"^3Your security code usage is currently %s^3.\n"
		"^3As Lugormod can not guarentee your password is safe, it is recommended you keep this on.\n"
		"^3You may enable or disable your security code by using the \'^2/seccode^3\' command.\n"
		"^3You may see your security code at any time by using the \'^2stats^3\' command.\n",
		(flags & ACCFLAGS_NOSECCODE) ? "^1disabled" : "^2enabled"));

	trap_SendServerCommand(ent->s.number, va("cp \""
		"^3Your security code is ^2%s^3\n"
		"^3This code is to ensure your account is not compromized if someone steals your password.\n"
		"^3More information has been written to the console."
		"\"", code));
}

qboolean Lmd_Accounts_Player_TryLogin(gentity_t *ent, char *username, char *pass, char *secCode){
	unsigned long int chk = Checksum(pass);
	int i;
	Account_t *acc;
	char *accSec;
	IP_t lastIP;
	int flags; 

	if (!ent || !ent->client || !pass || duelInProgress(&ent->client->ps)){
		Disp(ent, "^3You cannot login at this time.");
		return qfalse;
	}

	if(ent->client->pers.Lmd.account){
		Disp(ent, "^3You are currently logged in.");
		return qfalse;
	}

	acc = Accounts_GetByUsername(username);
	if (!acc) {
		Disp(ent, "^1Invalid username or password.");
		return qfalse;
	}

	if (Accounts_GetPassword(acc) != chk) {
		Disp(ent, "^1Invalid username or password.");
		return qfalse;
	}

	flags = Accounts_GetFlags(acc);

	accSec = Accounts_GetSeccode(acc);
	Accounts_GetLastIp(acc, lastIP);
	if (!(flags & ACCFLAGS_NOSECCODE)) {
		if (accSec && lastIP[0] && Lmd_IPs_CompareIP(ent->client->sess.Lmd.ip, lastIP) == qfalse) {
			if(!secCode[0]) {
				Disp(ent, "^3Your ip does no match the last logged ip.  You must enter your security code to login.");
				return qfalse;
			}
			else if(stricmp(accSec, secCode) != 0) {
				Disp(ent, "^3Invalid security code.");
				return qfalse;
			}
		}
	}

	for (i = 0; i < MAX_CLIENTS;i++){
		if (g_entities[i].inuse && i != ent->s.number && g_entities[i].client->pers.Lmd.account == acc){
			Disp(ent, "^1Someone is using that account.");
			trap_SendServerCommand(i, va("cp \"^1WARNING\n^3Client ^1%i^3 (name ^2\'%s\'^3) is trying\n^3to log on to your account.\"",
				ent->s.number, ent->client->pers.netname));
			Com_Printf("info: %s (%i) is trying to log on to %s's account.\n", ent->client->pers.netname, ent->s.number, g_entities[i].client->pers.netname);
			return qfalse;
		}
	}

	if(Lmd_Accounts_Player_Login(ent, acc)) {
		if (!Auths_AccHasAdmin(acc)){
			Disp(ent, va("^2Login successful.\n^3The account will expire in ^2%i^3 days if you do not login before that.", accountLiveTime(Accounts_Prof_GetLevel(acc))));
		}
		else{
			Disp(ent, va("^2Login successful. \n^3You are authenticated with ^2%s^3.", Auths_QuickAuthList(acc)));
		}
	}
	else {
		Disp(ent, "^1Could not log in.");
	}

	if (lmd_loginsecurity.integer == 1 || (lmd_loginsecurity.integer == 2 && Auths_GetRank(acc) > 0)) {
		if (flags & ACCFLAGS_NOSECCODE || accSec == NULL) {
			Disp(ent, "^3This server requires you to have a security code.  Your security code has been enabled.\n");
			Accounts_AddFlags(acc, -ACCFLAGS_NOSECCODE);
			if(!accSec) {
				Lmd_Accounts_Player_CreateSecurityCode(ent, acc);
			}
		}
	}

	
	return qtrue;
}

qboolean IsValidPlayerName(char *name, gentity_t *ent, qboolean isRegister, char **reason);

qboolean IsValidUsername(char *username, char **reason) {
	int i = 0, c = 0;
	char stripped[MAX_NETNAME];

	*reason = NULL;

	Q_strncpyz(stripped, username, sizeof(stripped));
	Q_CleanStr2(username);
	if (Q_stricmp(username, stripped) != 0 || strchr(username, ' ')){
		*reason = "^3Username must only contain numbers and letters.";
		return qfalse;
	}
	while (username[i]){
		if ((username[i] >= 'A' && username[i] <= 'Z') || (username[i] >= 'a' && username[i] <= 'z'))
			c++;
		i++;
	}
	if (c < 2){
		*reason = "^3Your username must have at least one non-number character.";
		return qfalse;
	}
	else if (Accounts_GetByUsername(username)){
		*reason = "^3That username is already taken.";
		return qfalse;
	}


	return qtrue;
}

qboolean Lmd_Accounts_Player_Register(gentity_t *ent, char *username, char *passwd){
	char stripped[MAX_NETNAME];
	char *failReason;
	int i = 0, c = 0;

	Account_t *account;

	if(ent->client->pers.Lmd.account){
		Disp(ent, "^3You are already registered.");
		return qfalse;
	}

	if(!IsValidPlayerName(ent->client->pers.netname, ent, qtrue, &failReason)){
		Disp(ent, va("^1You cannot register using your current name.\n^3%s", failReason));
		return qfalse;
	}

	if (!IsValidUsername(username, &failReason)) {
		Disp(ent, va("^1You cannot register this username.\n^3%s", failReason));
		return qfalse;
	}

	
	if(Accounts_GetByName(ent->client->pers.netname)){
		Disp(ent, "^1You cannot register using your current name.\n^3That name is already taken.");
		return qfalse;
	}

	account = Accounts_New(username, ent->client->pers.netname, passwd);
	if(!account){
		Disp(ent, "^1Could not create a new account.  There may be a problem with the server.");
		return qfalse;
	}

	Disp(ent, va(
		"%s^2 is now registered. ^3Username: ^2%s\n"
		"^3From now on, type '^2\\login %s [password]^3' to use your account.\n"
		"^1Be sure to remember your username and password, or you will not be able to log back in and use the name you have registered.", 
		ent->client->pers.netname, username, username));

	ClientUserinfoChanged(ent->s.number);

	Lmd_Accounts_LogAction(ent, account, "registered");


	if (lmd_loginsecurity.integer == 0) {
		Accounts_AddFlags(account, ACCFLAGS_NOSECCODE);
	}
	else if (lmd_loginsecurity.integer == 1) {
		// Required for all.
		Lmd_Accounts_Player_CreateSecurityCode(ent, account);
	}

	Lmd_Accounts_Player_Login(ent, account);

	return qtrue;
}

qboolean Lmd_Accounts_Bot_Login(gentity_t *ent){
	int i;
	Account_t *acc = Accounts_GetByName(ent->client->pers.netname);

	//Check for a bot using a regular player's name
	if(acc && Accounts_Prof_GetProfession(acc) != PROF_BOT)
		return qfalse; //return false, we cannot create an account.

	for (i = 0; i < MAX_CLIENTS;i++){
		if(i == ent->s.number || !g_entities[i].inuse || !g_entities[i].client)
			continue;
		//no login if sharing an account.
		if(acc && g_entities[i].client->pers.Lmd.account == acc)
			return qfalse;
		//no login if sharing a name.
		if(!Q_stricmpname(g_entities[i].client->pers.netname, ent->client->pers.netname))
			return qfalse;
	}

	if(!acc) {
		char newstr[MAX_STRING_CHARS];
		Q_strncpyz(newstr, ent->client->pers.netname, sizeof(newstr));
		Q_CleanStr2(newstr);
		acc = Accounts_New(newstr, ent->client->pers.netname, NULL);
		if(!acc)
			return qfalse;
	}

	if (Accounts_GetScore(acc) < 10) //Bots always start with at least 10
		Accounts_SetScore(acc, 10);

	return Lmd_Accounts_Player_Login(ent, acc);
}

qboolean IsValidName(char *name) {
	char cmpName[MAX_NETNAME];
	int c = 0;
	int i = 0;

	if(!name || !name[0])
		return qfalse;

	if(strlen(name) >= MAX_NETNAME)
		return qfalse;

	if(strchr(name, '\n') != NULL || strchr(name, '\r') != NULL)
		return qfalse;

	Q_strncpyz(cmpName, name, sizeof(cmpName));
	Q_CleanStr2(cmpName);

	while (cmpName[i] && i < MAX_NAME_LENGTH){
		if (Q_isprint(cmpName[i]) == qfalse) {
			return qfalse;
		}

		if ((cmpName[i] >= 'a' && cmpName[i] <= 'z') || (cmpName[i] >= 'A' && cmpName[i] <= 'Z')){
			c++;
		}

		// Must be a valid infostring
		if (cmpName[i] == '\"' || cmpName[i] == ';' || cmpName[i] == '\\') {
			return qfalse;
		}

		i++;
	}

	if(c < 3){
		return qfalse;
	}

	return qtrue;
}

qboolean IsValidPlayerName(char *name, gentity_t *ent, qboolean isRegister, char** reason) {
	char cmpName[MAX_NETNAME];

	*reason = NULL;

	if (ent && ent->r.svFlags & SVF_BOT) {
		return qtrue;
	}

	if (IsValidName(name) == qfalse) {
		return qfalse;
	}

	if (Q_stricmpname(name, "Padawan") == 0){
		if (isRegister || (ent && ent->client->pers.Lmd.account)) {
			*reason = "Registered accounts cannot use the name \'Padawan\'";
			return qfalse;
		}

		return qtrue; //just deal with it
	}

	Q_strncpyz(cmpName, name, sizeof(cmpName));
	Q_CleanStr2(cmpName);

	if (!((g_cmdDisable.integer * 2) & (1 << 1))) {
		Account_t *acc = Accounts_GetByName(cmpName);
		if (acc || G_GetBotInfoByName(cmpName)){
			if (!ent || ent->client->pers.Lmd.account != acc) {
				*reason = "This name is already in use";
				return qfalse;
			}
		}
	}

	return qtrue;
}

qboolean IsValidPlayerName(char *name, gentity_t *ent, qboolean isRegister){
	char *unused;
	return IsValidPlayerName(name, ent, isRegister, &unused);
}

void Cmd_ChPasswd_f (gentity_t *ent, int iArg){
	if (trap_Argc() < 2) {
		Disp (ent, "^3Usage: chpasswd ^2<password>");
		return;
	}
	char *passwd = ConcatArgs(1);
	PlayerAcc_SetPassword(ent, passwd);
	Disp(ent,"Password changed.");
}

int ForceDash(gentity_t* ent)
{
	vec3_t  angles, dir;
	VectorSet(angles, NULL, ent->client->ps.viewangles[YAW], NULL);
	if (ent->client->pers.cmd.rightmove > 0)
	{
		AngleVectors(angles, NULL, dir, NULL);
	}
	if (ent->client->pers.cmd.rightmove < 0)
	{
		AngleVectors(angles, NULL, dir, NULL);
		VectorScale(dir, -1, dir);
	}
	if (ent->client->pers.cmd.forwardmove > 0)
	{
		AngleVectors(angles, dir, NULL, NULL);
	}
	if (ent->client->pers.cmd.forwardmove < 0)
	{
		AngleVectors(angles, dir, NULL, NULL);
		VectorScale(dir, -1, dir);
	}
	if (ent->client->pers.cmd.forwardmove == 0 && ent->client->pers.cmd.rightmove == 0)
	{
		if (ent->client->ps.saberMove <= 3)
			AngleVectors(angles, NULL, NULL, dir);
		else
			AngleVectors(angles, dir, NULL, NULL);
	}
	
	G_Throw(ent, dir, 192);
	ent->client->ps.velocity[2] += 128;

	if (ent->client->ps.saberMove <= 3)
		NPC_SetAnim(ent, SETANIM_BOTH, 1151, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);

	trap_SendServerCommand(ent->s.number, "print \"^2You have dashed using the force.\n\"");
	return 1;
}


int BattleMeditation(gentity_t* ent)
{
	if (ent->client->ps.legsAnim != BOTH_MEDITATE || ent->client->ps.torsoAnim != BOTH_MEDITATE)
	{
		trap_SendServerCommand(ent->s.number, "print \"^1You must be meditating to use this skill.\n\"");
		return 0;
	}
	trap_SendServerCommand(ent->s.number, "print \"^2Battle Meditation will be soon used on your friends, if you continue to meditate.\n\"");
	p_playerInfo[ent->s.number].battlemedtimer = 0;
	p_playerInfo[ent->s.number].battlemed = 1;
	return 1;
}

void DestructionMissile(gentity_t* ent)
{
	vec3_t  start, angles, forward;
	gentity_t* missile;
	VectorCopy(ent->client->ps.viewangles, angles);
	AngleVectors(angles, forward, NULL, NULL);
	VectorMA(ent->r.currentOrigin, 32, forward, start);
	start[2] += ent->client->ps.viewheight;
	missile = CreateMissile(start, forward, 1000, 30000, ent, qfalse);
	missile->classname = "conc_proj";
	missile->s.weapon = WP_CONCUSSION;
	missile->mass = 10;
	VectorSet(missile->r.maxs, 3, 3, 3);
	VectorScale(missile->r.maxs, -1, missile->r.mins);
	missile->damage = 40;
	missile->splashDamage = 75;
	missile->splashRadius = 80;
	missile->methodOfDeath = MOD_FORCE_DARK;
	missile->splashMethodOfDeath = MOD_FORCE_DARK;
	missile->clipmask = MASK_SHOT | CONTENTS_LIGHTSABER;
	missile->bounceCount = 0;
}

int ForceDestruction(gentity_t* ent)
{
	NPC_SetAnim(ent, SETANIM_TORSO, BOTH_FORCEPUSH, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	DestructionMissile(ent);
	return 1;
}

int SummonBacta(gentity_t* ent)
{
	if (ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_MEDPAC) && ent->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_MEDPAC_BIG))
	{
		trap_SendServerCommand(ent->s.number, "print \"^1You already have both bactas in your inventory.\n\"");
		return 0;
	}
	NPC_SetAnim(ent, SETANIM_BOTH, 1375, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	G_PlayEffectID(G_EffectIndex("ships/scrape_sparks"), ent->client->ps.origin, ent->client->ps.viewangles);
	G_PlayEffectID(G_EffectIndex("ships/scrape_sparks"), ent->client->ps.origin, ent->client->ps.viewangles);
	G_PlayEffectID(G_EffectIndex("ships/scrape_sparks"), ent->client->ps.origin, ent->client->ps.viewangles);
	G_PlayEffectID(G_EffectIndex("ships/scrape_sparks"), ent->client->ps.origin, ent->client->ps.viewangles);
	G_PlayEffectID(G_EffectIndex("ships/scrape_sparks"), ent->client->ps.origin, ent->client->ps.viewangles);
	ent->client->ps.stats[STAT_HOLDABLE_ITEMS] |= (1 << HI_MEDPAC);
	ent->client->ps.stats[STAT_HOLDABLE_ITEMS] |= (1 << HI_MEDPAC_BIG);
	trap_SendServerCommand(ent->s.number, "print \"^2Your bactas have been summoned.\n\"");
	return 1;
}

void BombThink(gentity_t* self)
{
	gentity_t* ent = GetEnt(self->genericValue15);
	G_PlayEffectID(G_EffectIndex("explosions/wedge_explosion1"), self->s.origin, self->s.angles);
	G_RadiusDamage(self->s.origin, ent, 100000, 512, ent, NULL, MOD_THERMAL);
	G_FreeEntity(self);
	return;
}

void TimeBombThink(gentity_t* self)
{
	gentity_t* ent = GetEnt(self->genericValue15);
	G_PlayEffectID(G_EffectIndex("explosions/wedge_explosion1"), self->s.origin, self->s.angles);
	G_RadiusDamage(self->s.origin, ent, 100, 384, ent, NULL, MOD_THERMAL);
	G_FreeEntity(self);
	return;
}

void cliAimTrace(trace_t* tr, gentity_t* ent, int distance = 8192, vec3_t mins = 0, vec3_t maxs = 0)
{
	vec3_t location, forward, storage;
	AngleVectors(ent->client->ps.viewangles, forward, NULL, NULL);
	VectorNormalize(forward);
	VectorCopy(ent->client->ps.origin, location);
	VectorCopy(ent->client->ps.origin, storage);
	storage[2] += ent->client->ps.viewheight;
	VectorMA(ent->client->ps.origin, distance, forward, location);
	trap_Trace(tr, storage, mins, maxs, location, ent->s.number, ent->clipmask);
}

int TimeBomb(gentity_t* ent)
{
	int l1 = 0;
	int l2 = 0;
	int l3 = 0;
	vec3_t mins;
	vec3_t maxs;
	int i = 0;
	gentity_t* loop;
	trace_t tr;
	VectorSet(mins, -13, -13, 0);
	VectorSet(maxs, 13, 13, 33);
	cliAimTrace(&tr, ent, 128, mins, maxs);
	if (tr.fraction < 1.0)
	{
		if (tr.contents & CONTENTS_BODY)
		{
			trap_SendServerCommand(ent->s.number, "print \"^1You cannot place a time bomb on a player.\n\"");
			return 0;
		}
		l1 = (int)tr.endpos[0];
		l2 = (int)tr.endpos[1];
		l3 = (int)tr.endpos[2];
		loop = trySpawn(va("classname,misc_model_breakable,spawnflags,1,model,map_objects/imperial/crate_banded,origin,%d %d %d", l1, l2, l3));
		if (!loop)
			return 0;
		NPC_SetAnim(ent, SETANIM_BOTH, 121, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		loop->think = TimeBombThink;
		loop->nextthink = level.time + 3000;
		loop->genericValue15 = ent->s.number;
		trap_SendServerCommand(ent->s.number, "print \"^3You have placed a time bomb that will go off in 3 seconds.\n\"");
		return 1;
	}
	else
	{
		trap_SendServerCommand(ent->s.number, "print \"^1Target is too far away!\n\"");
		return 0;
	}
}

int SkillDuration(gentity_t* ent, int* var, int duration)
{
	if (var != NULL) *var = level.time + duration;
	return duration;
}

int SlowRegen(gentity_t* ent)
{
	if (ent->client->ps.groundEntityNum == 1023)
	{
		trap_SendServerCommand(ent->s.number, "print \"^1You cannot use this skill while mid air.\n\"");
		return 0;
	}
	trace_t tr;
	cliAimTrace(&tr, ent, 256);
	if (tr.fraction < 1.0 && tr.entityNum < 32)
	{
		gentity_t* to = GetEnt(tr.entityNum);
		if (!to || !to->client || to->client->ps.duelInProgress || to->client->invulnerableTimer > level.time)
			return 0;
		if (Accounts_Prof_GetProfession(to->client->pers.Lmd.account) == 4)
		{
			trap_SendServerCommand(ent->s.number, "print \"^1This skill cannot be used on mercenaries.\n\"");
			return 0;
		}

		NPC_SetAnim(ent, SETANIM_BOTH, 1335, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		//p_playerInfo[tr.entityNum].regenSlowed = level.time + 10000;
		int duration = SkillDuration(to, &p_playerInfo[tr.entityNum].regenSlowed, 4000);
		for (int i = 0; i < NUM_FORCE_POWERS; i++)
			WP_ForcePowerStop(to, i);
		to->client->ps.fd.forcePower = 0;
		trap_SendServerCommand(ent->s.number, va("print \"^3Force regeneration of the target slowed down for %d seconds.\n\"", duration / 1000));
		trap_SendServerCommand(tr.entityNum, va("print \"^4Your force regeneration has been slowed down for %d seconds.\n\"", duration / 1000));
		G_PlayEffectID(G_EffectIndex("repeater/concussion"), GetEnt(tr.entityNum)->client->ps.origin, GetEnt(tr.entityNum)->client->ps.viewangles);
		return 1;
	}
	else
	{
		trap_SendServerCommand(ent->s.number, "print \"^1Target is too far away!.\n\"");
		return 0;
	}
}

int DisableJP(gentity_t* ent)
{
	if (ent->client->ps.groundEntityNum == 1023)
	{
		trap_SendServerCommand(ent->s.number, "print \"^1You cannot use this skill while mid air.\n\"");
		return 0;
	}
	int num = ent->s.number;
	trace_t tr;
	gentity_t* to;
	cliAimTrace(&tr, ent, 3072);
	if (tr.fraction < 1.0 && tr.entityNum < MAX_CLIENTS)
	{
		to = GetEnt(tr.entityNum);
		if (!to || !to->client || to->client->ps.duelInProgress || to->client->invulnerableTimer > level.time)
			return 0;
		if (!(to->client->ps.stats[STAT_HOLDABLE_ITEMS] & (1 << HI_JETPACK)))
		{
			trap_SendServerCommand(ent->s.number, "print \"^1This skill can only be used on players with a jetpack.\n\"");
			return 0;
		}
		NPC_SetAnim(ent, SETANIM_BOTH, 1374, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		if (to->client->jetPackOn == qtrue)
		{
			to->client->jetPackOn = qfalse;
			p_playerInfo[to->s.number].highFallDmg = level.time + 5000;
		}
		to->client->jetPackToggleTime = level.time + 5000;
		to->client->ps.otherKiller = ent->s.number;
		to->client->ps.otherKillerTime = level.time + 5000;
		to->client->ps.otherKillerDebounceTime = level.time + 100;
		trap_SendServerCommand(ent->s.number, "print \"^3Jetpack of the target successfully disabled.\n\"");
		trap_SendServerCommand(tr.entityNum, "print \"^4Your jetpack has been disabled.\n\"");
		G_PlayEffectID(G_EffectIndex("repeater/concussion"), GetEnt(tr.entityNum)->client->ps.origin, GetEnt(tr.entityNum)->client->ps.viewangles);
		return 1;
	}
	else
	{
		trap_SendServerCommand(ent->s.number, "print \"^1Target is too far away!\n\"");
		return 0;
	}
}


int ForceSever(gentity_t* ent)
{
	if (fabs(ent->client->ps.velocity[0]) > 50.0f || fabs(ent->client->ps.velocity[1]) > 50.0f || fabs(ent->client->ps.velocity[2]) > 50.0f)
	{
		trap_SendServerCommand(ent->s.number, "print \"^1You have to stand still when using this skill.\n\"");
		return 0;
	}
	trace_t tr;
	gentity_t* to;
	cliAimTrace(&tr, ent, 256);
	if (tr.fraction < 1.0 && tr.entityNum < 32)
	{
		to = GetEnt(tr.entityNum);
		if (!to || !to->client || to->client->ps.duelInProgress || to->client->invulnerableTimer > level.time)
			return 0;
		if (Accounts_Prof_GetProfession(to->client->pers.Lmd.account) == 4)
		{
			trap_SendServerCommand(ent->s.number, "print \"^1This skill cannot be used on mercenaries.\n\"");
			return 0;
		}
		NPC_SetAnim(ent, SETANIM_BOTH, 1351, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		int duration = SkillDuration(to, &p_playerInfo[tr.entityNum].severed, 4000);
		trap_SendServerCommand(ent->s.number, va("print \"^3The target has been severed from the force for %d seconds.\n\"", duration / 1000));
		trap_SendServerCommand(tr.entityNum, va("print \"^4You have been severed from the force for %d seconds.\n\"", duration / 1000));
		//p_playerInfo[tr.entityNum].severed = level.time + 10000;
		to->client->ps.electrifyTime = level.time + 4000;
		to->client->ps.weaponTime = 10;
		to->client->ps.quickerGetup = qtrue;
		to->client->ps.forceHandExtendTime = level.time + 10;
		for (int i = 0; i < NUM_FORCE_POWERS; i++)
			WP_ForcePowerStop(to, i);
		to->client->ps.fd.forcePower = 0;
		G_PlayEffectID(G_EffectIndex("repeater/concussion"), to->client->ps.origin, to->client->ps.viewangles);
		return 1;
	}
	else
	{
		trap_SendServerCommand(ent->s.number, "print \"^1Target is too far away!\n\"");
		return 0;
	}
}

void CloakStop(gentity_t* to)
{
	if (!to || !to->client)
		return;
	p_playerInfo[to->s.number].cloaked = 0;
	to->s.eFlags &= ~EF_NODRAW;
	to->client->ps.eFlags &= ~EF_NODRAW;
	to->r.svFlags &= ~SVF_NOCLIENT;
	to->flags &= ~FL_NOTARGET;
	G_PlayEffectID(G_EffectIndex("ships/heavydmg"), to->client->ps.origin, to->client->ps.viewangles);
	G_PlayEffectID(G_EffectIndex("ships/heavydmg"), to->client->ps.origin, to->client->ps.viewangles);
	G_PlayEffectID(G_EffectIndex("ships/heavydmg"), to->client->ps.origin, to->client->ps.viewangles);
	G_PlayEffectID(G_EffectIndex("ships/heavydmg"), to->client->ps.origin, to->client->ps.viewangles);
	G_PlayEffectID(G_EffectIndex("ships/heavydmg"), to->client->ps.origin, to->client->ps.viewangles);
}

int CloakSkill(gentity_t* ent)
{
	ent->s.eFlags |= EF_NODRAW;
	ent->client->ps.eFlags |= EF_NODRAW;
	ent->r.svFlags |= SVF_NOCLIENT;
	ent->flags |= FL_NOTARGET;
	p_playerInfo[ent->s.number].cloaked = level.time + 3000;
	G_PlayEffectID(G_EffectIndex("ships/heavydmg"), ent->client->ps.origin, ent->client->ps.viewangles);
	G_PlayEffectID(G_EffectIndex("ships/heavydmg"), ent->client->ps.origin, ent->client->ps.viewangles);
	G_PlayEffectID(G_EffectIndex("ships/heavydmg"), ent->client->ps.origin, ent->client->ps.viewangles);
	G_PlayEffectID(G_EffectIndex("ships/heavydmg"), ent->client->ps.origin, ent->client->ps.viewangles);
	G_PlayEffectID(G_EffectIndex("ships/heavydmg"), ent->client->ps.origin, ent->client->ps.viewangles);
	trap_SendServerCommand(ent->s.number, "print \"^2You have been cloaked for 4 seconds.\n\"");
	return 1;
}

int ForceSlow(gentity_t* ent)
{
	if (fabs(ent->client->ps.velocity[0]) > 50.0f || fabs(ent->client->ps.velocity[1]) > 50.0f || fabs(ent->client->ps.velocity[2]) > 50.0f)
	{
		trap_SendServerCommand(ent->s.number, "print \"^1You have to stand still when using this skill.\n\"");
		return 0;
	}
	trace_t tr;
	gentity_t* to;
	cliAimTrace(&tr, ent, 256);
	if (tr.fraction < 1.0 && tr.entityNum < 32)
	{
		to = GetEnt(tr.entityNum);
		if (!to || !to->client || to->client->ps.duelInProgress || to->client->invulnerableTimer > level.time)
			return 0;

		WP_ForcePowerStop(to, FP_SPEED);
		NPC_SetAnim(ent, SETANIM_BOTH, 1374, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		G_PlayEffectID(G_EffectIndex("repeater/concussion"), to->client->ps.origin, to->client->ps.viewangles);
		int duration = SkillDuration(to, NULL, 5000);
		to->client->Lmd.customSpeed.value = (int)(g_speed.integer * 0.75);
		to->client->Lmd.customSpeed.time = level.time + duration;
		//to->client->Lmd.customGravity.value = (int)(JASS_GETINTCVAR("g_gravity") * 100);
		//to->client->Lmd.customGravity.time = g_level->time + duration;
		trap_SendServerCommand(ent->s.number, va("print \"^3You have slowed the enemy down for %d seconds.\n\"", duration / 1000));
		trap_SendServerCommand(to->s.number, va("print \"^4You have been slowed down for %d seconds.\n\"", duration / 1000));
		return 1;
	}
	else
	{
		trap_SendServerCommand(ent->s.number, "print \"^1Target is too far away!\n\"");
		return 0;
	}
}

int MercSprint(gentity_t* ent)
{
	int duration = 5000;
	p_playerInfo[ent->s.number].adrenaline = level.time + duration;
	ent->client->Lmd.customSpeed.value = g_speed.integer * 1.2;
	ent->client->Lmd.customSpeed.time = level.time + duration;
	NPC_SetAnim(ent, SETANIM_BOTH, 1361, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	G_PlayEffectID(G_EffectIndex("concussion/explosion"), ent->client->ps.origin, ent->client->ps.viewangles);
	trap_SendServerCommand(ent->s.number, "print \"^2You have used a stimulant to increase your agility for 10 seconds.\n\"");
	return 1;
}

int PoisonDart(gentity_t* ent)
{
	if (fabs(ent->client->ps.velocity[0]) > 50.0f || fabs(ent->client->ps.velocity[1]) > 50.0f || fabs(ent->client->ps.velocity[2]) > 50.0f)
	{
		trap_SendServerCommand(ent->s.number, "print \"^1You have to stand still when using this skill.\n\"");
		return 0;
	}
	trace_t tr;
	cliAimTrace(&tr, ent, 1024);
	if (tr.fraction < 1.0 && tr.entityNum < 32)
	{
		gentity_t* to = GetEnt(tr.entityNum);
		if (!to || !to->client || to->client->ps.duelInProgress || to->client->invulnerableTimer > level.time || p_playerInfo[tr.entityNum].poisoned > level.time)
			return 0;
		int duration = SkillDuration(to, &p_playerInfo[tr.entityNum].poisoned, 4000);
		NPC_SetAnim(ent, SETANIM_BOTH, 1335, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		//p_playerInfo[tr.entityNum].poisoned = g_level->time + 10000;
		p_playerInfo[tr.entityNum].poisonedBy = ent->s.number;
		G_PlayEffectID(G_EffectIndex("noghri_stick/gas_cloud"), to->client->ps.origin, to->client->ps.viewangles);
		G_PlayEffectID(G_EffectIndex("noghri_stick/muzzle_flash"), ent->client->ps.origin, ent->client->ps.viewangles);
		trap_SendServerCommand(tr.entityNum, va("print \"^4You have been poisoned for %d seconds.\n\"", duration / 1000));
		return 1;
	}
	else
	{
		trap_SendServerCommand(ent->s.number, "print \"^1Target is too far away!\n\"");
		return 0;
	}
}

void Repulse(gentity_t* ent, qboolean pull, float knockback, float distance)
{
	if (ent->client->pers.Lmd.persistantFlags & 16)
	{
		ent->health = 0;
		ent->client->ps.stats[STAT_HEALTH] = 0;
		player_die(ent, ent, ent, 99999, MOD_SUICIDE);
		return;
	}
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		gentity_t* other = GetEnt(i);
		if (ent->s.number == i || !other || !other->inuse || other->client->sess.spectatorState || !ForcePowerUsableOn(ent, other, pull ? FP_PULL : FP_PUSH) || !CanDamage(other, ent->client->ps.origin))
			continue;
		float dist = Distance(ent->client->ps.origin, other->client->ps.origin);
		if (dist <= distance)
		{
			vec3_t entDir;
			VectorSubtract(other->client->ps.origin, ent->client->ps.origin, entDir);
			entDir[2] += 0.1f;
			VectorNormalize(entDir);
			if (pull)
			{
				VectorScale(entDir, -1, entDir);
				other->client->pushEffectTime = level.time + 1000;
				if (other->client->ps.torsoAnim != BOTH_PULLED_INAIR_B)
					NPC_SetAnim(other, SETANIM_BOTH, BOTH_PULLED_INAIR_B, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
				if (other->client->ps.legsAnim != BOTH_PULLED_INAIR_B)
					other->client->ps.velocity[2] += 16;
				other->client->ps.weaponTime = 1000;
				knockback /= (distance / dist);
			}
			G_Throw(other, entDir, knockback);
			if (!pull)
			{

				G_Knockdown(other, 2000);
				other->client->ps.velocity[2] += 1024;
				other->client->ps.otherKiller = ent->s.number;
				other->client->ps.otherKillerTime = level.time + 5000;
				other->client->ps.otherKillerDebounceTime = level.time + 100;
				/*
				if (!HasImmunity(other))
				{
					p_playerInfo[other->s.number].highFallDmg = g_level->time + 5000;
				}
				*/
				vec3_t pos;
				VectorCopy(ent->client->ps.origin, pos);
				pos[2] += 16;
				G_PlayEffectID(G_EffectIndex("concussion/explosion"), pos, ent->client->ps.viewangles);
			}
		}
	}
}

int RepulseBegin(gentity_t* ent)
{
	if (ent->client->ps.groundEntityNum == 1023)
	{
		trap_SendServerCommand(ent->s.number, "print \"^1You cannot use this skill while mid air.\n\"");
		return 0;
	}
	NPC_SetAnim(ent, SETANIM_BOTH, BOTH_FORCE_RAGE, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
	p_playerInfo[ent->s.number].repulse = level.time + 1000;
	return 1;
}

void ForceScreamDo(gentity_t* ent, float distance)
{
	if (ent->client->ps.groundEntityNum == 1023)
	{
		p_playerInfo[ent->s.number].screamtimer = 0;
		ent->client->Lmd.customSpeed.time = 0;
		ent->client->Lmd.customSpeed.value = 0;
		return;
	}
	if ((ent->client->pers.Lmd.persistantFlags & 16) || ent->client->isHacking)
	{
		ent->health = 0;
		ent->client->ps.stats[STAT_HEALTH] = 0;
		player_die(ent, ent, ent, 99999, MOD_SUICIDE);
		return;
	}
	vec3_t org;
	VectorCopy(ent->client->ps.origin, org);
	org[2] += 10;
	G_Sound(ent, CHAN_VOICE, G_SoundIndex(va("sound/chars/howler/howl_yell%d", irand(1, 5))));
	G_PlayEffectID(G_EffectIndex("effects/howler/sonic"), org, ent->client->ps.viewangles);
	NPC_SetAnim(ent, SETANIM_TORSO, BOTH_FORCE_RAGE, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD | SETANIM_FLAG_RESTART);
	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		gentity_t* target = GetEnt(i);
		if (!target || !target->client || !target->inuse || !ForcePowerUsableOn(ent, target, FP_PUSH) || target == ent || target->health <= 0 || target->client->sess.spectatorState || target->client->ps.duelInProgress || !CanDamage(target, ent->client->ps.origin))
			continue;
		float cur = Distance(ent->client->ps.origin, target->client->ps.origin);
		if (cur <= distance)
		{
			target->client->Lmd.customSpeed.value = g_speed.integer * 0.4;
			target->client->Lmd.customSpeed.time = level.time + 1000;
			target->client->ps.weaponTime = 1000;
			p_playerInfo[target->s.number].screamedon = level.time + 1000;
			if (p_playerInfo[target->s.number].sparktime >= level.time)
			{
				p_playerInfo[ent->s.number].sparktime = 0;
				ent->client->Lmd.customSpeed.time = 0;
				ent->client->Lmd.customSpeed.value = 0;
			}
			NPC_SetAnim(target, SETANIM_TORSO, 1380, SETANIM_FLAG_OVERRIDE | SETANIM_FLAG_HOLD);
		}
	}
}

int ForceScream(gentity_t* ent)
{
	if (ent->client->ps.groundEntityNum == 1023)
	{
		trap_SendServerCommand(ent->s.number, "print \"^1You cannot use this skill while mid air.\n\"");
		return 0;
	}

	int durationMs = 4000;
	p_playerInfo[ent->s.number].screamtimer = durationMs + level.time;
	ent->client->Lmd.customSpeed.value = g_speed.integer * 0.5;
	ent->client->Lmd.customSpeed.time = level.time + durationMs;
	return 1;
}

void GetStats (gentity_t *ent, Account_t *acc);
void Cmd_GetStats_f(gentity_t *ent, int iArg) {
	GetStats(ent, ent->client->pers.Lmd.account);
}

qboolean DropCredits(gentity_t *ent, int amount);
gentity_t* AimAnyTarget (gentity_t *ent, int length);
void Cmd_Credits_f (gentity_t *ent, int iArg){
	int ownCreds = PlayerAcc_GetCredits(ent), numCreds;
	char sArg[MAX_TOKEN_CHARS];

	if(!ent->client->pers.Lmd.account) {
		Disp(ent, "^3You must be logged in to use this.");
		return;
	}

	if(iArg == 0){
		Disp(ent, va("^3You have ^2CR %i^3.", ownCreds));
		return;
	}

	if(trap_Argc() < 2){
		//RoboPhred
		Disp(ent, "^3Not enough arguments.");
		return;
	}

	//RoboPhred
	if(ent->client->sess.spectatorState != SPECTATOR_NOT){
		Disp(ent, "^3You cannot do this while spectating.");
		return;
	}

	if(ent->health <= 0) {
		Disp(ent, "^3You must be alive to use this command.");
		return;
	}

	trap_Argv(1, sArg, sizeof(sArg));
	numCreds = atoi(sArg);
	if(numCreds < 1){
		Disp(ent, "^3Invalid value, credits must be >= 0");
		return;
	}

	if (numCreds > ownCreds){
		Disp(ent, va("^3You cannot afford ^1CR %i^3.",numCreds));
		return;
	}
	if(iArg == 1){
		gentity_t *payEnt = AimAnyTarget(ent, 64);
		if(!payEnt){
			Disp(ent, "^3Nothing targeted");
			return;
		}
		if(!payEnt->client){
			if (!(payEnt->flags&FL_PAY) && !payEnt->pay){
				Disp(ent, "^3This is not a payable entity");
				return;
			}
			else if(payEnt->flags & FL_INACTIVE || !payEnt->use){
				Disp(ent, "^3This entity is inactive");
				return;
			}
		}

		if(payEnt->client){
			if(!payEnt->client->pers.Lmd.account) {
				Disp(ent, "^3This player is not logged in.");
				return;
			}
			PlayerAcc_SetCredits(ent, ownCreds - numCreds);

			Disp(ent, va("^3You paid ^2CR %i^3 to ^7%s.", numCreds, payEnt->client->pers.netname));

			GiveCredits(payEnt, numCreds, va("from ^7%s", ent->client->pers.netname));
		}
		else{
			if(payEnt->pay)
			{
				if(payEnt->pay(payEnt, numCreds, ent) == qtrue)
					PlayerAcc_SetCredits(ent, ownCreds - numCreds);
			}
			else
			{
				payEnt->genericValue5 = numCreds;
				payEnt->use(payEnt, ent, ent);
				// Reset ownCreds, as the entity might manipulate credits
				ownCreds = PlayerAcc_GetCredits(ent);
				PlayerAcc_SetCredits(ent, ownCreds - numCreds);
			}
		}
	}
	else if(iArg == 3){
		if (ent->s.m_iVehicleNum || ent->client->sess.sessionTeam == TEAM_SPECTATOR || 
			ent->client->pers.Lmd.jailTime > level.time){
				return;
		}
		DropCredits(ent, numCreds);
	}
}

void Cmd_Customskills_f(gentity_t *ent, int iArg) {
	int argc = trap_Argc();
	char arg[MAX_STRING_CHARS];
	int offset = 0;

	if (argc > 1) {
		trap_Argv(1, arg, sizeof(arg));
		offset = atoi(arg);

		if (offset < 0) {
			offset = 0;
		}
	}

	KeyPairSet_t* set = PlayerAcc_Custom_GetSet(ent);
	char dstr[MAX_STRING_CHARS];
	Q_strncpyz(dstr, "^3Your current custom skills:\n", sizeof(dstr));
	int endIndex = 25 + offset;

	if (set != NULL) {
		for (int i = offset; i < set->count && i < endIndex; i++) {
			Q_strcat(dstr, sizeof(dstr), va("^3%d. ^5%-25s     ^6Level: %-5s", i + 1, set->pairs[i].key, set->pairs[i].value));
			int nextIndex = i + 1;

			if (nextIndex < set->count && nextIndex < endIndex) {
				if (i % 5 == 0) {
					Disp(ent, dstr);
					Q_strncpyz(dstr, "", sizeof(dstr));
				}
				else {
					Q_strcat(dstr, sizeof(dstr), "\n");
				}
			}
			else {
				Disp(ent, dstr);
			}
		}
	}
}

qboolean Lmd_Accounts_Player_Register(gentity_t *ent, char *username, char *passwd);
void Cmd_Register_f (gentity_t *ent, int iArg){
	char username[MAX_STRING_CHARS]; //size checked in registerAccount()
	char *passwd;
	if (trap_Argc() < 3) {
		Disp(ent, "^3You have to choose a username and password.\n\\^2register <username> <password>");
		return;
	}
	trap_Argv(1, username, sizeof(username));
	passwd = ConcatArgs(2);
	Lmd_Accounts_Player_Register(ent, username, passwd);
}

void Cmd_Seccode_f(gentity_t *ent, int iArg) {
	int argc = trap_Argc();

	if(!ent->client->pers.Lmd.account) {
		Disp(ent, "^3You must be logged in to use this.");
		return;
	}

	qboolean codeEnabled = (PlayerAcc_GetFlags(ent) & ACCFLAGS_NOSECCODE) == 0;

	if (argc < 2) {
		char *code = Accounts_GetSeccode(ent->client->pers.Lmd.account);
		Disp(ent, va(	"^3Your security code is ^2%s^3\n"
						"^3Your security code is %s^3.\n"
						"^3Use \'^2/seccode new^3\' to create a new security code, or \'^2/seccode toggle^3\' to %s it.",
						code,
						codeEnabled ? "^2Enabled" : "^1Disabled",
						codeEnabled ? "disable" : "enable"));
		return;
	}

	char cmd[MAX_STRING_CHARS];
	trap_Argv(1, cmd, sizeof(cmd));

	if (Q_stricmp(cmd, "new") == 0) {
		Lmd_Accounts_Player_CreateSecurityCode(ent, ent->client->pers.Lmd.account);
		return;
	}
	else if (Q_stricmp(cmd, "toggle") == 0) {
		if (codeEnabled) {
			if (lmd_loginsecurity.integer == 1) {
				Disp(ent, "^3You cannot disable your security code.  This server requires security codes for all users.");
				return;
			}
			else if (Auths_GetRank(ent->client->pers.Lmd.account) > 0 && lmd_loginsecurity.integer >= 2) {
				Disp(ent, "^3You cannot disable your security code.  This server requires security codes for all admins.");
				return;
			}

			Disp(ent, "^3Your security code is now ^1disabled^3.");
			PlayerAcc_AddFlags(ent, ACCFLAGS_NOSECCODE);
		}
		else {
			Disp(ent, "^3Your security code is now ^2enabled^3.");
			PlayerAcc_AddFlags(ent, -ACCFLAGS_NOSECCODE);
		}
	}
	else {
		Disp(ent, va("^3Unknown argument ^2%s", cmd));
	}
}

qboolean Lmd_Accounts_Player_TryLogin(gentity_t *ent, char *username, char *pass, char *secCode);
void Cmd_Login_f (gentity_t *ent, int iArg)
{
	char username[MAX_STRING_CHARS];
	char passwd[MAX_STRING_CHARS];
	char secCode[MAX_STRING_CHARS];
	if (trap_Argc() < 2) {
		Disp(ent, "^3Usage: login ^2<username> <password> [seccode]");
		return; 
	}
	trap_Argv(1, username, sizeof(username));
	trap_Argv(2, passwd, sizeof(passwd));
	trap_Argv(3, secCode, sizeof(secCode));

	Lmd_Accounts_Player_TryLogin(ent, username, passwd, secCode);
}

void Lmd_Accounts_Player_Logout(gentity_t *ent);
void Cmd_Logout_f(gentity_t *ent, int iArg){
	//Ufo: forbid logging out while dueling or frozen
	if (!ent || !ent->client || duelInProgress(&ent->client->ps) || ent->client->Lmd.flags & SNF_FREEZE){
		Disp(ent, "^3You cannot logout at this time.");
		return;
	}
	Lmd_Accounts_Player_Logout(ent);
}

void listWorthy(gentity_t *ent);
void Cmd_Worthy_f (gentity_t *ent, int iArg){
	listWorthy(ent);
}

void Cmd_Inventory_f(gentity_t *ent, int iArg);
void Cmd_Property_f(gentity_t *ent, int iArg);

cmdEntry_t accountCommandEntries[] = {
	{"chpasswd","Change the password for your account.", Cmd_ChPasswd_f, 0, qfalse, 1, 1, 0, 0},
	{"credits","Check your current wealth.", Cmd_Credits_f, 0, qfalse, 1, 128, ~(1 << GT_FFA), 0},
	{"customskills","List custom skills.", Cmd_Customskills_f, 0, qfalse, 1, 128, ~(1 << GT_FFA), 0},
	{"dropcr", "Drop credits.", Cmd_Credits_f, 3, qfalse, 1, 128, ~(1 << GT_FFA), 0},
	{"inventory","View and use items in your inventory.", Cmd_Inventory_f, 0, qfalse, 1, 1, ~(1 << GT_FFA), 0},
	{"login"," Login to use the name you registered with \\register.\nIf you change name when you are logged in, the new name will become the registered name.", Cmd_Login_f, 0, qfalse, 0, 1, 0, 0, qtrue},
	{"logout", "Logs out of your account.  If you are not in an account but have admin, you will loose admin status.", Cmd_Logout_f, 0, qfalse, 1, 0, 0, 0},
	{"pay", "Give the player you are looking at CR <amount>.", Cmd_Credits_f, 1, qfalse, 1, 128, ~(1 << GT_FFA), 0},
	{"property", "View your owned properties.  If you have the right rank, you can modify your property access here.", Cmd_Property_f, 0, qfalse, 1, 0, 0, 0},
	{"register", "Register your account.", Cmd_Register_f, 0, qfalse, 0, 1, 0, 0, qtrue},
	{"seccode", "Show, edit, regenerate, or enable/disable your security code.", Cmd_Seccode_f, 0, qfalse, 1, 1, 0, 0},
	{"stats","Shows some stats (you need to be registered)", Cmd_GetStats_f, 0, qfalse, 1, 1, 0, 0},
	{"worthy", "List players on the server from whom you can score points in a duel.", Cmd_Worthy_f, 0, qfalse, 1, 1, ~(1 << GT_FFA), 0},
	{NULL},
};