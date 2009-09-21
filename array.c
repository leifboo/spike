
#include "array.h"

#include "class.h"
#include "heart.h"
#include "int.h"
#include "native.h"

#include <stdarg.h>
#include <stdlib.h>


#define ARRAY(op) ((SpkUnknown **)SpkVariableObject_ITEM_BASE(op))


/*------------------------------------------------------------------------*/
/* methods -- operators */

static SpkUnknown *Array_item(SpkUnknown *_self, SpkUnknown *arg0, SpkUnknown *arg1) {
    SpkArray *self; SpkInteger *arg;
    ptrdiff_t index;
    SpkUnknown *item;
    
    self = (SpkArray *)_self;
    arg = Spk_CAST(Integer, arg0);
    if (!arg) {
        Spk_Halt(Spk_HALT_TYPE_ERROR, "an integer is required");
        return 0;
    }
    index = SpkInteger_AsCPtrdiff(arg);
    if (index < 0 || self->size <= (size_t)index) {
        Spk_Halt(Spk_HALT_INDEX_ERROR, "index out of range");
        return 0;
    }
    item = ARRAY(self)[index];
    Spk_INCREF(item);
    return (SpkUnknown *)item;
}

static SpkUnknown *Array_setItem(SpkUnknown *_self, SpkUnknown *arg0, SpkUnknown *arg1) {
    SpkArray *self; SpkInteger *arg;
    ptrdiff_t index;
    
    self = (SpkArray *)_self;
    arg = Spk_CAST(Integer, arg0);
    if (!arg) {
        Spk_Halt(Spk_HALT_TYPE_ERROR, "an integer is required");
        return 0;
    }
    index = SpkInteger_AsCPtrdiff(arg);
    if (index < 0 || self->size <= (size_t)index) {
        Spk_Halt(Spk_HALT_INDEX_ERROR, "index out of range");
        return 0;
    }
    Spk_INCREF(arg1);
    Spk_DECREF(ARRAY(self)[index]);
    ARRAY(self)[index] = arg1;
    Spk_INCREF(Spk_void);
    return Spk_void;
}


/*------------------------------------------------------------------------*/
/* methods -- enumerating */

static SpkUnknown *Array_do(SpkUnknown *_self, SpkUnknown *arg0, SpkUnknown *arg1) {
    SpkArray *self;
    size_t i;
    SpkUnknown *result;
    
    self = (SpkArray *)_self;
    for (i = 0; i < self->size; ++i) {
        result = Spk_Call(theInterpreter, arg0, Spk_OPER_APPLY, ARRAY(self)[i], 0);
        if (!result)
            return 0; /* unwind */
    }
    Spk_INCREF(Spk_void);
    return Spk_void;
}


/*------------------------------------------------------------------------*/
/* low-level hooks */

static void Array_zero(SpkObject *_self) {
    SpkArray *self;
    size_t i;
    
    self = (SpkArray *)_self;
    (*Spk_CLASS(Array)->superclass->zero)(_self);
    for (i = 0; i < self->size; ++i) {
        Spk_INCREF(Spk_uninit);
        ARRAY(self)[i] = Spk_uninit;
    }
}


/*------------------------------------------------------------------------*/
/* memory layout of instances */

static void Array_traverse_init(SpkObject *self) {
    (*Spk_CLASS(Array)->superclass->traverse.init)(self);
}

static SpkUnknown **Array_traverse_current(SpkObject *_self) {
    SpkArray *self;
    
    self = (SpkArray *)_self;
    if (self->size > 0)
        return &(ARRAY(self)[self->size - 1]);
    return (*Spk_CLASS(Array)->superclass->traverse.current)(_self);
}

static void Array_traverse_next(SpkObject *_self) {
    SpkArray *self;
    
    self = (SpkArray *)_self;
    if (self->size > 0)
        --self->size;
    else
        (*Spk_CLASS(Array)->superclass->traverse.next)(_self);
}


/*------------------------------------------------------------------------*/
/* class template */

typedef SpkVariableObjectSubclass SpkArraySubclass;

static SpkAccessorTmpl accessors[] = {
    { "size", Spk_T_SIZE, offsetof(SpkArray, size), SpkAccessor_READ },
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
    { "do:", SpkNativeCode_ARGS_1, &Array_do },
    { 0, 0, 0}
};

static SpkMethodTmpl lvalueMethods[] = {
    { "__index__", SpkNativeCode_ARGS_2 | SpkNativeCode_LEAF, &Array_setItem },
    { 0, 0, 0}
};

static SpkTraverse traverse = {
    &Array_traverse_init,
    &Array_traverse_current,
    &Array_traverse_next,
};

SpkClassTmpl Spk_ClassArrayTmpl = {
    Spk_HEART_CLASS_TMPL(Array, VariableObject), {
        accessors,
        methods,
        lvalueMethods,
        offsetof(SpkArraySubclass, variables),
        sizeof(SpkUnknown *),
        &Array_zero,
        /*dealloc*/ 0,
        &traverse
    }, /*meta*/ {
    }
};


/*------------------------------------------------------------------------*/
/* C API */

SpkArray *SpkArray_New(size_t size) {
    return (SpkArray *)SpkObject_NewVar(Spk_CLASS(Array), size);
}

SpkArray *SpkArray_WithArguments(SpkUnknown **stackPointer, size_t argumentCount,
                                 SpkArray *varArgArray, size_t skip) {
    SpkArray *argumentArray;
    SpkUnknown *item;
    size_t i, n, varArgCount;
    
    varArgCount = varArgArray ? varArgArray->size - skip : 0;
    n = argumentCount + varArgCount;
    
    argumentArray = (SpkArray *)SpkObject_NewVar(Spk_CLASS(Array), n);
    
    /* copy & reverse fixed arguments */
    for (i = 0; i < argumentCount; ++i) {
        item = stackPointer[argumentCount - i - 1];
        Spk_INCREF(item);
        Spk_DECREF(ARRAY(argumentArray)[i]);
        ARRAY(argumentArray)[i] = item;
    }
    /* copy variable arguments */
    for ( ; i < n; ++i) {
        item = ARRAY(varArgArray)[skip + i - argumentCount];
        Spk_INCREF(item);
        Spk_DECREF(ARRAY(argumentArray)[i]);
        ARRAY(argumentArray)[i] = item;
    }
    
    return argumentArray;
}

SpkArray *SpkArray_FromVAList(va_list ap) {
    SpkArray *newArray;
    SpkUnknown *obj;
    size_t size, i;
    
    size = 2;
    newArray = (SpkArray *)SpkObject_NewVar(Spk_CLASS(Array), size);
    for (i = 0,  obj = va_arg(ap, SpkUnknown *);
         /*****/ obj;
         ++i,    obj = va_arg(ap, SpkUnknown *)) {
        if (i >= size) {
            SpkArray *tmpArray = newArray;
            size_t tmpSize = size, j;
            size *= 2;
            newArray = (SpkArray *)SpkObject_NewVar(Spk_CLASS(Array), size);
            for (j = 0; j < tmpSize; ++j) {
                SpkUnknown *tmp = ARRAY(newArray)[j];
                ARRAY(newArray)[j] = ARRAY(tmpArray)[j];
                ARRAY(tmpArray)[j] = tmp;
            }
            Spk_DECREF(tmpArray);
        }
        Spk_INCREF(obj);
        Spk_DECREF(ARRAY(newArray)[i]);
        ARRAY(newArray)[i] = obj;
    }
    newArray->size = i;
    return newArray;
}

size_t SpkArray_Size(SpkArray *self) {
    return self->size;
}

SpkUnknown *SpkArray_GetItem(SpkArray *self, size_t index) {
    SpkUnknown *item;
    
    if (index < 0 || self->size <= index) {
        Spk_Halt(Spk_HALT_INDEX_ERROR, "index out of range");
        return 0;
    }
    item = ARRAY(self)[index];
    Spk_INCREF(item);
    return item;
}

SpkUnknown *SpkArray_SetItem(SpkArray *self, size_t index, SpkUnknown *value) {
    if (index < 0 || self->size <= index) {
        Spk_Halt(Spk_HALT_INDEX_ERROR, "index out of range");
        return 0;
    }
    Spk_INCREF(value);
    Spk_DECREF(ARRAY(self)[index]);
    ARRAY(self)[index] = value;
    Spk_INCREF(Spk_void);
    return Spk_void;
}

void SpkArray_Sort(SpkArray *self,
                   int (*compare)(SpkUnknown *const *, SpkUnknown *const *))
{
    qsort(ARRAY(self),
          self->size,
          sizeof(SpkUnknown *),
          (int(*)(const void *, const void *))compare
        );
}
