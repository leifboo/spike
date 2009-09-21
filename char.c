
#include "char.h"

#include "bool.h"
#include "class.h"
#include "heart.h"
#include "interp.h"
#include "module.h"
#include "native.h"

#include <stddef.h>
#include <stdlib.h>


struct SpkChar {
    SpkObject base;
    char value;
};


#define BOOL(cond) ((cond) ? Spk_true : Spk_false)


/* XXX: There are only 256 of these objects... */


/*------------------------------------------------------------------------*/
/* method helpers */

static SpkUnknown *Char_unaryOper(SpkChar *self, SpkOper oper) {
    SpkChar *result;
    
    result = (SpkChar *)SpkObject_New(Spk_CLASS(Char));
    switch (oper) {
    case Spk_OPER_SUCC: result->value = self->value + 1; break;
    case Spk_OPER_PRED: result->value = self->value - 1; break;
    case Spk_OPER_POS:  result->value = +self->value;    break;
    case Spk_OPER_NEG:  result->value = -self->value;    break;
    case Spk_OPER_BNEG: result->value = ~self->value;    break;
    default:
        Spk_Halt(Spk_HALT_VALUE_ERROR, "bad operator");
        Spk_DECREF(result);
        return 0;
    }
    return (SpkUnknown *)result;
}

static SpkUnknown *Char_binaryOper(SpkChar *self, SpkUnknown *arg0, SpkOper oper) {
    SpkChar *arg, *result;
    
    arg = Spk_CAST(Char, arg0);
    if (!arg) {
        Spk_Halt(Spk_HALT_TYPE_ERROR, "a Char object is required");
        return 0;
    }
    result = (SpkChar *)SpkObject_New(Spk_CLASS(Char));
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
        Spk_Halt(Spk_HALT_VALUE_ERROR, "bad operator");
        Spk_DECREF(result);
        return 0;
    }
    return (SpkUnknown *)result;
}

static SpkUnknown *Char_binaryLogicalOper(SpkChar *self, SpkUnknown *arg0, SpkOper oper) {
    SpkChar *arg;
    SpkUnknown *result;
    
    arg = Spk_CAST(Char, arg0);
    if (!arg) switch (oper) {
    case Spk_OPER_EQ:
        Spk_INCREF(Spk_false);
        return Spk_false;
    case Spk_OPER_NE:
        Spk_INCREF(Spk_true);
        return Spk_true;
    default:
        Spk_Halt(Spk_HALT_TYPE_ERROR, "a Char object is required");
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
        Spk_Halt(Spk_HALT_VALUE_ERROR, "bad operator");
        return 0;
    }
    Spk_INCREF(result);
    return result;
}


/*------------------------------------------------------------------------*/
/* methods -- operators */

/* Spk_OPER_SUCC */
static SpkUnknown *Char_succ(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return Char_unaryOper((SpkChar *)self, Spk_OPER_SUCC);
}

/* Spk_OPER_PRED */
static SpkUnknown *Char_pred(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return Char_unaryOper((SpkChar *)self, Spk_OPER_PRED);
}

/* Spk_OPER_POS */
static SpkUnknown *Char_pos(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return Char_unaryOper((SpkChar *)self, Spk_OPER_POS);
}

/* Spk_OPER_NEG */
static SpkUnknown *Char_neg(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return Char_unaryOper((SpkChar *)self, Spk_OPER_NEG);
}

/* Spk_OPER_BNEG */
static SpkUnknown *Char_bneg(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return Char_unaryOper((SpkChar *)self, Spk_OPER_BNEG);
}

/* Spk_OPER_LNEG */
static SpkUnknown *Char_lneg(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    SpkUnknown *result;
    result = BOOL(!((SpkChar *)self)->value);
    Spk_INCREF(result);
    return result;
}

/* Spk_OPER_MUL */
static SpkUnknown *Char_mul(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return Char_binaryOper((SpkChar *)self, arg0, Spk_OPER_MUL);
}

/* Spk_OPER_DIV */
static SpkUnknown *Char_div(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return Char_binaryOper((SpkChar *)self, arg0, Spk_OPER_DIV);
}

/* Spk_OPER_MOD */
static SpkUnknown *Char_mod(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return Char_binaryOper((SpkChar *)self, arg0, Spk_OPER_MOD);
}

/* Spk_OPER_ADD */
static SpkUnknown *Char_add(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return Char_binaryOper((SpkChar *)self, arg0, Spk_OPER_ADD);
}

/* Spk_OPER_SUB */
static SpkUnknown *Char_sub(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return Char_binaryOper((SpkChar *)self, arg0, Spk_OPER_SUB);
}

/* Spk_OPER_LSHIFT */
static SpkUnknown *Char_lshift(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return Char_binaryOper((SpkChar *)self, arg0, Spk_OPER_LSHIFT);
}

/* Spk_OPER_RSHIFT */
static SpkUnknown *Char_rshift(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return Char_binaryOper((SpkChar *)self, arg0, Spk_OPER_RSHIFT);
}

/* Spk_OPER_LT */
static SpkUnknown *Char_lt(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return Char_binaryLogicalOper((SpkChar *)self, arg0, Spk_OPER_LT);
}

/* Spk_OPER_GT */
static SpkUnknown *Char_gt(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return Char_binaryLogicalOper((SpkChar *)self, arg0, Spk_OPER_GT);
}

/* Spk_OPER_LE */
static SpkUnknown *Char_le(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return Char_binaryLogicalOper((SpkChar *)self, arg0, Spk_OPER_LE);
}

/* Spk_OPER_GE */
static SpkUnknown *Char_ge(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return Char_binaryLogicalOper((SpkChar *)self, arg0, Spk_OPER_GE);
}

/* Spk_OPER_EQ */
static SpkUnknown *Char_eq(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return Char_binaryLogicalOper((SpkChar *)self, arg0, Spk_OPER_EQ);
}

/* Spk_OPER_NE */
static SpkUnknown *Char_ne(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return Char_binaryLogicalOper((SpkChar *)self, arg0, Spk_OPER_NE);
}

/* Spk_OPER_BAND */
static SpkUnknown *Char_band(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return Char_binaryOper((SpkChar *)self, arg0, Spk_OPER_BAND);
}

/* Spk_OPER_BXOR */
static SpkUnknown *Char_bxor(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return Char_binaryOper((SpkChar *)self, arg0, Spk_OPER_BXOR);
}

/* Spk_OPER_BOR */
static SpkUnknown *Char_bor(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return Char_binaryOper((SpkChar *)self, arg0, Spk_OPER_BOR);
}


/*------------------------------------------------------------------------*/
/* class template */

typedef struct SpkCharSubclass {
    SpkChar base;
    SpkUnknown *variables[1]; /* stretchy */
} SpkCharSubclass;

static SpkMethodTmpl methods[] = {
    /* operators */
    { "__succ__",   SpkNativeCode_UNARY_OPER  | SpkNativeCode_LEAF, &Char_succ   },
    { "__pred__",   SpkNativeCode_UNARY_OPER  | SpkNativeCode_LEAF, &Char_pred   },
    { "__pos__",    SpkNativeCode_UNARY_OPER  | SpkNativeCode_LEAF, &Char_pos    },
    { "__neg__",    SpkNativeCode_UNARY_OPER  | SpkNativeCode_LEAF, &Char_neg    },
    { "__bneg__",   SpkNativeCode_UNARY_OPER  | SpkNativeCode_LEAF, &Char_bneg   },
    { "__lneg__",   SpkNativeCode_UNARY_OPER  | SpkNativeCode_LEAF, &Char_lneg   },
    { "__mul__",    SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &Char_mul    },
    { "__div__",    SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &Char_div    },
    { "__mod__",    SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &Char_mod    },
    { "__add__",    SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &Char_add    },
    { "__sub__",    SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &Char_sub    },
    { "__lshift__", SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &Char_lshift },
    { "__rshift__", SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &Char_rshift },
    { "__lt__",     SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &Char_lt     },
    { "__gt__",     SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &Char_gt     },
    { "__le__",     SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &Char_le     },
    { "__ge__",     SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &Char_ge     },
    { "__eq__",     SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &Char_eq     },
    { "__ne__",     SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &Char_ne     },
    { "__band__",   SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &Char_band   },
    { "__bxor__",   SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &Char_bxor   },
    { "__bor__",    SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &Char_bor    },
    { 0, 0, 0}
};

SpkClassTmpl Spk_ClassCharTmpl = {
    Spk_HEART_CLASS_TMPL(Char, Object), {
        /*accessors*/ 0,
        methods,
        /*lvalueMethods*/ 0,
        offsetof(SpkCharSubclass, variables)
    }, /*meta*/ {
    }
};


/*------------------------------------------------------------------------*/
/* C API */

SpkChar *SpkChar_FromCChar(char c) {
    SpkChar *result;
    
    result = (SpkChar *)SpkObject_New(Spk_CLASS(Char));
    result->value = c;
    return result;
}

char SpkChar_AsCChar(SpkChar *aChar) {
    return aChar->value;
}
