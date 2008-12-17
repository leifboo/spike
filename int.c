
#include "int.h"

#include "behavior.h"
#include "bool.h"
#include "interp.h"
#include "module.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>


#define BOOL(cond) ((cond) ? Spk_true : Spk_false)


Behavior *Spk_ClassInteger;


/*------------------------------------------------------------------------*/
/* method helpers */

static Object *Integer_unaryOper(Integer *self, Oper oper) {
    Integer *result;
    
    result = (Integer *)malloc(sizeof(Integer));
    result->base.klass = Spk_ClassInteger;
    switch (oper) {
    case OPER_SUCC: result->value = self->value + 1; break;
    case OPER_PRED: result->value = self->value - 1; break;
    case OPER_POS:  result->value = +self->value;    break;
    case OPER_NEG:  result->value = -self->value;    break;
    case OPER_BNEG: result->value = ~self->value;    break;
    default: assert(0);
    }
    return (Object *)result;
}

static Object *Integer_binaryOper(Integer *self, Object *arg0, Oper oper) {
    Integer *arg, *result;
    
    assert(arg = Spk_CAST(Integer, arg0)); /* XXX */
    result = (Integer *)malloc(sizeof(Integer));
    result->base.klass = Spk_ClassInteger;
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
    default: assert(0);
    }
    return (Object *)result;
}

static Object *Integer_binaryLogicalOper(Integer *self, Object *arg0, Oper oper) {
    Integer *arg;
    Boolean *result;
    
    assert(arg = Spk_CAST(Integer, arg0)); /* XXX */
    switch (oper) {
    case OPER_LT: result = BOOL(self->value < arg->value);  break;
    case OPER_GT: result = BOOL(self->value > arg->value);  break;
    case OPER_LE: result = BOOL(self->value <= arg->value); break;
    case OPER_GE: result = BOOL(self->value >= arg->value); break;
    case OPER_EQ: result = BOOL(self->value == arg->value); break;
    case OPER_NE: result = BOOL(self->value != arg->value); break;
    default: assert(0);
    }
    return (Object *)result;
}


/*------------------------------------------------------------------------*/
/* methods -- operators */

/* OPER_SUCC */
static Object *Integer_succ(Object *self, Object *arg0, Object *arg1) {
    return Integer_unaryOper((Integer *)self, OPER_SUCC);
}

/* OPER_PRED */
static Object *Integer_pred(Object *self, Object *arg0, Object *arg1) {
    return Integer_unaryOper((Integer *)self, OPER_PRED);
}

/* OPER_POS */
static Object *Integer_pos(Object *self, Object *arg0, Object *arg1) {
    return Integer_unaryOper((Integer *)self, OPER_POS);
}

/* OPER_NEG */
static Object *Integer_neg(Object *self, Object *arg0, Object *arg1) {
    return Integer_unaryOper((Integer *)self, OPER_NEG);
}

/* OPER_BNEG */
static Object *Integer_bneg(Object *self, Object *arg0, Object *arg1) {
    return Integer_unaryOper((Integer *)self, OPER_BNEG);
}

/* OPER_LNEG */
static Object *Integer_lneg(Object *self, Object *arg0, Object *arg1) {
    return BOOL(!((Integer *)self)->value);
}

/* OPER_MUL */
static Object *Integer_mul(Object *self, Object *arg0, Object *arg1) {
    return Integer_binaryOper((Integer *)self, arg0, OPER_MUL);
}

/* OPER_DIV */
static Object *Integer_div(Object *self, Object *arg0, Object *arg1) {
    return Integer_binaryOper((Integer *)self, arg0, OPER_DIV);
}

/* OPER_MOD */
static Object *Integer_mod(Object *self, Object *arg0, Object *arg1) {
    return Integer_binaryOper((Integer *)self, arg0, OPER_MOD);
}

/* OPER_ADD */
static Object *Integer_add(Object *self, Object *arg0, Object *arg1) {
    return Integer_binaryOper((Integer *)self, arg0, OPER_ADD);
}

/* OPER_SUB */
static Object *Integer_sub(Object *self, Object *arg0, Object *arg1) {
    return Integer_binaryOper((Integer *)self, arg0, OPER_SUB);
}

/* OPER_LSHIFT */
static Object *Integer_lshift(Object *self, Object *arg0, Object *arg1) {
    return Integer_binaryOper((Integer *)self, arg0, OPER_LSHIFT);
}

/* OPER_RSHIFT */
static Object *Integer_rshift(Object *self, Object *arg0, Object *arg1) {
    return Integer_binaryOper((Integer *)self, arg0, OPER_RSHIFT);
}

/* OPER_LT */
static Object *Integer_lt(Object *self, Object *arg0, Object *arg1) {
    return Integer_binaryLogicalOper((Integer *)self, arg0, OPER_LT);
}

/* OPER_GT */
static Object *Integer_gt(Object *self, Object *arg0, Object *arg1) {
    return Integer_binaryLogicalOper((Integer *)self, arg0, OPER_GT);
}

/* OPER_LE */
static Object *Integer_le(Object *self, Object *arg0, Object *arg1) {
    return Integer_binaryLogicalOper((Integer *)self, arg0, OPER_LE);
}

/* OPER_GE */
static Object *Integer_ge(Object *self, Object *arg0, Object *arg1) {
    return Integer_binaryLogicalOper((Integer *)self, arg0, OPER_GE);
}

/* OPER_EQ */
static Object *Integer_eq(Object *self, Object *arg0, Object *arg1) {
    return Integer_binaryLogicalOper((Integer *)self, arg0, OPER_EQ);
}

/* OPER_NE */
static Object *Integer_ne(Object *self, Object *arg0, Object *arg1) {
    return Integer_binaryLogicalOper((Integer *)self, arg0, OPER_NE);
}

/* OPER_BAND */
static Object *Integer_band(Object *self, Object *arg0, Object *arg1) {
    return Integer_binaryOper((Integer *)self, arg0, OPER_BAND);
}

/* OPER_BXOR */
static Object *Integer_bxor(Object *self, Object *arg0, Object *arg1) {
    return Integer_binaryOper((Integer *)self, arg0, OPER_BXOR);
}

/* OPER_BOR */
static Object *Integer_bor(Object *self, Object *arg0, Object *arg1) {
    return Integer_binaryOper((Integer *)self, arg0, OPER_BOR);
}


/*------------------------------------------------------------------------*/
/* methods -- other */

static Object *Integer_print(Object *_self, Object *arg0, Object *arg1) {
    Integer *self;
    
    self = (Integer *)_self;
    printf("%ld", self->value);
    return Spk_void;
}


/*------------------------------------------------------------------------*/
/* class template */

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
    /* other */
    { "print", SpkNativeCode_METH_ATTR | SpkNativeCode_ARGS_0, &Integer_print },
    { 0, 0, 0}
};

SpkClassTmpl Spk_ClassIntegerTmpl = {
    "Integer",
    offsetof(IntegerSubclass, variables),
    sizeof(Integer),
    0,
    0,
    methods
};


/*------------------------------------------------------------------------*/
/* C API */

Integer *SpkInteger_fromLong(long value) {
    Integer *result;
    
    result = (Integer *)malloc(sizeof(Integer));
    result->base.klass = Spk_ClassInteger;
    result->value = value;
    return result;
}

long SpkInteger_asLong(Integer *anInteger) {
    return anInteger->value;
}
