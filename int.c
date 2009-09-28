
#include "int.h"

#include "bool.h"
#include "class.h"
#include "heart.h"
#include "native.h"
#include "str.h"

#include <stdio.h>


struct SpkInteger {
    SpkObject base;
    ptrdiff_t value;
};


#define BOOL(cond) ((cond) ? Spk_GLOBAL(xtrue) : Spk_GLOBAL(xfalse))


/*------------------------------------------------------------------------*/
/* method helpers */

static SpkUnknown *Integer_unaryOper(SpkInteger *self, SpkOper oper) {
    SpkInteger *result;
    
    result = (SpkInteger *)SpkObject_New(Spk_CLASS(Integer));
    switch (oper) {
    case Spk_OPER_SUCC: result->value = self->value + 1; break;
    case Spk_OPER_PRED: result->value = self->value - 1; break;
    case Spk_OPER_POS:  result->value = +self->value;    break;
    case Spk_OPER_NEG:  result->value = -self->value;    break;
    case Spk_OPER_BNEG: result->value = ~self->value;    break;
    default:
        Spk_Halt(Spk_HALT_ASSERTION_ERROR, "not reached");
        return 0;
    }
    return (SpkUnknown *)result;
}

static SpkUnknown *Integer_binaryOper(SpkInteger *self, SpkUnknown *arg0, SpkOper oper) {
    SpkInteger *arg, *result;
    
    arg = Spk_CAST(Integer, arg0);
    if (!arg) {
        Spk_Halt(Spk_HALT_TYPE_ERROR, "an integer is required");
        return 0;
    }
    result = (SpkInteger *)SpkObject_New(Spk_CLASS(Integer));
    switch (oper) {
    case Spk_OPER_MUL:    result->value = self->value * arg->value;  break;
    case Spk_OPER_DIV:    result->value = self->value / arg->value;  break;
    case Spk_OPER_MOD:    result->value = self->value % arg->value;  break;
    case Spk_OPER_ADD:    result->value = self->value + arg->value;  break;
    case Spk_OPER_SUB:    result->value = self->value - arg->value;  break;
    case Spk_OPER_LSHIFT: result->value = self->value << arg->value; break;
    case Spk_OPER_RSHIFT: result->value = self->value >> arg->value; break;
    case Spk_OPER_BAND:   result->value = self->value & arg->value;  break;
    case Spk_OPER_BXOR:   result->value = self->value ^ arg->value;  break;
    case Spk_OPER_BOR:    result->value = self->value | arg->value;  break;
    default:
        Spk_Halt(Spk_HALT_ASSERTION_ERROR, "not reached");
        return 0;
    }
    return (SpkUnknown *)result;
}

static SpkUnknown *Integer_binaryLogicalOper(SpkInteger *self, SpkUnknown *arg0, SpkOper oper) {
    SpkInteger *arg;
    SpkUnknown *result;
    
    arg = Spk_CAST(Integer, arg0);
    if (!arg) switch (oper) {
    case Spk_OPER_EQ:
        /* XXX: 0 == 0.0 */
        Spk_INCREF(Spk_GLOBAL(xfalse));
        return Spk_GLOBAL(xfalse);
    case Spk_OPER_NE:
        Spk_INCREF(Spk_GLOBAL(xtrue));
        return Spk_GLOBAL(xtrue);
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
static SpkUnknown *Integer_succ(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return Integer_unaryOper((SpkInteger *)self, Spk_OPER_SUCC);
}

/* Spk_OPER_PRED */
static SpkUnknown *Integer_pred(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return Integer_unaryOper((SpkInteger *)self, Spk_OPER_PRED);
}

/* Spk_OPER_POS */
static SpkUnknown *Integer_pos(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return Integer_unaryOper((SpkInteger *)self, Spk_OPER_POS);
}

/* Spk_OPER_NEG */
static SpkUnknown *Integer_neg(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return Integer_unaryOper((SpkInteger *)self, Spk_OPER_NEG);
}

/* Spk_OPER_BNEG */
static SpkUnknown *Integer_bneg(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return Integer_unaryOper((SpkInteger *)self, Spk_OPER_BNEG);
}

/* Spk_OPER_LNEG */
static SpkUnknown *Integer_lneg(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    SpkUnknown *result = BOOL(!((SpkInteger *)self)->value);
    Spk_INCREF(result);
    return result;
}

/* Spk_OPER_MUL */
static SpkUnknown *Integer_mul(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return Integer_binaryOper((SpkInteger *)self, arg0, Spk_OPER_MUL);
}

/* Spk_OPER_DIV */
static SpkUnknown *Integer_div(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return Integer_binaryOper((SpkInteger *)self, arg0, Spk_OPER_DIV);
}

/* Spk_OPER_MOD */
static SpkUnknown *Integer_mod(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return Integer_binaryOper((SpkInteger *)self, arg0, Spk_OPER_MOD);
}

/* Spk_OPER_ADD */
static SpkUnknown *Integer_add(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return Integer_binaryOper((SpkInteger *)self, arg0, Spk_OPER_ADD);
}

/* Spk_OPER_SUB */
static SpkUnknown *Integer_sub(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return Integer_binaryOper((SpkInteger *)self, arg0, Spk_OPER_SUB);
}

/* Spk_OPER_LSHIFT */
static SpkUnknown *Integer_lshift(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return Integer_binaryOper((SpkInteger *)self, arg0, Spk_OPER_LSHIFT);
}

/* Spk_OPER_RSHIFT */
static SpkUnknown *Integer_rshift(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return Integer_binaryOper((SpkInteger *)self, arg0, Spk_OPER_RSHIFT);
}

/* Spk_OPER_LT */
static SpkUnknown *Integer_lt(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return Integer_binaryLogicalOper((SpkInteger *)self, arg0, Spk_OPER_LT);
}

/* Spk_OPER_GT */
static SpkUnknown *Integer_gt(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return Integer_binaryLogicalOper((SpkInteger *)self, arg0, Spk_OPER_GT);
}

/* Spk_OPER_LE */
static SpkUnknown *Integer_le(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return Integer_binaryLogicalOper((SpkInteger *)self, arg0, Spk_OPER_LE);
}

/* Spk_OPER_GE */
static SpkUnknown *Integer_ge(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return Integer_binaryLogicalOper((SpkInteger *)self, arg0, Spk_OPER_GE);
}

/* Spk_OPER_EQ */
static SpkUnknown *Integer_eq(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return Integer_binaryLogicalOper((SpkInteger *)self, arg0, Spk_OPER_EQ);
}

/* Spk_OPER_NE */
static SpkUnknown *Integer_ne(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return Integer_binaryLogicalOper((SpkInteger *)self, arg0, Spk_OPER_NE);
}

/* Spk_OPER_BAND */
static SpkUnknown *Integer_band(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return Integer_binaryOper((SpkInteger *)self, arg0, Spk_OPER_BAND);
}

/* Spk_OPER_BXOR */
static SpkUnknown *Integer_bxor(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return Integer_binaryOper((SpkInteger *)self, arg0, Spk_OPER_BXOR);
}

/* Spk_OPER_BOR */
static SpkUnknown *Integer_bor(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return Integer_binaryOper((SpkInteger *)self, arg0, Spk_OPER_BOR);
}


/*------------------------------------------------------------------------*/
/* methods -- printing */

static SpkUnknown *Integer_printString(SpkUnknown *_self, SpkUnknown *arg0, SpkUnknown *arg1) {
    SpkInteger *self;
    SpkString *result;
    char *str;
    
    self = (SpkInteger *)_self;
    result = SpkString_FromCStringAndLength(0, 4*sizeof(self->value));
    if (!result)
        return 0;
    str = SpkString_AsCString(result);
    sprintf(str, "%ld", (long)self->value);
    return (SpkUnknown *)result;
}


/*------------------------------------------------------------------------*/
/* class tmpl */

typedef struct SpkIntegerSubclass {
    SpkInteger base;
    SpkUnknown *variables[1]; /* stretchy */
} SpkIntegerSubclass;

static SpkMethodTmpl methods[] = {
    /* operators */
    { "__succ__",   SpkNativeCode_UNARY_OPER  | SpkNativeCode_LEAF, &Integer_succ   },
    { "__pred__",   SpkNativeCode_UNARY_OPER  | SpkNativeCode_LEAF, &Integer_pred   },
    { "__pos__",    SpkNativeCode_UNARY_OPER  | SpkNativeCode_LEAF, &Integer_pos    },
    { "__neg__",    SpkNativeCode_UNARY_OPER  | SpkNativeCode_LEAF, &Integer_neg    },
    { "__bneg__",   SpkNativeCode_UNARY_OPER  | SpkNativeCode_LEAF, &Integer_bneg   },
    { "__lneg__",   SpkNativeCode_UNARY_OPER  | SpkNativeCode_LEAF, &Integer_lneg   },
    { "__mul__",    SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &Integer_mul    },
    { "__div__",    SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &Integer_div    },
    { "__mod__",    SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &Integer_mod    },
    { "__add__",    SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &Integer_add    },
    { "__sub__",    SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &Integer_sub    },
    { "__lshift__", SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &Integer_lshift },
    { "__rshift__", SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &Integer_rshift },
    { "__lt__",     SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &Integer_lt     },
    { "__gt__",     SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &Integer_gt     },
    { "__le__",     SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &Integer_le     },
    { "__ge__",     SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &Integer_ge     },
    { "__eq__",     SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &Integer_eq     },
    { "__ne__",     SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &Integer_ne     },
    { "__band__",   SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &Integer_band   },
    { "__bxor__",   SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &Integer_bxor   },
    { "__bor__",    SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &Integer_bor    },
    /* printing */
    { "printString", SpkNativeCode_ARGS_0, &Integer_printString },
    { 0 }
};

SpkClassTmpl Spk_ClassIntegerTmpl = {
    Spk_HEART_CLASS_TMPL(Integer, Object), {
        /*accessors*/ 0,
        methods,
        /*lvalueMethods*/ 0,
        offsetof(SpkIntegerSubclass, variables)
    }, /*meta*/ {
        0
    }
};


/*------------------------------------------------------------------------*/
/* C API */

SpkInteger *SpkInteger_FromCLong(long value) {
    return SpkInteger_FromCPtrdiff((ptrdiff_t)value);
}

SpkInteger *SpkInteger_FromCPtrdiff(ptrdiff_t value) {
    SpkInteger *result;
    
    result = (SpkInteger *)SpkObject_New(Spk_CLASS(Integer));
    result->value = value;
    return result;
}

long SpkInteger_AsCLong(SpkInteger *anInteger) {
    return (long)anInteger->value;
}

ptrdiff_t SpkInteger_AsCPtrdiff(SpkInteger *anInteger) {
    return anInteger->value;
}
