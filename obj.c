
#include "obj.h"

#include "behavior.h"
#include "bool.h"
#include <stdio.h>


Behavior *ClassObject;


/*------------------------------------------------------------------------*/
/* methods */

static Object *Object_eq(Object *self, Object *arg0, Object *arg1) {
    return self == arg0 ? Spk_true : Spk_false;
}

static Object *Object_ne(Object *self, Object *arg0, Object *arg1) {
    Object *temp;
    
    /* XXX: Assumes native code.  Should 'nativeCode' point to a
     * trampoline for interpreted methods?
     */
    temp = (*self->klass->operTable[OPER_EQ].method->nativeCode)(self, arg0, 0);
    temp = (*temp->klass->operTable[OPER_LNEG].method->nativeCode)(temp, 0, 0);
    return temp;
}

static Object *Object_print(Object *self, Object *arg0, Object *arg1) {
    printf("<%s instance at %p>", SpkBehavior_name(self->klass), self);
    return Spk_void;
}


/*------------------------------------------------------------------------*/
/* class template */

static SpkMethodTmpl methods[] = {
    { "__eq__", SpkNativeCode_ARGS_1 | SpkNativeCode_LEAF, &Object_eq },
    { "__ne__", SpkNativeCode_ARGS_1, &Object_ne },
    { "print", SpkNativeCode_ARGS_0 | SpkNativeCode_CALLABLE, &Object_print },
    { 0, 0, 0}
};

SpkClassTmpl ClassObjectTmpl = {
    "Object",
    offsetof(ObjectSubclass, variables),
    sizeof(Object),
    0,
    methods
};
