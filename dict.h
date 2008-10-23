
#ifndef __dict_h__
#define __dict_h__


#include "interp.h"

#include <stddef.h>


typedef struct IdentityDictionary {
    Object base;
    size_t size;
    size_t tally;
    Object **keyArray;
    Object **valueArray;
} IdentityDictionary;


Object *SpkIdentityDictionary_at(IdentityDictionary *self, Object *key);
Object *SpkIdentityDictionary_keyAtValue(IdentityDictionary *self, Object *value);
void SpkIdentityDictionary_atPut(IdentityDictionary *self, Object *key, Object *value);
IdentityDictionary *SpkIdentityDictionary_new(void);


#endif /* __dict_h__ */
