
#include "obj.h"

#include "behavior.h"
#include "bool.h"
#include "interp.h"
#include "native.h"
#include <stdio.h>


Behavior *ClassObject, *ClassVariableObject;


/*------------------------------------------------------------------------*/
/* methods */

static Object *Object_eq(Object *self, Object *arg0, Object *arg1) {
    return self == arg0 ? Spk_true : Spk_false;
}

static Object *Object_ne(Object *self, Object *arg0, Object *arg1) {
    Object *temp;
    
    temp = Spk_oper(theInterpreter, self, OPER_EQ, arg0, 0);
    temp = Spk_oper(theInterpreter, temp, OPER_LNEG, 0);
    return temp;
}

static Object *Object_class(Object *self, Object *arg0, Object *arg1) {
    return (Object *)self->klass;
}

static Object *Object_print(Object *self, Object *arg0, Object *arg1) {
    printf("<%s instance at %p>", SpkBehavior_name(self->klass), self);
    return Spk_void;
}


/*------------------------------------------------------------------------*/
/* class templates */

static SpkMethodTmpl ObjectMethods[] = {
    { "__eq__", SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &Object_eq },
    { "__ne__", SpkNativeCode_BINARY_OPER, &Object_ne },
    { "class", SpkNativeCode_LEAF, &Object_class },
    { "print", SpkNativeCode_METH_ATTR | SpkNativeCode_ARGS_0, &Object_print },
    { 0, 0, 0}
};

SpkClassTmpl ClassObjectTmpl = {
    "Object",
    offsetof(ObjectSubclass, variables),
    sizeof(Object),
    0,
    0,
    ObjectMethods
};


static SpkMethodTmpl VariableObjectMethods[] = {
    { 0, 0, 0}
};

SpkClassTmpl ClassVariableObjectTmpl = {
    "VariableObject",
    offsetof(VariableObjectSubclass, variables),
    sizeof(VariableObject),
    0,
    0,
    VariableObjectMethods
};
