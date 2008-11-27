
#include "char.h"

#include "behavior.h"
#include "bool.h"
#include "interp.h"
#include "module.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>


#define BOOL(cond) ((cond) ? Spk_true : Spk_false)


Behavior *ClassChar;


/*------------------------------------------------------------------------*/
/* method helpers */

static Object *Char_unaryOper(Char *self, Oper oper) {
    Char *result;
    
    result = (Char *)malloc(sizeof(Char));
    result->base.klass = ClassChar;
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

static Object *Char_binaryOper(Char *self, Object *arg0, Oper oper) {
    Char *arg, *result;
    
    assert(arg0->klass == ClassChar); /* XXX */
    arg = (Char *)arg0;
    result = (Char *)malloc(sizeof(Char));
    result->base.klass = ClassChar;
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

static Object *Char_binaryLogicalOper(Char *self, Object *arg0, Oper oper) {
    Char *arg;
    Boolean *result;
    
    assert(arg0->klass == ClassChar); /* XXX */
    arg = (Char *)arg0;
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
static Object *Char_succ(Object *self, Object *arg0, Object *arg1) {
    return Char_unaryOper((Char *)self, OPER_SUCC);
}

/* OPER_PRED */
static Object *Char_pred(Object *self, Object *arg0, Object *arg1) {
    return Char_unaryOper((Char *)self, OPER_PRED);
}

/* OPER_POS */
static Object *Char_pos(Object *self, Object *arg0, Object *arg1) {
    return Char_unaryOper((Char *)self, OPER_POS);
}

/* OPER_NEG */
static Object *Char_neg(Object *self, Object *arg0, Object *arg1) {
    return Char_unaryOper((Char *)self, OPER_NEG);
}

/* OPER_BNEG */
static Object *Char_bneg(Object *self, Object *arg0, Object *arg1) {
    return Char_unaryOper((Char *)self, OPER_BNEG);
}

/* OPER_LNEG */
static Object *Char_lneg(Object *self, Object *arg0, Object *arg1) {
    return BOOL(!((Char *)self)->value);
}

/* OPER_MUL */
static Object *Char_mul(Object *self, Object *arg0, Object *arg1) {
    return Char_binaryOper((Char *)self, arg0, OPER_MUL);
}

/* OPER_DIV */
static Object *Char_div(Object *self, Object *arg0, Object *arg1) {
    return Char_binaryOper((Char *)self, arg0, OPER_DIV);
}

/* OPER_MOD */
static Object *Char_mod(Object *self, Object *arg0, Object *arg1) {
    return Char_binaryOper((Char *)self, arg0, OPER_MOD);
}

/* OPER_ADD */
static Object *Char_add(Object *self, Object *arg0, Object *arg1) {
    return Char_binaryOper((Char *)self, arg0, OPER_ADD);
}

/* OPER_SUB */
static Object *Char_sub(Object *self, Object *arg0, Object *arg1) {
    return Char_binaryOper((Char *)self, arg0, OPER_SUB);
}

/* OPER_LSHIFT */
static Object *Char_lshift(Object *self, Object *arg0, Object *arg1) {
    return Char_binaryOper((Char *)self, arg0, OPER_LSHIFT);
}

/* OPER_RSHIFT */
static Object *Char_rshift(Object *self, Object *arg0, Object *arg1) {
    return Char_binaryOper((Char *)self, arg0, OPER_RSHIFT);
}

/* OPER_LT */
static Object *Char_lt(Object *self, Object *arg0, Object *arg1) {
    return Char_binaryLogicalOper((Char *)self, arg0, OPER_LT);
}

/* OPER_GT */
static Object *Char_gt(Object *self, Object *arg0, Object *arg1) {
    return Char_binaryLogicalOper((Char *)self, arg0, OPER_GT);
}

/* OPER_LE */
static Object *Char_le(Object *self, Object *arg0, Object *arg1) {
    return Char_binaryLogicalOper((Char *)self, arg0, OPER_LE);
}

/* OPER_GE */
static Object *Char_ge(Object *self, Object *arg0, Object *arg1) {
    return Char_binaryLogicalOper((Char *)self, arg0, OPER_GE);
}

/* OPER_EQ */
static Object *Char_eq(Object *self, Object *arg0, Object *arg1) {
    return Char_binaryLogicalOper((Char *)self, arg0, OPER_EQ);
}

/* OPER_NE */
static Object *Char_ne(Object *self, Object *arg0, Object *arg1) {
    return Char_binaryLogicalOper((Char *)self, arg0, OPER_NE);
}

/* OPER_BAND */
static Object *Char_band(Object *self, Object *arg0, Object *arg1) {
    return Char_binaryOper((Char *)self, arg0, OPER_BAND);
}

/* OPER_BXOR */
static Object *Char_bxor(Object *self, Object *arg0, Object *arg1) {
    return Char_binaryOper((Char *)self, arg0, OPER_BXOR);
}

/* OPER_BOR */
static Object *Char_bor(Object *self, Object *arg0, Object *arg1) {
    return Char_binaryOper((Char *)self, arg0, OPER_BOR);
}


/*------------------------------------------------------------------------*/
/* methods -- other */

static Object *Char_print(Object *_self, Object *arg0, Object *arg1) {
    Char *self;
    
    self = (Char *)_self;
    fputc(self->value, stdout);
    return Spk_void;
}


/*------------------------------------------------------------------------*/
/* class template */

static SpkMethodTmpl methods[] = {
    /* operators */
    { "__succ__",   SpkNativeCode_ARGS_0 | SpkNativeCode_LEAF, &Char_succ   },
    { "__pred__",   SpkNativeCode_ARGS_0 | SpkNativeCode_LEAF, &Char_pred   },
    { "__pos__",    SpkNativeCode_ARGS_0 | SpkNativeCode_LEAF, &Char_pos    },
    { "__neg__",    SpkNativeCode_ARGS_0 | SpkNativeCode_LEAF, &Char_neg    },
    { "__bneg__",   SpkNativeCode_ARGS_0 | SpkNativeCode_LEAF, &Char_bneg   },
    { "__lneg__",   SpkNativeCode_ARGS_0 | SpkNativeCode_LEAF, &Char_lneg   },
    { "__mul__",    SpkNativeCode_ARGS_1 | SpkNativeCode_LEAF, &Char_mul    },
    { "__div__",    SpkNativeCode_ARGS_1 | SpkNativeCode_LEAF, &Char_div    },
    { "__mod__",    SpkNativeCode_ARGS_1 | SpkNativeCode_LEAF, &Char_mod    },
    { "__add__",    SpkNativeCode_ARGS_1 | SpkNativeCode_LEAF, &Char_add    },
    { "__sub__",    SpkNativeCode_ARGS_1 | SpkNativeCode_LEAF, &Char_sub    },
    { "__lshift__", SpkNativeCode_ARGS_1 | SpkNativeCode_LEAF, &Char_lshift },
    { "__rshift__", SpkNativeCode_ARGS_1 | SpkNativeCode_LEAF, &Char_rshift },
    { "__lt__",     SpkNativeCode_ARGS_1 | SpkNativeCode_LEAF, &Char_lt     },
    { "__gt__",     SpkNativeCode_ARGS_1 | SpkNativeCode_LEAF, &Char_gt     },
    { "__le__",     SpkNativeCode_ARGS_1 | SpkNativeCode_LEAF, &Char_le     },
    { "__ge__",     SpkNativeCode_ARGS_1 | SpkNativeCode_LEAF, &Char_ge     },
    { "__eq__",     SpkNativeCode_ARGS_1 | SpkNativeCode_LEAF, &Char_eq     },
    { "__ne__",     SpkNativeCode_ARGS_1 | SpkNativeCode_LEAF, &Char_ne     },
    { "__band__",   SpkNativeCode_ARGS_1 | SpkNativeCode_LEAF, &Char_band   },
    { "__bxor__",   SpkNativeCode_ARGS_1 | SpkNativeCode_LEAF, &Char_bxor   },
    { "__bor__",    SpkNativeCode_ARGS_1 | SpkNativeCode_LEAF, &Char_bor    },
    /* other */
    { "print", SpkNativeCode_ARGS_0 | SpkNativeCode_CALLABLE, &Char_print },
    { 0, 0, 0}
};

SpkClassTmpl ClassCharTmpl = {
    "Char",
    offsetof(CharSubclass, variables),
    sizeof(Char),
    0,
    methods
};


/*------------------------------------------------------------------------*/
/* C API */

Char *SpkChar_fromChar(char c) {
    Char *result;
    
    result = (Char *)malloc(sizeof(Char));
    result->base.klass = ClassChar;
    result->value = c;
    return result;
}

Char *SpkChar_fromLiteral(char *str, size_t len) {
    Char *result;
    char *d, *s;
    char c;
    
    /* strip quotes */
    s = str + 1; len -= 2;
    
    result = (Char *)malloc(sizeof(Char));
    result->base.klass = ClassChar;
    
    while (len--) {
        c = *s++;
        /* Note: a trailing '\' is syntactically impossible. */
        if (len && c == '\\') {
            c = *s++;
            --len;
            
            /* XXX: numeric escape codes */
            switch (c) {
            case 'a': c = '\a'; break;
            case 'b': c = '\b'; break;
            case 'f': c = '\f'; break;
            case 'n': c = '\n'; break;
            case 'r': c = '\r'; break;
            case 't': c = '\t'; break;
            case 'v': c = '\v'; break;
                
            case '\\':
            case '\'':
            case '"':
                break;
                
            default:
                /* XXX: error */
                break;
            }
            result->value = c;
        } else {
            result->value = c;
        }
    }
    
    return result;
}

char SpkChar_asChar(Char *aChar) {
    return aChar->value;
}
