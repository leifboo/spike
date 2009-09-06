
#include "float.h"

#include "bool.h"
#include "class.h"
#include "native.h"


struct SpkFloat {
    SpkObject base;
    double value;
};


#define BOOL(cond) ((cond) ? Spk_true : Spk_false)


SpkBehavior *Spk_ClassFloat;


/*------------------------------------------------------------------------*/
/* method helpers */

static SpkUnknown *Float_unaryOper(SpkFloat *self, SpkOper oper) {
    SpkFloat *result;
    
    result = (SpkFloat *)SpkObject_New(Spk_ClassFloat);
    switch (oper) {
    case Spk_OPER_SUCC: result->value = self->value + 1; break;
    case Spk_OPER_PRED: result->value = self->value - 1; break;
    case Spk_OPER_POS:  result->value = +self->value;    break;
    case Spk_OPER_NEG:  result->value = -self->value;    break;
    default:
        Spk_Halt(Spk_HALT_ASSERTION_ERROR, "not reached");
        return 0;
    }
    return (SpkUnknown *)result;
}

static SpkUnknown *Float_binaryOper(SpkFloat *self, SpkUnknown *arg0, SpkOper oper) {
    SpkFloat *arg, *result;
    
    arg = Spk_CAST(Float, arg0);
    if (!arg) {
        Spk_Halt(Spk_HALT_TYPE_ERROR, "a float is required");
        return 0;
    }
    result = (SpkFloat *)SpkObject_New(Spk_ClassFloat);
    switch (oper) {
    case Spk_OPER_MUL: result->value = self->value * arg->value;  break;
    case Spk_OPER_DIV: result->value = self->value / arg->value;  break;
    case Spk_OPER_ADD: result->value = self->value + arg->value;  break;
    case Spk_OPER_SUB: result->value = self->value - arg->value;  break;
    default:
        Spk_Halt(Spk_HALT_ASSERTION_ERROR, "not reached");
        return 0;
    }
    return (SpkUnknown *)result;
}

static SpkUnknown *Float_binaryLogicalOper(SpkFloat *self, SpkUnknown *arg0, SpkOper oper) {
    SpkFloat *arg;
    SpkUnknown *result;
    
    arg = Spk_CAST(Float, arg0);
    if (!arg) switch (oper) {
    case Spk_OPER_EQ:
        /* XXX: 0.0 == 0 */
        Spk_INCREF(Spk_false);
        return Spk_false;
    case Spk_OPER_NE:
        Spk_INCREF(Spk_true);
        return Spk_true;
    default:
        Spk_Halt(Spk_HALT_ASSERTION_ERROR, "XXX");
        return 0;
    }
    
    switch (oper) {
    case Spk_OPER_LT: result = BOOL(self->value < arg->value);  break;
    case Spk_OPER_GT: result = BOOL(self->value > arg->value);  break;
    case Spk_OPER_LE: result = BOOL(self->value <= arg->value); break;
    case Spk_OPER_GE: result = BOOL(self->value >= arg->value); break;
    case Spk_OPER_EQ: result = BOOL(self->value == arg->value); break;
    case Spk_OPER_NE: result = BOOL(self->value != arg->value); break;
    default:
        Spk_Halt(Spk_HALT_ASSERTION_ERROR, "not reached");
        return 0;
    }
    Spk_INCREF(result);
    return result;
}


/*------------------------------------------------------------------------*/
/* methods -- operators */

/* Spk_OPER_SUCC */
static SpkUnknown *Float_succ(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return Float_unaryOper((SpkFloat *)self, Spk_OPER_SUCC);
}

/* Spk_OPER_PRED */
static SpkUnknown *Float_pred(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return Float_unaryOper((SpkFloat *)self, Spk_OPER_PRED);
}

/* Spk_OPER_POS */
static SpkUnknown *Float_pos(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return Float_unaryOper((SpkFloat *)self, Spk_OPER_POS);
}

/* Spk_OPER_NEG */
static SpkUnknown *Float_neg(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return Float_unaryOper((SpkFloat *)self, Spk_OPER_NEG);
}

/* Spk_OPER_LNEG */
static SpkUnknown *Float_lneg(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    SpkUnknown *result = BOOL(!((SpkFloat *)self)->value);
    Spk_INCREF(result);
    return result;
}

/* Spk_OPER_MUL */
static SpkUnknown *Float_mul(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return Float_binaryOper((SpkFloat *)self, arg0, Spk_OPER_MUL);
}

/* Spk_OPER_DIV */
static SpkUnknown *Float_div(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return Float_binaryOper((SpkFloat *)self, arg0, Spk_OPER_DIV);
}

/* Spk_OPER_ADD */
static SpkUnknown *Float_add(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return Float_binaryOper((SpkFloat *)self, arg0, Spk_OPER_ADD);
}

/* Spk_OPER_SUB */
static SpkUnknown *Float_sub(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return Float_binaryOper((SpkFloat *)self, arg0, Spk_OPER_SUB);
}

/* Spk_OPER_LT */
static SpkUnknown *Float_lt(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return Float_binaryLogicalOper((SpkFloat *)self, arg0, Spk_OPER_LT);
}

/* Spk_OPER_GT */
static SpkUnknown *Float_gt(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return Float_binaryLogicalOper((SpkFloat *)self, arg0, Spk_OPER_GT);
}

/* Spk_OPER_LE */
static SpkUnknown *Float_le(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return Float_binaryLogicalOper((SpkFloat *)self, arg0, Spk_OPER_LE);
}

/* Spk_OPER_GE */
static SpkUnknown *Float_ge(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return Float_binaryLogicalOper((SpkFloat *)self, arg0, Spk_OPER_GE);
}

/* Spk_OPER_EQ */
static SpkUnknown *Float_eq(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return Float_binaryLogicalOper((SpkFloat *)self, arg0, Spk_OPER_EQ);
}

/* Spk_OPER_NE */
static SpkUnknown *Float_ne(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return Float_binaryLogicalOper((SpkFloat *)self, arg0, Spk_OPER_NE);
}


/*------------------------------------------------------------------------*/
/* class template */

typedef struct SpkFloatSubclass {
    SpkFloat base;
    SpkUnknown *variables[1]; /* stretchy */
} SpkFloatSubclass;

static SpkMethodTmpl methods[] = {
    /* operators */
    { "__succ__",   SpkNativeCode_UNARY_OPER  | SpkNativeCode_LEAF, &Float_succ   },
    { "__pred__",   SpkNativeCode_UNARY_OPER  | SpkNativeCode_LEAF, &Float_pred   },
    { "__pos__",    SpkNativeCode_UNARY_OPER  | SpkNativeCode_LEAF, &Float_pos    },
    { "__neg__",    SpkNativeCode_UNARY_OPER  | SpkNativeCode_LEAF, &Float_neg    },
    { "__lneg__",   SpkNativeCode_UNARY_OPER  | SpkNativeCode_LEAF, &Float_lneg   },
    { "__mul__",    SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &Float_mul    },
    { "__div__",    SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &Float_div    },
    { "__add__",    SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &Float_add    },
    { "__sub__",    SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &Float_sub    },
    { "__lt__",     SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &Float_lt     },
    { "__gt__",     SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &Float_gt     },
    { "__le__",     SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &Float_le     },
    { "__ge__",     SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &Float_ge     },
    { "__eq__",     SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &Float_eq     },
    { "__ne__",     SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &Float_ne     },
    { 0, 0, 0}
};

SpkClassTmpl Spk_ClassFloatTmpl = {
    "Float", {
        /*accessors*/ 0,
        methods,
        /*lvalueMethods*/ 0,
        offsetof(SpkFloatSubclass, variables)
    }, /*meta*/ {
    }
};


/*------------------------------------------------------------------------*/
/* C API */

SpkFloat *SpkFloat_FromCDouble(double value) {
    SpkFloat *result;
    
    result = (SpkFloat *)SpkObject_New(Spk_ClassFloat);
    result->value = value;
    return result;
}

double SpkFloat_AsCDouble(SpkFloat *self) {
    return self->value;
}
