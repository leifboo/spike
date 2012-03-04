
#include "array.h"

#include "class.h"
#include "heart.h"
#include "int.h"
#include "native.h"
#include "rodata.h"
#include "str.h"

#include <stdarg.h>
#include <stdlib.h>


#define ARRAY(op) ((Unknown **)VariableObject_ITEM_BASE(op))


/*------------------------------------------------------------------------*/
/* methods -- operators */

static Unknown *Array_item(Unknown *_self, Unknown *arg0, Unknown *arg1) {
    Array *self; Integer *arg;
    ptrdiff_t index;
    Unknown *item;
    
    self = (Array *)_self;
    arg = CAST(Integer, arg0);
    if (!arg) {
        Halt(HALT_TYPE_ERROR, "an integer is required");
        return 0;
    }
    index = Integer_AsCPtrdiff(arg);
    if (index < 0 || self->size <= (size_t)index) {
        Halt(HALT_INDEX_ERROR, "index out of range");
        return 0;
    }
    item = ARRAY(self)[index];
    INCREF(item);
    return (Unknown *)item;
}

static Unknown *Array_setItem(Unknown *_self, Unknown *arg0, Unknown *arg1) {
    Array *self; Integer *arg;
    ptrdiff_t index;
    
    self = (Array *)_self;
    arg = CAST(Integer, arg0);
    if (!arg) {
        Halt(HALT_TYPE_ERROR, "an integer is required");
        return 0;
    }
    index = Integer_AsCPtrdiff(arg);
    if (index < 0 || self->size <= (size_t)index) {
        Halt(HALT_INDEX_ERROR, "index out of range");
        return 0;
    }
    INCREF(arg1);
    DECREF(ARRAY(self)[index]);
    ARRAY(self)[index] = arg1;
    INCREF(GLOBAL(xvoid));
    return GLOBAL(xvoid);
}


/*------------------------------------------------------------------------*/
/* methods -- enumerating */

static Unknown *Array_do(Unknown *_self, Unknown *arg0, Unknown *arg1) {
    Array *self;
    size_t i;
    Unknown *result;
    
    self = (Array *)_self;
    for (i = 0; i < self->size; ++i) {
        result = Call(GLOBAL(theInterpreter), arg0, OPER_APPLY, ARRAY(self)[i], 0);
        if (!result)
            return 0; /* unwind */
    }
    INCREF(GLOBAL(xvoid));
    return GLOBAL(xvoid);
}


/*------------------------------------------------------------------------*/
/* methods -- printing */

static Unknown *Array_printString(Unknown *_self, Unknown *arg0, Unknown *arg1) {
    Array *self;
    size_t i;
    String *result, *s, *comma;
    
    self = (Array *)_self;
    
    if (self->size == 0)
        return (Unknown *)String_FromCString("{}");
    
    result = String_FromCString("{ ");
    comma = String_FromCString(", ");
    
    for (i = 0; i < self->size; ++i) {
        s = (String *)Attr(GLOBAL(theInterpreter), ARRAY(self)[i], printString);
        if (!s)
            return 0; /* unwind */
        String_Concat(&result, s);
        if (i + 1 < self->size)
            String_Concat(&result, comma);
    }
    
    String_Concat(&result, String_FromCString(" }"));
    return (Unknown *)result;
}


/*------------------------------------------------------------------------*/
/* low-level hooks */

static void Array_zero(Object *_self) {
    Array *self;
    size_t i;
    
    self = (Array *)_self;
    (*CLASS(Array)->superclass->zero)(_self);
    for (i = 0; i < self->size; ++i) {
        INCREF(GLOBAL(uninit));
        ARRAY(self)[i] = GLOBAL(uninit);
    }
}

static void Array_dealloc(Object *_self, Unknown **l) {
    Array *self;
    size_t i;
    
    self = (Array *)_self;
    for (i = 0; i < self->size; ++i) {
        LDECREF(ARRAY(self)[i], l);
    }
    (*CLASS(Array)->superclass->dealloc)(_self, l);
}


/*------------------------------------------------------------------------*/
/* class template */

typedef VariableObjectSubclass ArraySubclass;

static AccessorTmpl accessors[] = {
    { "size", T_SIZE, offsetof(Array, size), Accessor_READ },
    { 0 }
};

static MethodTmpl methods[] = {
    /* operators */
#if 0 /*XXX*/
    { "__mul__",    NativeCode_BINARY_OPER | NativeCode_LEAF, &Array_mul },
    { "__mod__",    NativeCode_BINARY_OPER | NativeCode_LEAF, &Array_mod },
    { "__add__",    NativeCode_BINARY_OPER | NativeCode_LEAF, &Array_add },
    { "__lt__",     NativeCode_BINARY_OPER | NativeCode_LEAF, &Array_lt  },
    { "__gt__",     NativeCode_BINARY_OPER | NativeCode_LEAF, &Array_gt  },
    { "__le__",     NativeCode_BINARY_OPER | NativeCode_LEAF, &Array_le  },
    { "__ge__",     NativeCode_BINARY_OPER | NativeCode_LEAF, &Array_ge  },
    { "__eq__",     NativeCode_BINARY_OPER | NativeCode_LEAF, &Array_eq  },
    { "__ne__",     NativeCode_BINARY_OPER | NativeCode_LEAF, &Array_ne  },
#endif
    /* call operators */
    { "__index__", NativeCode_ARGS_1 | NativeCode_LEAF, &Array_item },
    /* enumerating */
    { "do:", NativeCode_ARGS_1, &Array_do },
    /* printing */
    { "printString", NativeCode_ARGS_0, &Array_printString },
    { 0 }
};

static MethodTmpl lvalueMethods[] = {
    { "__index__", NativeCode_ARGS_2 | NativeCode_LEAF, &Array_setItem },
    { 0 }
};

ClassTmpl ClassArrayTmpl = {
    HEART_CLASS_TMPL(Array, VariableObject), {
        accessors,
        methods,
        lvalueMethods,
        offsetof(ArraySubclass, variables),
        sizeof(Unknown *),
        &Array_zero,
        &Array_dealloc
    }, /*meta*/ {
        0
    }
};


/*------------------------------------------------------------------------*/
/* C API */

Array *Array_New(size_t size) {
    return (Array *)Object_NewVar(CLASS(Array), size);
}

Array *Array_WithArguments(Unknown **stackPointer, size_t argumentCount,
                                 Array *varArgArray, size_t skip) {
    Array *argumentArray;
    Unknown *item;
    size_t i, n, varArgCount;
    
    varArgCount = varArgArray ? varArgArray->size - skip : 0;
    n = argumentCount + varArgCount;
    
    argumentArray = (Array *)Object_NewVar(CLASS(Array), n);
    
    /* copy & reverse fixed arguments */
    for (i = 0; i < argumentCount; ++i) {
        item = stackPointer[argumentCount - i - 1];
        INCREF(item);
        DECREF(ARRAY(argumentArray)[i]);
        ARRAY(argumentArray)[i] = item;
    }
    /* copy variable arguments */
    for ( ; i < n; ++i) {
        item = ARRAY(varArgArray)[skip + i - argumentCount];
        INCREF(item);
        DECREF(ARRAY(argumentArray)[i]);
        ARRAY(argumentArray)[i] = item;
    }
    
    return argumentArray;
}

Array *Array_FromVAList(va_list ap) {
    Array *newArray;
    Unknown *obj;
    size_t size, i;
    
    size = 2;
    newArray = (Array *)Object_NewVar(CLASS(Array), size);
    for (i = 0,  obj = va_arg(ap, Unknown *);
         /*****/ obj;
         ++i,    obj = va_arg(ap, Unknown *)) {
        if (i >= size) {
            Array *tmpArray = newArray;
            size_t tmpSize = size, j;
            size *= 2;
            newArray = (Array *)Object_NewVar(CLASS(Array), size);
            for (j = 0; j < tmpSize; ++j) {
                Unknown *tmp = ARRAY(newArray)[j];
                ARRAY(newArray)[j] = ARRAY(tmpArray)[j];
                ARRAY(tmpArray)[j] = tmp;
            }
            DECREF(tmpArray);
        }
        INCREF(obj);
        DECREF(ARRAY(newArray)[i]);
        ARRAY(newArray)[i] = obj;
    }
    newArray->size = i;
    return newArray;
}

size_t Array_Size(Array *self) {
    return self->size;
}

Unknown *Array_GetItem(Array *self, size_t index) {
    Unknown *item;
    
    if (index < 0 || self->size <= index) {
        Halt(HALT_INDEX_ERROR, "index out of range");
        return 0;
    }
    item = ARRAY(self)[index];
    INCREF(item);
    return item;
}

Unknown *Array_SetItem(Array *self, size_t index, Unknown *value) {
    if (index < 0 || self->size <= index) {
        Halt(HALT_INDEX_ERROR, "index out of range");
        return 0;
    }
    INCREF(value);
    DECREF(ARRAY(self)[index]);
    ARRAY(self)[index] = value;
    INCREF(GLOBAL(xvoid));
    return GLOBAL(xvoid);
}

void Array_Sort(Array *self,
                   int (*compare)(Unknown *const *, Unknown *const *))
{
    qsort(ARRAY(self),
          self->size,
          sizeof(Unknown *),
          (int(*)(const void *, const void *))compare
        );
}
