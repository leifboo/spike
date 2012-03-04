
#include "dict.h"

#include "class.h"
#include "heart.h"
#include "native.h"
#include <stdlib.h>


struct IdentityDictionary {
    Object base;
    size_t size;
    size_t tally;
    Unknown **keyArray;
    Unknown **valueArray;
};


#define HASH(key) (mask & ((size_t)(key) / sizeof(Unknown *)))


/*------------------------------------------------------------------------*/
/* C API */

Unknown *IdentityDictionary_GetItem(IdentityDictionary *self, Unknown *key) {
    size_t mask, start, i;
    
    mask = self->size - 1;
    start = HASH(key);
    
    for (i = start; i < self->size; ++i) {
        if (self->keyArray[i] == key) {
            INCREF(self->valueArray[i]);
            return self->valueArray[i];
        }
    }
    
    for (i = 0; i < start; ++i) {
        if (self->keyArray[i] == key) {
            INCREF(self->valueArray[i]);
            return self->valueArray[i];
        }
    }

    return 0;
}

Unknown *IdentityDictionary_KeyWithValue(IdentityDictionary *self, Unknown *value) {
    size_t i;
    
    for (i = 0; i < self->size; ++i) {
        if (value == self->valueArray[i]) {
            INCREF(self->keyArray[i]);
            return self->keyArray[i];
        }
    }
    return 0;
}

void IdentityDictionary_SetItem(IdentityDictionary *self, Unknown *key, Unknown *value) {
    size_t mask, start, i, o;
    
    mask = self->size - 1;
    start = HASH(key);
    
    for (i = start; i < self->size; ++i) {
        if (!self->keyArray[i]) {
            goto insert;
        } else if (self->keyArray[i] == key) {
            INCREF(value);
            DECREF(self->valueArray[i]);
            self->valueArray[i] = value;
            return;
        }
    }
    
    for (i = 0; i < start; ++i) {
        if (!self->keyArray[i]) {
            goto insert;
        } else if (self->keyArray[i] == key) {
            INCREF(value);
            DECREF(self->valueArray[i]);
            self->valueArray[i] = value;
            return;
        }
    }
    
    Halt(HALT_ASSERTION_ERROR, "not reached");
    return;
    
 insert:
    INCREF(key);
    INCREF(value);
    self->keyArray[i] = key;
    self->valueArray[i] = value;
    ++self->tally;
    
    if (self->size - self->tally < self->size / 4 + 1) {
        /* grow */
        size_t oldSize;
        Unknown **oldKeyArray, **oldValueArray;
        
        oldSize = self->size;
        oldKeyArray = self->keyArray;
        oldValueArray = self->valueArray;
        
        self->size <<= 1;
        self->keyArray = (Unknown **)calloc(self->size, sizeof(Unknown *));
        self->valueArray = (Unknown **)calloc(self->size, sizeof(Unknown *));
        
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
                Halt(HALT_ASSERTION_ERROR, "not reached");
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

IdentityDictionary *IdentityDictionary_New(void) {
    return (IdentityDictionary *)Object_New(CLASS(IdentityDictionary));
}

int IdentityDictionary_Next(IdentityDictionary *self,
                               size_t *pPos,
                               Unknown **pKey,
                               Unknown **pValue)
{
    size_t pos;
    
    pos = *pPos;
    for ( ; pos < self->size; ++pos) {
        Unknown *key = self->keyArray[pos];
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

size_t IdentityDictionary_Size(IdentityDictionary *self) {
    return self->tally;
}


/*------------------------------------------------------------------------*/
/* methods */

static Unknown *IdentityDictionary_item(Unknown *_self, Unknown *key, Unknown *arg1) {
    IdentityDictionary *self;
    Unknown *value;
    
    self = (IdentityDictionary *)_self;
    value = IdentityDictionary_GetItem(self, key);
    if (!value) {
        Halt(HALT_KEY_ERROR, "");
        return 0;
    }
    return value;
}

static Unknown *IdentityDictionary_get(Unknown *_self, Unknown *key, Unknown *arg1) {
    IdentityDictionary *self;
    Unknown *value;
    
    self = (IdentityDictionary *)_self;
    value = IdentityDictionary_GetItem(self, key);
    if (!value) {
        value = GLOBAL(null);
        INCREF(value);
    }
    return value;
}

static Unknown *IdentityDictionary_setItem(Unknown *_self, Unknown *key, Unknown *value) {
    IdentityDictionary *self;
    
    self = (IdentityDictionary *)_self;
    IdentityDictionary_SetItem(self, key, value);
    INCREF(GLOBAL(xvoid));
    return GLOBAL(xvoid);
}


/*------------------------------------------------------------------------*/
/* low-level hooks */

static void IdentityDictionary_zero(Object *_self) {
    IdentityDictionary *self = (IdentityDictionary *)_self;
    (*CLASS(IdentityDictionary)->superclass->zero)(_self);
    self->size = 2; /* must be a power of 2 */
    self->tally = 0;
    self->keyArray = (Unknown **)calloc(self->size, sizeof(Unknown *));
    self->valueArray = (Unknown **)calloc(self->size, sizeof(Unknown *));
}

static void IdentityDictionary_dealloc(Object *_self, Unknown **l) {
    IdentityDictionary *self = (IdentityDictionary *)_self;
    size_t pos;
    
    for (pos = 0; pos < self->size; ++pos) {
        Unknown *key = self->keyArray[pos];
        if (key) {
            LDECREF(key, l);
            LDECREF(self->valueArray[pos], l);
        }
    }
    
    free(self->keyArray);
    free(self->valueArray);
    
    (*CLASS(IdentityDictionary)->superclass->dealloc)(_self, l);
}


/*------------------------------------------------------------------------*/
/* class tmpl */

typedef struct IdentityDictionarySubclass {
    IdentityDictionary base;
    Unknown *variables[1]; /* stretchy */
} IdentityDictionarySubclass;

static AccessorTmpl accessors[] = {
    { "size", T_SIZE, offsetof(IdentityDictionary, tally), Accessor_READ },
    { 0 }
};

static MethodTmpl methods[] = {
    { "__index__", NativeCode_ARGS_1 | NativeCode_LEAF, &IdentityDictionary_item },
    { "get", NativeCode_ARGS_1 | NativeCode_LEAF, &IdentityDictionary_get },
    { 0 }
};

static MethodTmpl lvalueMethods[] = {
    { "__index__", NativeCode_ARGS_2 | NativeCode_LEAF, &IdentityDictionary_setItem },
    { 0 }
};

ClassTmpl ClassIdentityDictionaryTmpl = {
    HEART_CLASS_TMPL(IdentityDictionary, Object), {
        accessors,
        methods,
        lvalueMethods,
        offsetof(IdentityDictionarySubclass, variables),
        /*itemSize*/ 0,
        &IdentityDictionary_zero,
        &IdentityDictionary_dealloc
    }, /*meta*/ {
        0
    }
};
