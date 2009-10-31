
#ifndef __spk_dict_h__
#define __spk_dict_h__


#include "obj.h"


typedef struct SpkIdentityDictionary SpkIdentityDictionary;


extern struct SpkClassTmpl Spk_ClassIdentityDictionaryTmpl;


SpkUnknown *SpkIdentityDictionary_GetItem(SpkIdentityDictionary *self, SpkUnknown *key);
SpkUnknown *SpkIdentityDictionary_KeyWithValue(SpkIdentityDictionary *self, SpkUnknown *value);
void SpkIdentityDictionary_SetItem(SpkIdentityDictionary *self, SpkUnknown *key, SpkUnknown *value);
SpkIdentityDictionary *SpkIdentityDictionary_New(void);
int SpkIdentityDictionary_Next(SpkIdentityDictionary *, size_t *,
                               SpkUnknown **, SpkUnknown **);
size_t SpkIdentityDictionary_Size(SpkIdentityDictionary *);


#endif /* __spk_dict_h__ */
