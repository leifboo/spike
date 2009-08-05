
#include "dict.h"

#include "class.h"
#include "native.h"
#include <stdlib.h>


#define HASH(key) (mask & ((size_t)(key) / sizeof(SpkUnknown *)))


struct SpkBehavior *Spk_ClassIdentityDictionary;


/*------------------------------------------------------------------------*/
/* methods */

SpkUnknown *SpkIdentityDictionary_at(SpkIdentityDictionary *self, SpkUnknown *key) {
    size_t mask, start, i;
    
    mask = self->size - 1;
    start = HASH(key);
    
    for (i = start; i < self->size; ++i) {
        if (self->keyArray[i] == key) {
            Spk_INCREF(self->valueArray[i]);
            return self->valueArray[i];
        }
    }
    
    for (i = 0; i < start; ++i) {
        if (self->keyArray[i] == key) {
            Spk_INCREF(self->valueArray[i]);
            return self->valueArray[i];
        }
    }

    return 0;
}

SpkUnknown *SpkIdentityDictionary_keyAtValue(SpkIdentityDictionary *self, SpkUnknown *value) {
    size_t i;
    
    for (i = 0; i < self->size; ++i) {
        if (value == self->valueArray[i]) {
            Spk_INCREF(self->keyArray[i]);
            return self->keyArray[i];
        }
    }
    return 0;
}

void SpkIdentityDictionary_atPut(SpkIdentityDictionary *self, SpkUnknown *key, SpkUnknown *value) {
    size_t mask, start, i, o;
    
    mask = self->size - 1;
    start = HASH(key);
    
    for (i = start; i < self->size; ++i) {
        if (!self->keyArray[i]) {
            goto insert;
        } else if (self->keyArray[i] == key) {
            Spk_INCREF(value);
            Spk_DECREF(self->valueArray[i]);
            self->valueArray[i] = value;
            return;
        }
    }
    
    for (i = 0; i < start; ++i) {
        if (!self->keyArray[i]) {
            goto insert;
        } else if (self->keyArray[i] == key) {
            Spk_INCREF(value);
            Spk_DECREF(self->valueArray[i]);
            self->valueArray[i] = value;
            return;
        }
    }
    
    Spk_Halt(Spk_HALT_ASSERTION_ERROR, "not reached");
    return;
    
 insert:
    Spk_INCREF(key);
    Spk_INCREF(value);
    self->keyArray[i] = key;
    self->valueArray[i] = value;
    ++self->tally;
    
    if (self->size - self->tally < self->size / 4 + 1) {
        /* grow */
        size_t oldSize;
        SpkUnknown **oldKeyArray, **oldValueArray;
        
        oldSize = self->size;
        oldKeyArray = self->keyArray;
        oldValueArray = self->valueArray;
        
        self->size <<= 1;
        self->keyArray = (SpkUnknown **)calloc(self->size, sizeof(SpkUnknown *));
        self->valueArray = (SpkUnknown **)calloc(self->size, sizeof(SpkUnknown *));
        
        mask = self->size - 1;
        for (o = 0; o < oldSize; ++o) {
            key = oldKeyArray[o];
            if (key) {
                value = oldValueArray[o];
                start = HASH(key);
                for (i = start; i < self->size; ++i) {
                    if (!self->keyArray[i]) {
                        goto reinsert;
                    }
                }
                for (i = 0; i < start; ++i) {
                    if (!self->keyArray[i]) {
                        goto reinsert;
                    }
                }
                Spk_Halt(Spk_HALT_ASSERTION_ERROR, "not reached");
                return;
 reinsert:
                self->keyArray[i] = key;
                self->valueArray[i] = value;
            }
        }
        
        free(oldKeyArray);
        free(oldValueArray);
    }
}

SpkIdentityDictionary *SpkIdentityDictionary_new(void) {
    SpkIdentityDictionary *self;
    
    self = (SpkIdentityDictionary *)SpkObject_New(Spk_ClassIdentityDictionary);
    if (!self)
        return 0;
    self->size = 2; /* must be a power of 2 */
    self->tally = 0;
    self->keyArray = (SpkUnknown **)calloc(self->size, sizeof(SpkUnknown *));
    self->valueArray = (SpkUnknown **)calloc(self->size, sizeof(SpkUnknown *));
    return self;
}


/*------------------------------------------------------------------------*/
/* class template */

static SpkMethodTmpl methods[] = {
    { 0, 0, 0}
};

SpkClassTmpl Spk_ClassIdentityDictionaryTmpl = {
    "IdentityDictionary", {
        /*accessors*/ 0,
        methods,
        /*lvalueMethods*/ 0,
        offsetof(SpkIdentityDictionarySubclass, variables)
    }, /*meta*/ {
    }
};
