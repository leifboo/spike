
#include "float.h"

#include "bool.h"
#include "class.h"
#include "heart.h"
#include "native.h"


struct Float {
    Object base;
    double value;
};


#define BOOL(cond) ((cond) ? GLOBAL(xtrue) : GLOBAL(xfalse))


/*------------------------------------------------------------------------*/
/* method helpers */

static Unknown *Float_unaryOper(Float *self, Oper oper) {
    Float *result;
    
    result = (Float *)Object_New(CLASS(Float));
    switch (oper) {
    case OPER_SUCC: result->value = self->value + 1; break;
    case OPER_PRED: result->value = self->value - 1; break;
    case OPER_POS:  result->value = +self->value;    break;
    case OPER_NEG:  result->value = -self->value;    break;
    default:
        Halt(HALT_ASSERTION_ERROR, "not reached");
        return 0;
    }
    return (Unknown *)result;
}

static Unknown *Float_binaryOper(Float *self, Unknown *arg0, Oper oper) {
    Float *arg, *result;
    
    arg = CAST(Float, arg0);
    if (!arg) {
        Halt(HALT_TYPE_ERROR, "a float is required");
        return 0;
    }
    result = (Float *)Object_New(CLASS(Float));
    switch (oper) {
    case OPER_MUL: result->value = self->value * arg->value;  break;
    case OPER_DIV: result->value = self->value / arg->value;  break;
    case OPER_ADD: result->value = self->value + arg->value;  break;
    case OPER_SUB: result->value = self->value - arg->value;  break;
    default:
        Halt(HALT_ASSERTION_ERROR, "not reached");
        return 0;
    }
    return (Unknown *)result;
}

static Unknown *Float_binaryLogicalOper(Float *self, Unknown *arg0, Oper oper) {
    Float *arg;
    Unknown *result;
    
    arg = CAST(Float, arg0);
    if (!arg) switch (oper) {
    case OPER_EQ:
        /* XXX: 0.0 == 0 */
        INCREF(GLOBAL(xfalse));
        return GLOBAL(xfalse);
    case OPER_NE:
        INCREF(GLOBAL(xtrue));
        return GLOBAL(xtrue);
    default:
        Halt(HALT_ASSERTION_ERROR, "XXX");
        return 0;
    }
    
    switch (oper) {
    case OPER_LT: result = BOOL(self->value < arg->value);  break;
    case OPER_GT: result = BOOL(self->value > arg->value);  break;
    case OPER_LE: result = BOOL(self->value <= arg->value); break;
    case OPER_GE: result = BOOL(self->value >= arg->value); break;
    case OPER_EQ: result = BOOL(self->value == arg->value); break;
    case OPER_NE: result = BOOL(self->value != arg->value); break;
    default:
        Halt(HALT_ASSERTION_ERROR, "not reached");
        return 0;
    }
    INCREF(result);
    return result;
}


/*------------------------------------------------------------------------*/
/* methods -- operators */

/* OPER_SUCC */
static Unknown *Float_succ(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return Float_unaryOper((Float *)self, OPER_SUCC);
}

/* OPER_PRED */
static Unknown *Float_pred(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return Float_unaryOper((Float *)self, OPER_PRED);
}

/* OPER_POS */
static Unknown *Float_pos(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return Float_unaryOper((Float *)self, OPER_POS);
}

/* OPER_NEG */
static Unknown *Float_neg(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return Float_unaryOper((Float *)self, OPER_NEG);
}

/* OPER_LNEG */
static Unknown *Float_lneg(Unknown *self, Unknown *arg0, Unknown *arg1) {
    Unknown *result = BOOL(!((Float *)self)->value);
    INCREF(result);
    return result;
}

/* OPER_MUL */
static Unknown *Float_mul(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return Float_binaryOper((Float *)self, arg0, OPER_MUL);
}

/* OPER_DIV */
static Unknown *Float_div(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return Float_binaryOper((Float *)self, arg0, OPER_DIV);
}

/* OPER_ADD */
static Unknown *Float_add(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return Float_binaryOper((Float *)self, arg0, OPER_ADD);
}

/* OPER_SUB */
static Unknown *Float_sub(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return Float_binaryOper((Float *)self, arg0, OPER_SUB);
}

/* OPER_LT */
static Unknown *Float_lt(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return Float_binaryLogicalOper((Float *)self, arg0, OPER_LT);
}

/* OPER_GT */
static Unknown *Float_gt(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return Float_binaryLogicalOper((Float *)self, arg0, OPER_GT);
}

/* OPER_LE */
static Unknown *Float_le(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return Float_binaryLogicalOper((Float *)self, arg0, OPER_LE);
}

/* OPER_GE */
static Unknown *Float_ge(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return Float_binaryLogicalOper((Float *)self, arg0, OPER_GE);
}

/* OPER_EQ */
static Unknown *Float_eq(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return Float_binaryLogicalOper((Float *)self, arg0, OPER_EQ);
}

/* OPER_NE */
static Unknown *Float_ne(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return Float_binaryLogicalOper((Float *)self, arg0, OPER_NE);
}


/*------------------------------------------------------------------------*/
/* class tmpl */

typedef struct FloatSubclass {
    Float base;
    Unknown *variables[1]; /* stretchy */
} FloatSubclass;

static MethodTmpl methods[] = {
    /* operators */
    { "__succ__",   NativeCode_UNARY_OPER  | NativeCode_LEAF, &Float_succ   },
    { "__pred__",   NativeCode_UNARY_OPER  | NativeCode_LEAF, &Float_pred   },
    { "__pos__",    NativeCode_UNARY_OPER  | NativeCode_LEAF, &Float_pos    },
    { "__neg__",    NativeCode_UNARY_OPER  | NativeCode_LEAF, &Float_neg    },
    { "__lneg__",   NativeCode_UNARY_OPER  | NativeCode_LEAF, &Float_lneg   },
    { "__mul__",    NativeCode_BINARY_OPER | NativeCode_LEAF, &Float_mul    },
    { "__div__",    NativeCode_BINARY_OPER | NativeCode_LEAF, &Float_div    },
    { "__add__",    NativeCode_BINARY_OPER | NativeCode_LEAF, &Float_add    },
    { "__sub__",    NativeCode_BINARY_OPER | NativeCode_LEAF, &Float_sub    },
    { "__lt__",     NativeCode_BINARY_OPER | NativeCode_LEAF, &Float_lt     },
    { "__gt__",     NativeCode_BINARY_OPER | NativeCode_LEAF, &Float_gt     },
    { "__le__",     NativeCode_BINARY_OPER | NativeCode_LEAF, &Float_le     },
    { "__ge__",     NativeCode_BINARY_OPER | NativeCode_LEAF, &Float_ge     },
    { "__eq__",     NativeCode_BINARY_OPER | NativeCode_LEAF, &Float_eq     },
    { "__ne__",     NativeCode_BINARY_OPER | NativeCode_LEAF, &Float_ne     },
    { 0 }
};

ClassTmpl ClassFloatTmpl = {
    HEART_CLASS_TMPL(Float, Object), {
        /*accessors*/ 0,
        methods,
        /*lvalueMethods*/ 0,
        offsetof(FloatSubclass, variables)
    }, /*meta*/ {
        0
    }
};


/*------------------------------------------------------------------------*/
/* C API */

Float *Float_FromCDouble(double value) {
    Float *result;
    
    result = (Float *)Object_New(CLASS(Float));
    result->value = value;
    return result;
}

double Float_AsCDouble(Float *self) {
    return self->value;
}
