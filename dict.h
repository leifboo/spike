
#ifndef __spk_dict_h__
#define __spk_dict_h__


#include "obj.h"


typedef struct SpkIdentityDictionary {
    SpkObject base;
    size_t size;
    size_t tally;
    SpkUnknown **keyArray;
    SpkUnknown **valueArray;
} SpkIdentityDictionary;

typedef struct SpkIdentityDictionarySubclass {
    SpkIdentityDictionary base;
    SpkUnknown *variables[1]; /* stretchy */
} SpkIdentityDictionarySubclass;


extern struct SpkBehavior *Spk_ClassIdentityDictionary;
extern struct SpkClassTmpl Spk_ClassIdentityDictionaryTmpl;


SpkUnknown *SpkIdentityDictionary_at(SpkIdentityDictionary *self, SpkUnknown *key);
SpkUnknown *SpkIdentityDictionary_keyAtValue(SpkIdentityDictionary *self, SpkUnknown *value);
void SpkIdentityDictionary_atPut(SpkIdentityDictionary *self, SpkUnknown *key, SpkUnknown *value);
SpkIdentityDictionary *SpkIdentityDictionary_new(void);


#endif /* __spk_dict_h__ */
