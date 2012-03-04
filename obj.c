
#include "obj.h"

#include "behavior.h"
#include "bool.h"
#include "class.h"
#include "heart.h"
#include "host.h"
#include "int.h"
#include "interp.h"
#include "native.h"
#include "str.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*------------------------------------------------------------------------*/
/* methods */

static Unknown *Object_eq(Unknown *self, Unknown *arg0, Unknown *arg1) {
    Unknown *result;

    result = (self == arg0 ? GLOBAL(xtrue) : GLOBAL(xfalse));
    INCREF(result);
    return result;
}

static Unknown *Object_ne(Unknown *self, Unknown *arg0, Unknown *arg1) {
    Unknown *temp, *result;
    
    temp = SendOper(GLOBAL(theInterpreter), self, OPER_EQ, arg0, 0);
    result = SendOper(GLOBAL(theInterpreter), temp, OPER_LNEG, 0);
    DECREF(temp);
    return result;
}

static Unknown *Object_compoundExpression(Unknown *self, Unknown *arg0, Unknown *arg1) {
    INCREF(arg0);
    return arg0;
}

static Unknown *Object_id(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return (Unknown *)Integer_FromCPtrdiff((char *)self - (char *)0);
}

static Unknown *Object_printString(Unknown *self, Unknown *arg0, Unknown *arg1) {
    static const char *format = "<%s instance at %p>";
    const char *className;
    String *result;
    size_t len;
    char *str;
    
    className = Behavior_NameAsCString(((Object *)self)->klass);
    len = strlen(format) + strlen(className) + 2*sizeof(void*); /* assumes %p is hex */
    result = String_FromCStringAndLength(0, len);
    if (!result)
        return 0;
    str = String_AsCString(result);
    sprintf(str, format, className, self);
    result->size = strlen(str) + 1;
    return (Unknown *)result;
}


/*------------------------------------------------------------------------*/
/* meta-methods */

static Unknown *ClassObject_new(Unknown *self, Unknown *arg0, Unknown *arg1) {
    /* Answer a new instance of the receiver. */
    return (Unknown *)Object_New((Behavior *)self);
}

static Unknown *ClassVariableObject_new(Unknown *_self, Unknown *nItemsObj, Unknown *arg1) {
    /* Answer a new instance of the receiver. */
    Class *self;
    long nItems;
    
    self = (Class *)_self;
    if (self->base.itemSize == 0) {
        Halt(HALT_VALUE_ERROR, "bad item size in class object");
        return 0;
    }
    if (!Host_IsInteger(nItemsObj)) {
        Halt(HALT_TYPE_ERROR, "an integer object is required");
        return 0;
    }
    nItems = Host_IntegerAsCLong(nItemsObj);
    if (nItems < 0) {
        Halt(HALT_VALUE_ERROR, "number of items cannot be negative");
        return 0;
    }
    
    return (Unknown *)Object_NewVar((Behavior *)self, nItems);
}


/*------------------------------------------------------------------------*/
/* low-level hooks */

static void Object_zero(Object *self) {
    Unknown **variables;
    Behavior *klass;
    size_t instVarTotal, i;
    
    klass = self->klass;
    
    variables = (Unknown **)((char *)self + klass->instVarOffset);
    instVarTotal = klass->instVarBaseIndex + klass->instVarCount;
    
    for (i = 0; i < instVarTotal; ++i) {
        INCREF(GLOBAL(uninit));
        variables[i] = GLOBAL(uninit);
    }
}

static void Object_dealloc(Object *self, Unknown **l) {
    Unknown **variables;
    Behavior *klass;
    size_t instVarTotal, i;
    
    klass = self->klass;
    
    variables = (Unknown **)((char *)self + klass->instVarOffset);
    instVarTotal = klass->instVarBaseIndex + klass->instVarCount;
    
    for (i = 0; i < instVarTotal; ++i) {
        LDECREF(variables[i], l);
    }
    
    LDECREF(klass, l);
    self->klass = 0;
}

static void VariableObject_zero(Object *_self) {
    VariableObject *self = (VariableObject *)_self;
    (*CLASS(VariableObject)->superclass->zero)(_self);
    memset(VariableObject_ITEM_BASE(self), 0,
           self->size * self->base.klass->itemSize);
}


/*------------------------------------------------------------------------*/
/* class templates */

static AccessorTmpl ObjectAccessors[] = {
    { "class", T_OBJECT, offsetof(Object, klass), Accessor_READ },
    { 0 }
};

static MethodTmpl ObjectMethods[] = {
    { "__eq__", NativeCode_BINARY_OPER | NativeCode_LEAF, &Object_eq },
    { "__ne__", NativeCode_BINARY_OPER, &Object_ne },
    { "compoundExpression", NativeCode_ARGS_ARRAY, &Object_compoundExpression },
    { "id", NativeCode_ARGS_0, &Object_id },
    { "printString", NativeCode_ARGS_0, &Object_printString },
    { 0 }
};

static MethodTmpl ClassObjectMethods[] = {
    { "new", NativeCode_ARGS_0, &ClassObject_new },
    { 0 }
};

ClassTmpl ClassObjectTmpl = {
    "Object", offsetof(Heart, Object), 0, {
        ObjectAccessors,
        ObjectMethods,
        /*lvalueMethods*/ 0,
        offsetof(ObjectSubclass, variables),
        /*itemSize*/ 0,
        &Object_zero,
        &Object_dealloc
    }, /*meta*/ {
        /*accessors*/ 0,
        ClassObjectMethods,
        /*lvalueMethods*/ 0
    }
};


static MethodTmpl VariableObjectMethods[] = {
    { 0 }
};

static MethodTmpl ClassVariableObjectMethods[] = {
    { "new", NativeCode_ARGS_1, &ClassVariableObject_new },
    { 0 }
};

ClassTmpl ClassVariableObjectTmpl = {
    HEART_CLASS_TMPL(VariableObject, Object), {
        /*accessors*/ 0,
        VariableObjectMethods,
        /*lvalueMethods*/ 0,
        offsetof(VariableObjectSubclass, variables),
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

Object *Cast(Behavior *target, Unknown *unk) {
    Object *op;
    Behavior *c;
    
    op = (Object *)unk;
    for (c = op->klass; c; c = c->superclass) {
        if (c == target) {
            return (Object *)op;
        }
    }
    return 0;
}


/*------------------------------------------------------------------------*/
/* object memory */

Object *Object_New(Behavior *klass) {
    Object *newObject;
    
    newObject = (Object *)ObjMem_Alloc(klass->instanceSize);
    if (!newObject) {
        return 0;
    }
    newObject->klass = klass;  INCREF(klass);
    /* XXX: 'zero' should be called 'uninit'; and, for symmetry,
       should be called from ObjMem_Alloc() */
    (*klass->zero)(newObject);
    return newObject;
}

Object *Object_NewVar(Behavior *klass, size_t size) {
    Object *newObject;
    
    newObject = (Object *)ObjMem_Alloc(klass->instanceSize + size * klass->itemSize);
    if (!newObject) {
        return 0;
    }
    newObject->klass = klass;  INCREF(klass);
    ((VariableObject *)newObject)->size = size;
    (*klass->zero)(newObject);
    return newObject;
}
