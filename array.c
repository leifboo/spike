
#include "array.h"

#include "behavior.h"
#include "int.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>


Behavior *ClassArray;


#define ARRAY(op) ((Object **)SpkObject_ITEM_BASE(op))


/*------------------------------------------------------------------------*/
/* methods -- operators */

/* OPER_GET_ITEM */
static Object *Array_item(Object *_self, Object *arg0, Object *arg1) {
    Array *self;
    long index;
    Object *item;
    
    self = (Array *)_self;
    assert(arg0->klass == ClassInteger); /* XXX */
    index = SpkInteger_asLong((Integer *)arg0);
    assert(0 <= index && index < self->size); /* XXX */
    item = ARRAY(self)[index];
    if (!item)
        item = Spk_uninit;
    return item;
}

/* OPER_SET_ITEM */
static Object *Array_setItem(Object *_self, Object *arg0, Object *arg1) {
    Array *self;
    long index;
    
    self = (Array *)_self;
    assert(arg0->klass == ClassInteger); /* XXX */
    index = SpkInteger_asLong((Integer *)arg0);
    assert(0 <= index && index < self->size); /* XXX */
    ARRAY(self)[index] = arg1;
    if (0) {
        /* XXX */
        return Spk_void;
    }
    return arg1;
}


/*------------------------------------------------------------------------*/
/* methods -- other */

static Object *Array_print(Object *_self, Object *arg0, Object *arg1) {
    Array *self;
    int i;
    
    self = (Array *)_self;
    fputc('[', stdout);
    for (i = 0; i < self->size; ++i) {
        fputs("XXX, ", stdout);
    }
    fputc(']', stdout);
    return Spk_void;
}


/*------------------------------------------------------------------------*/
/* class template */

static SpkMethodTmpl methods[] = {
    /* operators */
#if 0 /*XXX*/
    { "__mul__",    SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &Array_mul },
    { "__mod__",    SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &Array_mod },
    { "__add__",    SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &Array_add },
    { "__lt__",     SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &Array_lt  },
    { "__gt__",     SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &Array_gt  },
    { "__le__",     SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &Array_le  },
    { "__ge__",     SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &Array_ge  },
    { "__eq__",     SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &Array_eq  },
    { "__ne__",     SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &Array_ne  },
#endif
    /* call operators */
    { "__item__",    SpkNativeCode_CALL | SpkNativeCode_ARGS_1 | SpkNativeCode_LEAF, &Array_item },
    { "__setItem__", SpkNativeCode_CALL | SpkNativeCode_ARGS_2 | SpkNativeCode_LEAF, &Array_setItem },
    /* other */
    { "print", SpkNativeCode_METH_ATTR | SpkNativeCode_ARGS_0, &Array_print },
    { 0, 0, 0}
};

SpkClassTmpl ClassArrayTmpl = {
    "Array",
    offsetof(ArraySubclass, variables),
    sizeof(Array),
    sizeof(Object *),
    0,
    methods
};


/*------------------------------------------------------------------------*/
/* C API */

Array *SpkArray_withItems(Object **items, size_t n) {
    Array *newArray;
    size_t i;
    
    newArray = (Array *)malloc(sizeof(Array) + n*sizeof(Object **));
    newArray->base.klass = ClassArray;
    newArray->size = n;
    for (i = 0; i < n; ++i) {
        ARRAY(newArray)[i] = items[n];
    }
    return newArray;
}

size_t SpkArray_size(Array *self) {
    return self->size;
}

Object *SpkArray_item(Array *self, long index) {
    Object *item;
    
    assert(0 <= index && index < self->size); /* XXX */
    item = ARRAY(self)[index];
    if (!item)
        item = Spk_uninit;
    return item;
}
