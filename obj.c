
#include "obj.h"

#include "behavior.h"
#include "bool.h"
#ifdef MALTIPY
#include "bridge.h"
#endif
#include "class.h"
#include "heart.h"
#include "host.h"
#include "interp.h"
#include "native.h"
#include "str.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*------------------------------------------------------------------------*/
/* methods */

static SpkUnknown *Object_eq(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    SpkUnknown *result;

    result = (self == arg0 ? Spk_GLOBAL(xtrue) : Spk_GLOBAL(xfalse));
    Spk_INCREF(result);
    return result;
}

static SpkUnknown *Object_ne(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    SpkUnknown *temp, *result;
    
    temp = Spk_Oper(Spk_GLOBAL(theInterpreter), self, Spk_OPER_EQ, arg0, 0);
    result = Spk_Oper(Spk_GLOBAL(theInterpreter), temp, Spk_OPER_LNEG, 0);
    Spk_DECREF(temp);
    return result;
}

static SpkUnknown *Object_compoundExpression(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    Spk_INCREF(arg0);
    return arg0;
}

static SpkUnknown *Object_printString(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    static const char *format = "<%s instance at %p>";
    const char *className;
    SpkString *result;
    size_t len;
    char *str;
    
    className = SpkBehavior_NameAsCString(((SpkObject *)self)->klass);
    len = strlen(format) + strlen(className) + 2*sizeof(void*); /* assumes %p is hex */
    result = SpkString_FromCStringAndLength(0, len);
    if (!result)
        return 0;
    str = SpkString_AsCString(result);
    sprintf(str, format, className, self);
    result->size = strlen(str) + 1;
    return (SpkUnknown *)result;
}


/*------------------------------------------------------------------------*/
/* meta-methods */

static SpkUnknown *ClassObject_new(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    /* Answer a new instance of the receiver. */
    return (SpkUnknown *)SpkObject_New((SpkBehavior *)self);
}

static SpkUnknown *ClassVariableObject_new(SpkUnknown *_self, SpkUnknown *nItemsObj, SpkUnknown *arg1) {
    /* Answer a new instance of the receiver. */
    SpkClass *self;
    long nItems;
    
    self = (SpkClass *)_self;
    if (self->base.itemSize == 0) {
        Spk_Halt(Spk_HALT_VALUE_ERROR, "bad item size in class object");
        return 0;
    }
    if (!SpkHost_IsInteger(nItemsObj)) {
        Spk_Halt(Spk_HALT_TYPE_ERROR, "an integer object is required");
        return 0;
    }
    nItems = SpkHost_IntegerAsCLong(nItemsObj);
    if (nItems < 0) {
        Spk_Halt(Spk_HALT_VALUE_ERROR, "number of items cannot be negative");
        return 0;
    }
    
    return (SpkUnknown *)SpkObject_NewVar((SpkBehavior *)self, nItems);
}


/*------------------------------------------------------------------------*/
/* low-level hooks */

static void Object_zero(SpkObject *self) {
    SpkUnknown **variables;
    SpkBehavior *klass;
    size_t instVarTotal, i;
    
    klass = self->klass;
    
    variables = (SpkUnknown **)((char *)self + klass->instVarOffset);
    instVarTotal = klass->instVarBaseIndex + klass->instVarCount;
    
    for (i = 0; i < instVarTotal; ++i) {
        Spk_INCREF(Spk_GLOBAL(uninit));
        variables[i] = Spk_GLOBAL(uninit);
    }
}

static void Object_dealloc(SpkObject *self, SpkUnknown **l) {
    SpkUnknown **variables;
    SpkBehavior *klass;
    size_t instVarTotal, i;
    
    klass = self->klass;
    
    variables = (SpkUnknown **)((char *)self + klass->instVarOffset);
    instVarTotal = klass->instVarBaseIndex + klass->instVarCount;
    
    for (i = 0; i < instVarTotal; ++i) {
        Spk_LDECREF(variables[i], l);
    }
    
    Spk_LDECREF(klass, l);
    self->klass = 0;
}

static void VariableObject_zero(SpkObject *_self) {
    SpkVariableObject *self = (SpkVariableObject *)_self;
    (*Spk_CLASS(VariableObject)->superclass->zero)(_self);
    memset(SpkVariableObject_ITEM_BASE(self), 0,
           self->size * self->base.klass->itemSize);
}


/*------------------------------------------------------------------------*/
/* class templates */

static SpkAccessorTmpl ObjectAccessors[] = {
    { "class", Spk_T_OBJECT, offsetof(SpkObject, klass), SpkAccessor_READ },
    { 0 }
};

static SpkMethodTmpl ObjectMethods[] = {
    { "__eq__", SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &Object_eq },
    { "__ne__", SpkNativeCode_BINARY_OPER, &Object_ne },
    { "compoundExpression", SpkNativeCode_ARGS_ARRAY, &Object_compoundExpression },
    { "printString", SpkNativeCode_ARGS_0, &Object_printString },
    { 0 }
};

static SpkMethodTmpl ClassObjectMethods[] = {
    { "new", SpkNativeCode_ARGS_0, &ClassObject_new },
    { 0 }
};

SpkClassTmpl Spk_ClassObjectTmpl = {
    "Object", offsetof(SpkHeart, Object), 0, {
        ObjectAccessors,
        ObjectMethods,
        /*lvalueMethods*/ 0,
        offsetof(SpkObjectSubclass, variables),
        /*itemSize*/ 0,
        &Object_zero,
        &Object_dealloc
    }, /*meta*/ {
        /*accessors*/ 0,
        ClassObjectMethods,
        /*lvalueMethods*/ 0
    }
};


static SpkMethodTmpl VariableObjectMethods[] = {
    { 0 }
};

static SpkMethodTmpl ClassVariableObjectMethods[] = {
    { "new", SpkNativeCode_ARGS_1, &ClassVariableObject_new },
    { 0 }
};

SpkClassTmpl Spk_ClassVariableObjectTmpl = {
    Spk_HEART_CLASS_TMPL(VariableObject, Object), {
        /*accessors*/ 0,
        VariableObjectMethods,
        /*lvalueMethods*/ 0,
        offsetof(SpkVariableObjectSubclass, variables),
        /*itemSize*/ 0,
        &VariableObject_zero
    }, /*meta*/ {
        /*accessors*/ 0,
        ClassVariableObjectMethods,
        /*lvalueMethods*/ 0
    }
};


/*------------------------------------------------------------------------*/
/* casting */

SpkObject *Spk_Cast(SpkBehavior *target, SpkUnknown *unk) {
    SpkObject *op;
    SpkBehavior *c;
    
#ifdef MALTIPY
    if (!PyObject_TypeCheck(unk, &SpkSpikeObject_Type)) {
        return 0;
    }
#endif /* MALTIPY */
    op = (SpkObject *)unk;
    for (c = op->klass; c; c = c->superclass) {
        if (c == target) {
            return (SpkObject *)op;
        }
    }
    return 0;
}


/*------------------------------------------------------------------------*/
/* object memory */

SpkObject *SpkObject_New(SpkBehavior *klass) {
    SpkObject *newObject;
    
    newObject = (SpkObject *)SpkObjMem_Alloc(klass->instanceSize);
    if (!newObject) {
        return 0;
    }
    newObject->klass = klass;  Spk_INCREF(klass);
    /* XXX: 'zero' should be called 'uninit'; and, for symmetry,
       should be called from SpkObjMem_Alloc() */
    (*klass->zero)(newObject);
    return newObject;
}

SpkObject *SpkObject_NewVar(SpkBehavior *klass, size_t size) {
    SpkObject *newObject;
    
    newObject = (SpkObject *)SpkObjMem_Alloc(klass->instanceSize + size * klass->itemSize);
    if (!newObject) {
        return 0;
    }
    newObject->klass = klass;  Spk_INCREF(klass);
    ((SpkVariableObject *)newObject)->size = size;
    (*klass->zero)(newObject);
    return newObject;
}
