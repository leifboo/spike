
#include "obj.h"

#include "behavior.h"
#include "bool.h"
#include "dict.h"
#include "module.h"
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
    printf("<object at %p>", self);
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

static SpkClassTmpl tmpl = {
    offsetof(ModuleSubclass, variables),
    sizeof(Module),
    0,
    methods
};


/*------------------------------------------------------------------------*/
/* C API */

void SpkClassObject_init(void) {
    ClassObject = SpkBehavior_new(0, 0 /*builtInModule*/, 0);
}

void SpkClassObject_init2(void) {
    ClassObject->base.klass = ClassBehavior;
    ClassObject->module = builtInModule;
    ClassObject->methodDict->base.klass = ClassIdentityDictionary;
    SpkBehavior_initFromTemplate(ClassObject, &tmpl);
}
