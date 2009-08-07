
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
    return (SpkIdentityDictionary *)SpkObject_New(Spk_ClassIdentityDictionary);
}


/*------------------------------------------------------------------------*/
/* low-level hooks */

static void IdentityDictionary_zero(SpkObject *_self) {
    SpkIdentityDictionary *self = (SpkIdentityDictionary *)_self;
    (*Spk_ClassIdentityDictionary->superclass->zero)(_self);
    self->size = 2; /* must be a power of 2 */
    self->tally = 0;
    self->keyArray = (SpkUnknown **)calloc(self->size, sizeof(SpkUnknown *));
    self->valueArray = (SpkUnknown **)calloc(self->size, sizeof(SpkUnknown *));
}

static void IdentityDictionary_dealloc(SpkObject *_self) {
    SpkIdentityDictionary *self = (SpkIdentityDictionary *)_self;
    free(self->keyArray);
    free(self->valueArray);
    self->keyArray = 0;
    self->valueArray = 0;
    (*Spk_ClassIdentityDictionary->superclass->dealloc)(_self);
}


/*------------------------------------------------------------------------*/
/* memory layout of instances */

static void IdentityDictionary_traverse_init(SpkObject *_self) {
    SpkIdentityDictionary *self;
    
    self = (SpkIdentityDictionary *)_self;
    (*Spk_ClassIdentityDictionary->superclass->traverse.init)(_self);
    for ( ; self->size > 0; --self->size) {
        if (self->keyArray[self->size - 1])
            break;
    }
    self->tally = self->size;
}

static SpkUnknown **IdentityDictionary_traverse_current(SpkObject *_self) {
    SpkIdentityDictionary *self;
    
    self = (SpkIdentityDictionary *)_self;
    if (self->size > 0) {
        return &self->keyArray[self->size - 1];
    }
    if (self->tally > 0) {
        return &self->valueArray[self->tally - 1];
    }
    return (*Spk_ClassIdentityDictionary->superclass->traverse.current)(_self);
}

static void IdentityDictionary_traverse_next(SpkObject *_self) {
    SpkIdentityDictionary *self;
    size_t i;
    
    self = (SpkIdentityDictionary *)_self;
    if (self->size > 0) {
        if (self->size > 1) {
            for (i = self->size - 1; i > 0; --i) {
                if (self->keyArray[i - 1]) {
                    self->size = i;
                    return;
                }
            }
        }
        self->size = 0;
        return; /* next item is from 'valueArray' */
    }
    if (self->tally > 0) {
        if (self->tally > 1) {
            for (i = self->tally - 1; i > 0; --i) {
                if (self->valueArray[i - 1]) {
                    self->tally = i;
                    return;
                }
            }
        }
        self->tally = 0;
    }
    (*Spk_ClassIdentityDictionary->superclass->traverse.next)(_self);
}


/*------------------------------------------------------------------------*/
/* class template */

static SpkMethodTmpl methods[] = {
    { 0, 0, 0}
};

static SpkTraverse traverse = {
    &IdentityDictionary_traverse_init,
    &IdentityDictionary_traverse_current,
    &IdentityDictionary_traverse_next,
};

SpkClassTmpl Spk_ClassIdentityDictionaryTmpl = {
    "IdentityDictionary", {
        /*accessors*/ 0,
        methods,
        /*lvalueMethods*/ 0,
        offsetof(SpkIdentityDictionarySubclass, variables),
        /*itemSize*/ 0,
        &IdentityDictionary_zero,
        &IdentityDictionary_dealloc,
        &traverse
    }, /*meta*/ {
    }
};
