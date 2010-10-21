
#include "dict.h"

#include "class.h"
#include "heart.h"
#include "native.h"
#include <stdlib.h>


struct SpkIdentityDictionary {
    SpkObject base;
    size_t size;
    size_t tally;
    SpkUnknown **keyArray;
    SpkUnknown **valueArray;
};


#define HASH(key) (mask & ((size_t)(key) / sizeof(SpkUnknown *)))


/*------------------------------------------------------------------------*/
/* C API */

SpkUnknown *SpkIdentityDictionary_GetItem(SpkIdentityDictionary *self, SpkUnknown *key) {
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

SpkUnknown *SpkIdentityDictionary_KeyWithValue(SpkIdentityDictionary *self, SpkUnknown *value) {
    size_t i;
    
    for (i = 0; i < self->size; ++i) {
        if (value == self->valueArray[i]) {
            Spk_INCREF(self->keyArray[i]);
            return self->keyArray[i];
        }
    }
    return 0;
}

void SpkIdentityDictionary_SetItem(SpkIdentityDictionary *self, SpkUnknown *key, SpkUnknown *value) {
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

SpkIdentityDictionary *SpkIdentityDictionary_New(void) {
    return (SpkIdentityDictionary *)SpkObject_New(Spk_CLASS(IdentityDictionary));
}

int SpkIdentityDictionary_Next(SpkIdentityDictionary *self,
                               size_t *pPos,
                               SpkUnknown **pKey,
                               SpkUnknown **pValue)
{
    size_t pos;
    
    pos = *pPos;
    for ( ; pos < self->size; ++pos) {
        SpkUnknown *key = self->keyArray[pos];
        if (key) {
            *pPos = pos + 1;
            *pKey = key;
            *pValue = self->valueArray[pos];
            return 1;
        }
    }
    *pPos = pos;
    *pKey = 0;
    *pValue = 0;
    return 0;
}

size_t SpkIdentityDictionary_Size(SpkIdentityDictionary *self) {
    return self->tally;
}


/*------------------------------------------------------------------------*/
/* methods */

static SpkUnknown *IdentityDictionary_item(SpkUnknown *_self, SpkUnknown *key, SpkUnknown *arg1) {
    SpkIdentityDictionary *self;
    SpkUnknown *value;
    
    self = (SpkIdentityDictionary *)_self;
    value = SpkIdentityDictionary_GetItem(self, key);
    if (!value) {
        Spk_Halt(Spk_HALT_KEY_ERROR, "");
        return 0;
    }
    return value;
}

static SpkUnknown *IdentityDictionary_get(SpkUnknown *_self, SpkUnknown *key, SpkUnknown *arg1) {
    SpkIdentityDictionary *self;
    SpkUnknown *value;
    
    self = (SpkIdentityDictionary *)_self;
    value = SpkIdentityDictionary_GetItem(self, key);
    if (!value) {
        value = Spk_GLOBAL(null);
        Spk_INCREF(value);
    }
    return value;
}

static SpkUnknown *IdentityDictionary_setItem(SpkUnknown *_self, SpkUnknown *key, SpkUnknown *value) {
    SpkIdentityDictionary *self;
    
    self = (SpkIdentityDictionary *)_self;
    SpkIdentityDictionary_SetItem(self, key, value);
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
}


/*------------------------------------------------------------------------*/
/* low-level hooks */

static void IdentityDictionary_zero(SpkObject *_self) {
    SpkIdentityDictionary *self = (SpkIdentityDictionary *)_self;
    (*Spk_CLASS(IdentityDictionary)->superclass->zero)(_self);
    self->size = 2; /* must be a power of 2 */
    self->tally = 0;
    self->keyArray = (SpkUnknown **)calloc(self->size, sizeof(SpkUnknown *));
    self->valueArray = (SpkUnknown **)calloc(self->size, sizeof(SpkUnknown *));
}

static void IdentityDictionary_dealloc(SpkObject *_self, SpkUnknown **l) {
    SpkIdentityDictionary *self = (SpkIdentityDictionary *)_self;
    size_t pos;
    
    for (pos = 0; pos < self->size; ++pos) {
        SpkUnknown *key = self->keyArray[pos];
        if (key) {
            Spk_LDECREF(key, l);
            Spk_LDECREF(self->valueArray[pos], l);
        }
    }
    
    free(self->keyArray);
    free(self->valueArray);
    
    (*Spk_CLASS(IdentityDictionary)->superclass->dealloc)(_self, l);
}


/*------------------------------------------------------------------------*/
/* class tmpl */

typedef struct SpkIdentityDictionarySubclass {
    SpkIdentityDictionary base;
    SpkUnknown *variables[1]; /* stretchy */
} SpkIdentityDictionarySubclass;

static SpkAccessorTmpl accessors[] = {
    { "size", Spk_T_SIZE, offsetof(SpkIdentityDictionary, tally), SpkAccessor_READ },
    { 0 }
};

static SpkMethodTmpl methods[] = {
    { "__index__", SpkNativeCode_ARGS_1 | SpkNativeCode_LEAF, &IdentityDictionary_item },
    { "get", SpkNativeCode_ARGS_1 | SpkNativeCode_LEAF, &IdentityDictionary_get },
    { 0 }
};

static SpkMethodTmpl lvalueMethods[] = {
    { "__index__", SpkNativeCode_ARGS_2 | SpkNativeCode_LEAF, &IdentityDictionary_setItem },
    { 0 }
};

SpkClassTmpl Spk_ClassIdentityDictionaryTmpl = {
    Spk_HEART_CLASS_TMPL(IdentityDictionary, Object), {
        accessors,
        methods,
        lvalueMethods,
        offsetof(SpkIdentityDictionarySubclass, variables),
        /*itemSize*/ 0,
        &IdentityDictionary_zero,
        &IdentityDictionary_dealloc
    }, /*meta*/ {
        0
    }
};
