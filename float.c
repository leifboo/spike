
#include "float.h"

#include "behavior.h"
#include "bool.h"
#include "interp.h"
#include "module.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>


#define BOOL(cond) ((cond) ? Spk_true : Spk_false)


Behavior *ClassFloat;


/*------------------------------------------------------------------------*/
/* method helpers */

static Object *Float_unaryOper(Float *self, Oper oper) {
    Float *result;
    
    result = (Float *)malloc(sizeof(Float));
    result->base.klass = ClassFloat;
    switch (oper) {
    case OPER_SUCC: result->value = self->value + 1; break;
    case OPER_PRED: result->value = self->value - 1; break;
    case OPER_POS:  result->value = +self->value;    break;
    case OPER_NEG:  result->value = -self->value;    break;
    default: assert(0);
    }
    return (Object *)result;
}

static Object *Float_binaryOper(Float *self, Object *arg0, Oper oper) {
    Float *arg, *result;
    
    assert(arg0->klass == ClassFloat); /* XXX */
    arg = (Float *)arg0;
    result = (Float *)malloc(sizeof(Float));
    result->base.klass = ClassFloat;
    switch (oper) {
    case OPER_MUL:    result->value = self->value * arg->value;  break;
    case OPER_DIV:    result->value = self->value / arg->value;  break;
    case OPER_ADD:    result->value = self->value + arg->value;  break;
    case OPER_SUB:    result->value = self->value - arg->value;  break;
    default: assert(0);
    }
    return (Object *)result;
}

static Object *Float_binaryLogicalOper(Float *self, Object *arg0, Oper oper) {
    Float *arg;
    Boolean *result;
    
    assert(arg0->klass == ClassFloat); /* XXX */
    arg = (Float *)arg0;
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
static Object *Float_succ(Object *self, Object *arg0, Object *arg1) {
    return Float_unaryOper((Float *)self, OPER_SUCC);
}

/* OPER_PRED */
static Object *Float_pred(Object *self, Object *arg0, Object *arg1) {
    return Float_unaryOper((Float *)self, OPER_PRED);
}

/* OPER_POS */
static Object *Float_pos(Object *self, Object *arg0, Object *arg1) {
    return Float_unaryOper((Float *)self, OPER_POS);
}

/* OPER_NEG */
static Object *Float_neg(Object *self, Object *arg0, Object *arg1) {
    return Float_unaryOper((Float *)self, OPER_NEG);
}

/* OPER_LNEG */
static Object *Float_lneg(Object *self, Object *arg0, Object *arg1) {
    return BOOL(!((Float *)self)->value);
}

/* OPER_MUL */
static Object *Float_mul(Object *self, Object *arg0, Object *arg1) {
    return Float_binaryOper((Float *)self, arg0, OPER_MUL);
}

/* OPER_DIV */
static Object *Float_div(Object *self, Object *arg0, Object *arg1) {
    return Float_binaryOper((Float *)self, arg0, OPER_DIV);
}

/* OPER_ADD */
static Object *Float_add(Object *self, Object *arg0, Object *arg1) {
    return Float_binaryOper((Float *)self, arg0, OPER_ADD);
}

/* OPER_SUB */
static Object *Float_sub(Object *self, Object *arg0, Object *arg1) {
    return Float_binaryOper((Float *)self, arg0, OPER_SUB);
}

/* OPER_LT */
static Object *Float_lt(Object *self, Object *arg0, Object *arg1) {
    return Float_binaryLogicalOper((Float *)self, arg0, OPER_LT);
}

/* OPER_GT */
static Object *Float_gt(Object *self, Object *arg0, Object *arg1) {
    return Float_binaryLogicalOper((Float *)self, arg0, OPER_GT);
}

/* OPER_LE */
static Object *Float_le(Object *self, Object *arg0, Object *arg1) {
    return Float_binaryLogicalOper((Float *)self, arg0, OPER_LE);
}

/* OPER_GE */
static Object *Float_ge(Object *self, Object *arg0, Object *arg1) {
    return Float_binaryLogicalOper((Float *)self, arg0, OPER_GE);
}

/* OPER_EQ */
static Object *Float_eq(Object *self, Object *arg0, Object *arg1) {
    return Float_binaryLogicalOper((Float *)self, arg0, OPER_EQ);
}

/* OPER_NE */
static Object *Float_ne(Object *self, Object *arg0, Object *arg1) {
    return Float_binaryLogicalOper((Float *)self, arg0, OPER_NE);
}


/*------------------------------------------------------------------------*/
/* methods -- other */

static Object *Float_print(Object *_self, Object *arg0, Object *arg1) {
    Float *self;
    
    self = (Float *)_self;
    printf("%e", self->value);
    return Spk_void;
}


/*------------------------------------------------------------------------*/
/* class template */

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
    /* other */
    { "print", SpkNativeCode_METH_ATTR | SpkNativeCode_ARGS_0, &Float_print },
    { 0, 0, 0}
};

SpkClassTmpl ClassFloatTmpl = {
    "Float",
    offsetof(FloatSubclass, variables),
    sizeof(Float),
    0,
    0,
    methods
};


/*------------------------------------------------------------------------*/
/* C API */

Float *SpkFloat_fromLiteral(char *str, size_t len) {
    Float *result;
    
    result = (Float *)malloc(sizeof(Float));
    result->base.klass = ClassFloat;
    result->value = strtod(str, 0);
    return result;
}
