

char* Accounts_Custom_GetValue(Account_t *acc, char *key);
#define PlayerAcc_Custom_GetValue(ent, key) Accounts_Custom_GetValue(ent->client->pers.Lmd.account, key)

void Accounts_Custom_SetValue(Account_t *acc, char *key, char *value);
#define PlayerAcc_Custom_SetValue(ent, key, value) Accounts_Custom_SetValue(ent->client->pers.Lmd.account, key, value)

KeyPairSet_t* Accounts_Custom_GetSet(Account_t *acc);
#define PlayerAcc_Custom_GetSet(ent) Accounts_Custom_GetSet(ent->client->pers.Lmd.account)
