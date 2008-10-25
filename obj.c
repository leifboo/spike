
#include "obj.h"

#include "behavior.h"
#include "dict.h"
#include "module.h"
#include <stdio.h>


Behavior *ClassObject;


/*------------------------------------------------------------------------*/
/* methods */

static Object *Object_print(Object *self, Object *arg0, Object *arg1) {
    printf("<object at %p>", self);
}


/*------------------------------------------------------------------------*/
/* class template */

static SpkMethodTmpl methods[] = {
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
