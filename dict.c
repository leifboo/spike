
#include "dict.h"

#include "behavior.h"
#include "module.h"
#include <assert.h>
#include <stdlib.h>


#define HASH(key) (mask & ((size_t)(key) / sizeof(Object *)))


Behavior *ClassIdentityDictionary;


void SpkClassIdentityDictionary_init(void) {
    ClassIdentityDictionary = SpkBehavior_new(ClassObject, builtInModule, 0);
    ClassIdentityDictionary->methodDict->base.klass = ClassIdentityDictionary;
    ClassIdentityDictionary->instVarOffset = offsetof(IdentityDictionarySubclass, variables);
}

Object *SpkIdentityDictionary_at(IdentityDictionary *self, Object *key) {
    size_t mask, start, i;
    
    mask = self->size - 1;
    start = HASH(key);
    
    for (i = start; i < self->size; ++i) {
        if (self->keyArray[i] == key) {
            return self->valueArray[i];
        }
    }
    
    for (i = 0; i < start; ++i) {
        if (self->keyArray[i] == key) {
            return self->valueArray[i];
        }
    }

    return 0;
}

Object *SpkIdentityDictionary_keyAtValue(IdentityDictionary *self, Object *value) {
    size_t i;
    
    for (i = 0; i < self->size; ++i) {
        if (value == self->valueArray[i]) {
            return self->keyArray[i];
        }
    }
    return 0;
}

void SpkIdentityDictionary_atPut(IdentityDictionary *self, Object *key, Object *value) {
    size_t mask, start, i, o;
    
    mask = self->size - 1;
    start = HASH(key);
    
    for (i = start; i < self->size; ++i) {
        if (!self->keyArray[i]) {
            goto insert;
        } else if (self->keyArray[i] == key) {
            self->valueArray[i] = value;
            return;
        }
    }
    
    for (i = 0; i < start; ++i) {
        if (!self->keyArray[i]) {
            goto insert;
        } else if (self->keyArray[i] == key) {
            self->valueArray[i] = value;
            return;
        }
    }
    
    assert(0 && "not reached");
    
 insert:
    self->keyArray[i] = key;
    self->valueArray[i] = value;
    ++self->tally;
    
    if (self->size - self->tally < self->size / 4 + 1) {
        /* grow */
        size_t oldSize;
        Object **oldKeyArray, **oldValueArray;
        
        oldSize = self->size;
        oldKeyArray = self->keyArray;
        oldValueArray = self->valueArray;
        
        self->size <<= 1;
        self->keyArray = (Object **)calloc(self->size, sizeof(Object *));
        self->valueArray = (Object **)calloc(self->size, sizeof(Object *));
        
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
                assert(0 && "not reached");
 reinsert:
                self->keyArray[i] = key;
                self->valueArray[i] = value;
            }
        }
        
        free(oldKeyArray);
        free(oldValueArray);
    }
}

IdentityDictionary *SpkIdentityDictionary_new(void) {
    IdentityDictionary *self;
    
    self = (IdentityDictionary *)malloc(sizeof(IdentityDictionary));
    self->base.klass = ClassIdentityDictionary;
    self->size = 2; /* must be a power of 2 */
    self->tally = 0;
    self->keyArray = (Object **)calloc(self->size, sizeof(Object *));
    self->valueArray = (Object **)calloc(self->size, sizeof(Object *));
    return self;
}
