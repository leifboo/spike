
#include "bool.h"

#include "class.h"
#include "native.h"


SpkUnknown *Spk_false, *Spk_true;
SpkBehavior *Spk_ClassBoolean, *Spk_ClassFalse, *Spk_ClassTrue;


/*------------------------------------------------------------------------*/
/* methods */

static SpkUnknown *False_lneg(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    Spk_INCREF(Spk_true);
    return Spk_true;
}

static SpkUnknown *False_bneg(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    Spk_INCREF(Spk_true);
    return Spk_true;
}

static SpkUnknown *False_band(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    Spk_INCREF(Spk_false);
    return Spk_false;
}

static SpkUnknown *False_bxor(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    Spk_INCREF(arg0);
    return arg0;
}

static SpkUnknown *False_bor(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    Spk_INCREF(arg0);
    return arg0;
}

static SpkUnknown *True_lneg(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    Spk_INCREF(Spk_false);
    return Spk_false;
}

static SpkUnknown *True_bneg(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    Spk_INCREF(Spk_false);
    return Spk_false;
}

static SpkUnknown *True_band(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    Spk_INCREF(arg0);
    return arg0;
}

static SpkUnknown *True_bxor(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return Spk_Oper(theInterpreter, arg0, Spk_OPER_BNEG, 0);
}

static SpkUnknown *True_bor(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    Spk_INCREF(Spk_true);
    return Spk_true;
}


/*------------------------------------------------------------------------*/
/* class templates */

static SpkMethodTmpl BooleanMethods[] = {
    { 0, 0, 0}
};

SpkClassTmpl Spk_ClassBooleanTmpl = {
    "Boolean", {
        /*accessors*/ 0,
        BooleanMethods,
        /*lvalueMethods*/ 0,
        offsetof(SpkObjectSubclass, variables),
    }, /*meta*/ {
    }
};


static SpkMethodTmpl FalseMethods[] = {
    /* operators */
    { "__lneg__", SpkNativeCode_UNARY_OPER  | SpkNativeCode_LEAF, &False_lneg },
    { "__bneg__", SpkNativeCode_UNARY_OPER  | SpkNativeCode_LEAF, &False_bneg },
    { "__band__", SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &False_band },
    { "__bxor__", SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &False_bxor },
    { "__bor__",  SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &False_bor  },
    { 0, 0, 0}
};

SpkClassTmpl Spk_ClassFalseTmpl = {
    "False", {
        /*accessors*/ 0,
        FalseMethods,
        /*lvalueMethods*/ 0,
        offsetof(SpkObjectSubclass, variables)
    }, /*meta*/ {
    }
};


static SpkMethodTmpl TrueMethods[] = {
    /* operators */
    { "__lneg__", SpkNativeCode_UNARY_OPER  | SpkNativeCode_LEAF, &True_lneg },
    { "__bneg__", SpkNativeCode_UNARY_OPER  | SpkNativeCode_LEAF, &True_bneg },
    { "__band__", SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &True_band },
    { "__bxor__", SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &True_bxor },
    { "__bor__",  SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &True_bor  },
    { 0, 0, 0}
};

SpkClassTmpl Spk_ClassTrueTmpl = {
    "True", {
        /*accessors*/ 0,
        TrueMethods,
        /*lvalueMethods*/ 0,
        offsetof(SpkObjectSubclass, variables)
    }, /*meta*/ {
    }
};
