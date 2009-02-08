
#include "array.h"

#include "behavior.h"
#include "int.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>


Behavior *Spk_ClassArray;


#define ARRAY(op) ((Object **)SpkVariableObject_ITEM_BASE(op))


/*------------------------------------------------------------------------*/
/* methods -- operators */

/* OPER_GET_ITEM */
static Object *Array_item(Object *_self, Object *arg0, Object *arg1) {
    Array *self; Integer *arg;
    long index;
    Object *item;
    
    self = (Array *)_self;
    assert(arg = Spk_CAST(Integer, arg0)); /* XXX */
    index = SpkInteger_asLong(arg);
    assert(0 <= index && index < self->size); /* XXX */
    item = ARRAY(self)[index];
    if (!item)
        item = Spk_uninit;
    return item;
}

/* OPER_SET_ITEM */
static Object *Array_setItem(Object *_self, Object *arg0, Object *arg1) {
    Array *self; Integer *arg;
    long index;
    
    self = (Array *)_self;
    assert(arg = Spk_CAST(Integer, arg0)); /* XXX */
    index = SpkInteger_asLong(arg);
    assert(0 <= index && index < self->size); /* XXX */
    ARRAY(self)[index] = arg1;
    return Spk_void;
}


/*------------------------------------------------------------------------*/
/* methods -- enumerating */

static Object *Array_enumerate(Object *_self, Object *arg0, Object *arg1) {
    Array *self;
    size_t i;
    Object *result;
    
    self = (Array *)_self;
    for (i = 0; i < self->size; ++i) {
        result = Spk_call(theInterpreter, arg0, OPER_APPLY, ARRAY(self)[i], 0);
        if (!result)
            return 0; /* unwind */
    }
    return Spk_void;
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
/* memory layout of instances */

static void Array_traverse_init(Object *self) {
    (*self->klass->superclass->traverse.init)(self);
}

static Object **Array_traverse_current(Object *_self) {
    Array *self;
    
    self = (Array *)_self;
    if (self->size > 0)
        return &(ARRAY(self)[self->size - 1]);
    return (*self->base.klass->superclass->traverse.current)(_self);
}

static void Array_traverse_next(Object *_self) {
    Array *self;
    
    self = (Array *)_self;
    if (self->size > 0)
        --self->size;
    else
        (*self->base.klass->superclass->traverse.next)(_self);
}


/*------------------------------------------------------------------------*/
/* class template */

static SpkAccessorTmpl accessors[] = {
    { "size", Spk_T_SIZE, offsetof(Array, size), SpkAccessor_READ },
    { 0, 0, 0, 0 }
};

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
    { "__index__", SpkNativeCode_ARGS_1 | SpkNativeCode_LEAF, &Array_item },
    /* enumerating */
    { "enumerate", SpkNativeCode_METH_ATTR | SpkNativeCode_ARGS_1, &Array_enumerate },
    /* other */
    { "print", SpkNativeCode_METH_ATTR | SpkNativeCode_ARGS_0, &Array_print },
    { 0, 0, 0}
};

static SpkMethodTmpl lvalueMethods[] = {
    { "__index__", SpkNativeCode_ARGS_2 | SpkNativeCode_LEAF, &Array_setItem },
    { 0, 0, 0}
};

static traverse_t traverse = {
    &Array_traverse_init,
    &Array_traverse_current,
    &Array_traverse_next,
};

SpkClassTmpl Spk_ClassArrayTmpl = {
    "Array",
    offsetof(ArraySubclass, variables),
    sizeof(Array),
    sizeof(Object *),
    accessors,
    methods,
    lvalueMethods,
    0,
    0,
    &traverse
};


/*------------------------------------------------------------------------*/
/* C API */

Array *SpkArray_new(size_t size) {
    Array *newArray;
    size_t i;
    
    newArray = (Array *)malloc(sizeof(Array) + size*sizeof(Object **));
    newArray->base.klass = Spk_ClassArray;
    newArray->size = size;
    for (i = 0; i < size; ++i) {
        ARRAY(newArray)[i] = Spk_uninit;
    }
    return newArray;
}

Array *SpkArray_withArguments(Object **stackPointer, size_t argumentCount,
                              Array *varArgArray, size_t skip) {
    Array *argumentArray;
    size_t i, n, varArgCount;
    
    varArgCount = varArgArray ? varArgArray->size - skip : 0;
    n = argumentCount + varArgCount;
    
    argumentArray = SpkArray_new(n);
    
    /* copy & reverse fixed arguments */
    for (i = 0; i < argumentCount; ++i) {
        ARRAY(argumentArray)[i] = stackPointer[argumentCount - i - 1];
    }
    /* copy variable arguments */
    for ( ; i < n; ++i) {
        ARRAY(argumentArray)[i] = ARRAY(varArgArray)[skip + i - argumentCount];
    }
    
    return argumentArray;
}

Array *SpkArray_fromVAList(va_list ap) {
    Array *newArray;
    Object *obj;
    size_t size, i;
    
    size = 2;
    newArray = (Array *)malloc(sizeof(Array) + size*sizeof(Object **));
    newArray->base.klass = Spk_ClassArray;
    for (i = 0,  obj = va_arg(ap, Object *);
                 obj;
         ++i,    obj = va_arg(ap, Object *)) {
        if (i >= size) {
            size *= 2;
            newArray = (Array *)realloc(newArray, sizeof(Array) + size*sizeof(Object **));
        }
        ARRAY(newArray)[i] = obj;
    }
    newArray->size = i;
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
