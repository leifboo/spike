
#include "bool.h"

#include "behavior.h"
#include "interp.h"
#include "module.h"
#include "obj.h"
#include <stdio.h>
#include <stdlib.h>


Boolean *Spk_false, *Spk_true;
static Behavior *ClassBoolean, *ClassFalse, *ClassTrue;


/*------------------------------------------------------------------------*/
/* methods */

static Object *False_not(Object *self, Object *arg0, Object *arg1) {
    return Spk_true;
}

static Object *False_print(Object *self, Object *arg0, Object *arg1) {
    printf("false");
    return Spk_void;
}

static Object *True_not(Object *self, Object *arg0, Object *arg1) {
    return Spk_false;
}

static Object *True_print(Object *self, Object *arg0, Object *arg1) {
    printf("true");
    return Spk_void;
}


/*------------------------------------------------------------------------*/
/* class templates */

static SpkMethodTmpl FalseMethods[] = {
    { "__not__", SpkNativeCode_ARGS_0 | SpkNativeCode_LEAF, &False_not },
    { "print", SpkNativeCode_ARGS_0 | SpkNativeCode_CALLABLE, &False_print },
    { 0, 0, 0}
};

static SpkClassTmpl FalseTmpl = {
    offsetof(ObjectSubclass, variables),
    sizeof(Boolean),
    0,
    FalseMethods
};


static SpkMethodTmpl TrueMethods[] = {
    { "__not__", SpkNativeCode_ARGS_0 | SpkNativeCode_LEAF, &True_not },
    { "print", SpkNativeCode_ARGS_0 | SpkNativeCode_CALLABLE, &True_print },
    { 0, 0, 0}
};

static SpkClassTmpl TrueTmpl = {
    offsetof(ObjectSubclass, variables),
    sizeof(Boolean),
    0,
    TrueMethods
};


/*------------------------------------------------------------------------*/
/* C API */

void SpkClassBoolean_init(void) {
    ClassBoolean = SpkBehavior_new(ClassObject, builtInModule, 0);
    ClassFalse = SpkBehavior_fromTemplate(&FalseTmpl, ClassBoolean, builtInModule);
    ClassTrue = SpkBehavior_fromTemplate(&TrueTmpl, ClassBoolean, builtInModule);
    
    Spk_false = (Boolean *)malloc(sizeof(Boolean));
    Spk_false->klass = ClassFalse;
    Spk_true = (Boolean *)malloc(sizeof(Boolean));
    Spk_true->klass = ClassTrue;
}
