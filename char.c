
#include "char.h"

#include "bool.h"
#include "class.h"
#include "heart.h"
#include "interp.h"
#include "module.h"
#include "native.h"

#include <stddef.h>
#include <stdlib.h>


struct Char {
    Object base;
    char value;
};


#define BOOL(cond) ((cond) ? GLOBAL(xtrue) : GLOBAL(xfalse))


/* XXX: There are only 256 of these objects... */


/*------------------------------------------------------------------------*/
/* method helpers */

static Unknown *Char_unaryOper(Char *self, Oper oper) {
    Char *result;
    
    result = (Char *)Object_New(CLASS(Char));
    switch (oper) {
    case OPER_SUCC: result->value = self->value + 1; break;
    case OPER_PRED: result->value = self->value - 1; break;
    case OPER_POS:  result->value = +self->value;    break;
    case OPER_NEG:  result->value = -self->value;    break;
    case OPER_BNEG: result->value = ~self->value;    break;
    default:
        Halt(HALT_VALUE_ERROR, "bad operator");
        return 0;
    }
    return (Unknown *)result;
}

static Unknown *Char_binaryOper(Char *self, Unknown *arg0, Oper oper) {
    Char *arg, *result;
    
    arg = CAST(Char, arg0);
    if (!arg) {
        Halt(HALT_TYPE_ERROR, "a Char object is required");
        return 0;
    }
    result = (Char *)Object_New(CLASS(Char));
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
        Halt(HALT_VALUE_ERROR, "bad operator");
        return 0;
    }
    return (Unknown *)result;
}

static Unknown *Char_binaryLogicalOper(Char *self, Unknown *arg0, Oper oper) {
    Char *arg;
    Unknown *result;
    
    arg = CAST(Char, arg0);
    if (!arg) switch (oper) {
    case OPER_EQ:
        return GLOBAL(xfalse);
    case OPER_NE:
        return GLOBAL(xtrue);
    default:
        Halt(HALT_TYPE_ERROR, "a Char object is required");
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
        Halt(HALT_VALUE_ERROR, "bad operator");
        return 0;
    }
    return result;
}


/*------------------------------------------------------------------------*/
/* methods -- operators */

/* OPER_SUCC */
static Unknown *Char_succ(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return Char_unaryOper((Char *)self, OPER_SUCC);
}

/* OPER_PRED */
static Unknown *Char_pred(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return Char_unaryOper((Char *)self, OPER_PRED);
}

/* OPER_POS */
static Unknown *Char_pos(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return Char_unaryOper((Char *)self, OPER_POS);
}

/* OPER_NEG */
static Unknown *Char_neg(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return Char_unaryOper((Char *)self, OPER_NEG);
}

/* OPER_BNEG */
static Unknown *Char_bneg(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return Char_unaryOper((Char *)self, OPER_BNEG);
}

/* OPER_LNEG */
static Unknown *Char_lneg(Unknown *self, Unknown *arg0, Unknown *arg1) {
    Unknown *result;
    result = BOOL(!((Char *)self)->value);
    return result;
}

/* OPER_MUL */
static Unknown *Char_mul(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return Char_binaryOper((Char *)self, arg0, OPER_MUL);
}

/* OPER_DIV */
static Unknown *Char_div(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return Char_binaryOper((Char *)self, arg0, OPER_DIV);
}

/* OPER_MOD */
static Unknown *Char_mod(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return Char_binaryOper((Char *)self, arg0, OPER_MOD);
}

/* OPER_ADD */
static Unknown *Char_add(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return Char_binaryOper((Char *)self, arg0, OPER_ADD);
}

/* OPER_SUB */
static Unknown *Char_sub(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return Char_binaryOper((Char *)self, arg0, OPER_SUB);
}

/* OPER_LSHIFT */
static Unknown *Char_lshift(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return Char_binaryOper((Char *)self, arg0, OPER_LSHIFT);
}

/* OPER_RSHIFT */
static Unknown *Char_rshift(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return Char_binaryOper((Char *)self, arg0, OPER_RSHIFT);
}

/* OPER_LT */
static Unknown *Char_lt(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return Char_binaryLogicalOper((Char *)self, arg0, OPER_LT);
}

/* OPER_GT */
static Unknown *Char_gt(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return Char_binaryLogicalOper((Char *)self, arg0, OPER_GT);
}

/* OPER_LE */
static Unknown *Char_le(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return Char_binaryLogicalOper((Char *)self, arg0, OPER_LE);
}

/* OPER_GE */
static Unknown *Char_ge(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return Char_binaryLogicalOper((Char *)self, arg0, OPER_GE);
}

/* OPER_EQ */
static Unknown *Char_eq(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return Char_binaryLogicalOper((Char *)self, arg0, OPER_EQ);
}

/* OPER_NE */
static Unknown *Char_ne(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return Char_binaryLogicalOper((Char *)self, arg0, OPER_NE);
}

/* OPER_BAND */
static Unknown *Char_band(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return Char_binaryOper((Char *)self, arg0, OPER_BAND);
}

/* OPER_BXOR */
static Unknown *Char_bxor(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return Char_binaryOper((Char *)self, arg0, OPER_BXOR);
}

/* OPER_BOR */
static Unknown *Char_bor(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return Char_binaryOper((Char *)self, arg0, OPER_BOR);
}


/*------------------------------------------------------------------------*/
/* class tmpl */

typedef struct CharSubclass {
    Char base;
    Unknown *variables[1]; /* stretchy */
} CharSubclass;

static MethodTmpl methods[] = {
    /* operators */
    { "__succ__",   NativeCode_UNARY_OPER  | NativeCode_LEAF, &Char_succ   },
    { "__pred__",   NativeCode_UNARY_OPER  | NativeCode_LEAF, &Char_pred   },
    { "__pos__",    NativeCode_UNARY_OPER  | NativeCode_LEAF, &Char_pos    },
    { "__neg__",    NativeCode_UNARY_OPER  | NativeCode_LEAF, &Char_neg    },
    { "__bneg__",   NativeCode_UNARY_OPER  | NativeCode_LEAF, &Char_bneg   },
    { "__lneg__",   NativeCode_UNARY_OPER  | NativeCode_LEAF, &Char_lneg   },
    { "__mul__",    NativeCode_BINARY_OPER | NativeCode_LEAF, &Char_mul    },
    { "__div__",    NativeCode_BINARY_OPER | NativeCode_LEAF, &Char_div    },
    { "__mod__",    NativeCode_BINARY_OPER | NativeCode_LEAF, &Char_mod    },
    { "__add__",    NativeCode_BINARY_OPER | NativeCode_LEAF, &Char_add    },
    { "__sub__",    NativeCode_BINARY_OPER | NativeCode_LEAF, &Char_sub    },
    { "__lshift__", NativeCode_BINARY_OPER | NativeCode_LEAF, &Char_lshift },
    { "__rshift__", NativeCode_BINARY_OPER | NativeCode_LEAF, &Char_rshift },
    { "__lt__",     NativeCode_BINARY_OPER | NativeCode_LEAF, &Char_lt     },
    { "__gt__",     NativeCode_BINARY_OPER | NativeCode_LEAF, &Char_gt     },
    { "__le__",     NativeCode_BINARY_OPER | NativeCode_LEAF, &Char_le     },
    { "__ge__",     NativeCode_BINARY_OPER | NativeCode_LEAF, &Char_ge     },
    { "__eq__",     NativeCode_BINARY_OPER | NativeCode_LEAF, &Char_eq     },
    { "__ne__",     NativeCode_BINARY_OPER | NativeCode_LEAF, &Char_ne     },
    { "__band__",   NativeCode_BINARY_OPER | NativeCode_LEAF, &Char_band   },
    { "__bxor__",   NativeCode_BINARY_OPER | NativeCode_LEAF, &Char_bxor   },
    { "__bor__",    NativeCode_BINARY_OPER | NativeCode_LEAF, &Char_bor    },
    { 0 }
};

ClassTmpl ClassCharTmpl = {
    HEART_CLASS_TMPL(Char, Object), {
        /*accessors*/ 0,
        methods,
        /*lvalueMethods*/ 0,
        offsetof(CharSubclass, variables)
    }, /*meta*/ {
        0
    }
};


/*------------------------------------------------------------------------*/
/* C API */

Char *Char_FromCChar(char c) {
    Char *result;
    
    result = (Char *)Object_New(CLASS(Char));
    result->value = c;
    return result;
}

char Char_AsCChar(Char *aChar) {
    return aChar->value;
}
