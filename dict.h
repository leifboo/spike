
#ifndef __dict_h__
#define __dict_h__


#include "obj.h"


typedef struct IdentityDictionary IdentityDictionary;


extern struct ClassTmpl ClassIdentityDictionaryTmpl;


Unknown *IdentityDictionary_GetItem(IdentityDictionary *self, Unknown *key);
Unknown *IdentityDictionary_KeyWithValue(IdentityDictionary *self, Unknown *value);
void IdentityDictionary_SetItem(IdentityDictionary *self, Unknown *key, Unknown *value);
IdentityDictionary *IdentityDictionary_New(void);
int IdentityDictionary_Next(IdentityDictionary *, size_t *,
                               Unknown **, Unknown **);
size_t IdentityDictionary_Size(IdentityDictionary *);


#endif /* __dict_h__ */
