
#include "bool.h"

#include "behavior.h"
#include "interp.h"
#include <stdio.h>


Boolean *Spk_false, *Spk_true;
Behavior *ClassBoolean, *ClassFalse, *ClassTrue;


/*------------------------------------------------------------------------*/
/* methods */

static Object *False_lneg(Object *self, Object *arg0, Object *arg1) {
    return Spk_true;
}

static Object *False_bneg(Object *self, Object *arg0, Object *arg1) {
    return Spk_true;
}

static Object *False_band(Object *self, Object *arg0, Object *arg1) {
    return Spk_false;
}

static Object *False_bxor(Object *self, Object *arg0, Object *arg1) {
    return arg0;
}

static Object *False_bor(Object *self, Object *arg0, Object *arg1) {
    return arg0;
}

static Object *False_print(Object *self, Object *arg0, Object *arg1) {
    printf("false");
    return Spk_void;
}

static Object *True_lneg(Object *self, Object *arg0, Object *arg1) {
    return Spk_false;
}

static Object *True_bneg(Object *self, Object *arg0, Object *arg1) {
    return Spk_false;
}

static Object *True_band(Object *self, Object *arg0, Object *arg1) {
    return arg0;
}

static Object *True_bxor(Object *self, Object *arg0, Object *arg1) {
    /* XXX: Assumes native code. */
    return (*arg0->klass->operTable[OPER_BNEG].method->nativeCode)(arg0, 0, 0);
}

static Object *True_bor(Object *self, Object *arg0, Object *arg1) {
    return Spk_true;
}

static Object *True_print(Object *self, Object *arg0, Object *arg1) {
    printf("true");
    return Spk_void;
}


/*------------------------------------------------------------------------*/
/* class templates */

static SpkMethodTmpl BooleanMethods[] = {
    { 0, 0, 0}
};

SpkClassTmpl ClassBooleanTmpl = {
    "Boolean",
    offsetof(ObjectSubclass, variables),
    sizeof(Boolean),
    0,
    0,
    BooleanMethods
};


static SpkMethodTmpl FalseMethods[] = {
    /* operators */
    { "__lneg__", SpkNativeCode_ARGS_0 | SpkNativeCode_LEAF, &False_lneg },
    { "__bneg__", SpkNativeCode_ARGS_0 | SpkNativeCode_LEAF, &False_bneg },
    { "__band__", SpkNativeCode_ARGS_1 | SpkNativeCode_LEAF, &False_band },
    { "__bxor__", SpkNativeCode_ARGS_1 | SpkNativeCode_LEAF, &False_bxor },
    { "__bor__",  SpkNativeCode_ARGS_1 | SpkNativeCode_LEAF, &False_bor  },
    /* other */
    { "print", SpkNativeCode_ARGS_0 | SpkNativeCode_CALLABLE, &False_print },
    { 0, 0, 0}
};

SpkClassTmpl ClassFalseTmpl = {
    "False",
    offsetof(ObjectSubclass, variables),
    sizeof(Boolean),
    0,
    0,
    FalseMethods
};


static SpkMethodTmpl TrueMethods[] = {
    /* operators */
    { "__lneg__", SpkNativeCode_ARGS_0 | SpkNativeCode_LEAF, &True_lneg },
    { "__bneg__", SpkNativeCode_ARGS_0 | SpkNativeCode_LEAF, &True_bneg },
    { "__band__", SpkNativeCode_ARGS_1 | SpkNativeCode_LEAF, &True_band },
    { "__bxor__", SpkNativeCode_ARGS_1 | SpkNativeCode_LEAF, &True_bxor },
    { "__bor__",  SpkNativeCode_ARGS_1 | SpkNativeCode_LEAF, &True_bor  },
    /* other */
    { "print", SpkNativeCode_ARGS_0 | SpkNativeCode_CALLABLE, &True_print },
    { 0, 0, 0}
};

SpkClassTmpl ClassTrueTmpl = {
    "True",
    offsetof(ObjectSubclass, variables),
    sizeof(Boolean),
    0,
    0,
    TrueMethods
};
