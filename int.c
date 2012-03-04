
#include "int.h"

#include "bool.h"
#include "class.h"
#include "heart.h"
#include "native.h"
#include "str.h"

#include <stdio.h>
#include <string.h>


struct Integer {
    Object base;
    ptrdiff_t value;
};


#define BOOL(cond) ((cond) ? GLOBAL(xtrue) : GLOBAL(xfalse))


/*------------------------------------------------------------------------*/
/* method helpers */

static Unknown *Integer_unaryOper(Integer *self, Oper oper) {
    Integer *result;
    
    result = (Integer *)Object_New(CLASS(Integer));
    switch (oper) {
    case OPER_SUCC: result->value = self->value + 1; break;
    case OPER_PRED: result->value = self->value - 1; break;
    case OPER_POS:  result->value = +self->value;    break;
    case OPER_NEG:  result->value = -self->value;    break;
    case OPER_BNEG: result->value = ~self->value;    break;
    default:
        Halt(HALT_ASSERTION_ERROR, "not reached");
        return 0;
    }
    return (Unknown *)result;
}

static Unknown *Integer_binaryOper(Integer *self, Unknown *arg0, Oper oper) {
    Integer *arg, *result;
    
    arg = CAST(Integer, arg0);
    if (!arg) {
        Halt(HALT_TYPE_ERROR, "an integer is required");
        return 0;
    }
    result = (Integer *)Object_New(CLASS(Integer));
    switch (oper) {
    case OPER_MUL:    result->value = self->value * arg->value;  break;
    case OPER_DIV:    result->value = self->value / arg->value;  break;
    case OPER_MOD:    result->value = self->value % arg->value;  break;
    case OPER_ADD:    result->value = self->value + arg->value;  break;
    case OPER_SUB:    result->value = self->value - arg->value;  break;
    case OPER_LSHIFT: result->value = self->value << arg->value; break;
    case OPER_RSHIFT: result->value = self->value >> arg->value; break;
    case OPER_BAND:   result->value = self->value & arg->value;  break;
    case OPER_BXOR:   result->value = self->value ^ arg->value;  break;
    case OPER_BOR:    result->value = self->value | arg->value;  break;
    default:
        Halt(HALT_ASSERTION_ERROR, "not reached");
        return 0;
    }
    return (Unknown *)result;
}

static Unknown *Integer_binaryLogicalOper(Integer *self, Unknown *arg0, Oper oper) {
    Integer *arg;
    Unknown *result;
    
    arg = CAST(Integer, arg0);
    if (!arg) switch (oper) {
    case OPER_EQ:
        /* XXX: 0 == 0.0 */
        return GLOBAL(xfalse);
    case OPER_NE:
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
    return result;
}


/*------------------------------------------------------------------------*/
/* methods -- operators */

/* OPER_SUCC */
static Unknown *Integer_succ(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return Integer_unaryOper((Integer *)self, OPER_SUCC);
}

/* OPER_PRED */
static Unknown *Integer_pred(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return Integer_unaryOper((Integer *)self, OPER_PRED);
}

/* OPER_POS */
static Unknown *Integer_pos(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return Integer_unaryOper((Integer *)self, OPER_POS);
}

/* OPER_NEG */
static Unknown *Integer_neg(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return Integer_unaryOper((Integer *)self, OPER_NEG);
}

/* OPER_BNEG */
static Unknown *Integer_bneg(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return Integer_unaryOper((Integer *)self, OPER_BNEG);
}

/* OPER_LNEG */
static Unknown *Integer_lneg(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return BOOL(!((Integer *)self)->value);
}

/* OPER_MUL */
static Unknown *Integer_mul(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return Integer_binaryOper((Integer *)self, arg0, OPER_MUL);
}

/* OPER_DIV */
static Unknown *Integer_div(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return Integer_binaryOper((Integer *)self, arg0, OPER_DIV);
}

/* OPER_MOD */
static Unknown *Integer_mod(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return Integer_binaryOper((Integer *)self, arg0, OPER_MOD);
}

/* OPER_ADD */
static Unknown *Integer_add(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return Integer_binaryOper((Integer *)self, arg0, OPER_ADD);
}

/* OPER_SUB */
static Unknown *Integer_sub(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return Integer_binaryOper((Integer *)self, arg0, OPER_SUB);
}

/* OPER_LSHIFT */
static Unknown *Integer_lshift(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return Integer_binaryOper((Integer *)self, arg0, OPER_LSHIFT);
}

/* OPER_RSHIFT */
static Unknown *Integer_rshift(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return Integer_binaryOper((Integer *)self, arg0, OPER_RSHIFT);
}

/* OPER_LT */
static Unknown *Integer_lt(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return Integer_binaryLogicalOper((Integer *)self, arg0, OPER_LT);
}

/* OPER_GT */
static Unknown *Integer_gt(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return Integer_binaryLogicalOper((Integer *)self, arg0, OPER_GT);
}

/* OPER_LE */
static Unknown *Integer_le(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return Integer_binaryLogicalOper((Integer *)self, arg0, OPER_LE);
}

/* OPER_GE */
static Unknown *Integer_ge(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return Integer_binaryLogicalOper((Integer *)self, arg0, OPER_GE);
}

/* OPER_EQ */
static Unknown *Integer_eq(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return Integer_binaryLogicalOper((Integer *)self, arg0, OPER_EQ);
}

/* OPER_NE */
static Unknown *Integer_ne(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return Integer_binaryLogicalOper((Integer *)self, arg0, OPER_NE);
}

/* OPER_BAND */
static Unknown *Integer_band(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return Integer_binaryOper((Integer *)self, arg0, OPER_BAND);
}

/* OPER_BXOR */
static Unknown *Integer_bxor(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return Integer_binaryOper((Integer *)self, arg0, OPER_BXOR);
}

/* OPER_BOR */
static Unknown *Integer_bor(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return Integer_binaryOper((Integer *)self, arg0, OPER_BOR);
}


/*------------------------------------------------------------------------*/
/* methods -- printing */

static Unknown *Integer_printString(Unknown *_self, Unknown *arg0, Unknown *arg1) {
    Integer *self;
    String *result;
    char *str;
    
    self = (Integer *)_self;
    result = String_FromCStringAndLength(0, 4*sizeof(self->value));
    if (!result)
        return 0;
    str = String_AsCString(result);
    sprintf(str, "%ld", (long)self->value);
    result->size = strlen(str) + 1;
    return (Unknown *)result;
}


/*------------------------------------------------------------------------*/
/* class tmpl */

typedef struct IntegerSubclass {
    Integer base;
    Unknown *variables[1]; /* stretchy */
} IntegerSubclass;

static MethodTmpl methods[] = {
    /* operators */
    { "__succ__",   NativeCode_UNARY_OPER  | NativeCode_LEAF, &Integer_succ   },
    { "__pred__",   NativeCode_UNARY_OPER  | NativeCode_LEAF, &Integer_pred   },
    { "__pos__",    NativeCode_UNARY_OPER  | NativeCode_LEAF, &Integer_pos    },
    { "__neg__",    NativeCode_UNARY_OPER  | NativeCode_LEAF, &Integer_neg    },
    { "__bneg__",   NativeCode_UNARY_OPER  | NativeCode_LEAF, &Integer_bneg   },
    { "__lneg__",   NativeCode_UNARY_OPER  | NativeCode_LEAF, &Integer_lneg   },
    { "__mul__",    NativeCode_BINARY_OPER | NativeCode_LEAF, &Integer_mul    },
    { "__div__",    NativeCode_BINARY_OPER | NativeCode_LEAF, &Integer_div    },
    { "__mod__",    NativeCode_BINARY_OPER | NativeCode_LEAF, &Integer_mod    },
    { "__add__",    NativeCode_BINARY_OPER | NativeCode_LEAF, &Integer_add    },
    { "__sub__",    NativeCode_BINARY_OPER | NativeCode_LEAF, &Integer_sub    },
    { "__lshift__", NativeCode_BINARY_OPER | NativeCode_LEAF, &Integer_lshift },
    { "__rshift__", NativeCode_BINARY_OPER | NativeCode_LEAF, &Integer_rshift },
    { "__lt__",     NativeCode_BINARY_OPER | NativeCode_LEAF, &Integer_lt     },
    { "__gt__",     NativeCode_BINARY_OPER | NativeCode_LEAF, &Integer_gt     },
    { "__le__",     NativeCode_BINARY_OPER | NativeCode_LEAF, &Integer_le     },
    { "__ge__",     NativeCode_BINARY_OPER | NativeCode_LEAF, &Integer_ge     },
    { "__eq__",     NativeCode_BINARY_OPER | NativeCode_LEAF, &Integer_eq     },
    { "__ne__",     NativeCode_BINARY_OPER | NativeCode_LEAF, &Integer_ne     },
    { "__band__",   NativeCode_BINARY_OPER | NativeCode_LEAF, &Integer_band   },
    { "__bxor__",   NativeCode_BINARY_OPER | NativeCode_LEAF, &Integer_bxor   },
    { "__bor__",    NativeCode_BINARY_OPER | NativeCode_LEAF, &Integer_bor    },
    /* printing */
    { "printString", NativeCode_ARGS_0, &Integer_printString },
    { 0 }
};

ClassTmpl ClassIntegerTmpl = {
    HEART_CLASS_TMPL(Integer, Object), {
        /*accessors*/ 0,
        methods,
        /*lvalueMethods*/ 0,
        offsetof(IntegerSubclass, variables)
    }, /*meta*/ {
        0
    }
};


/*------------------------------------------------------------------------*/
/* C API */

Integer *Integer_FromCLong(long value) {
    return Integer_FromCPtrdiff((ptrdiff_t)value);
}

Integer *Integer_FromCPtrdiff(ptrdiff_t value) {
    Integer *result;
    
    result = (Integer *)Object_New(CLASS(Integer));
    result->value = value;
    return result;
}

long Integer_AsCLong(Integer *anInteger) {
    return (long)anInteger->value;
}

ptrdiff_t Integer_AsCPtrdiff(Integer *anInteger) {
    return anInteger->value;
}
