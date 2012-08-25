
#include "rtl.h"

#include <math.h>
#include <stdlib.h>


typedef unsigned int Oper;

enum /*Oper*/ {
    OPER_SUCC,
    OPER_PRED,
    OPER_ADDR,
    OPER_IND,
    OPER_POS,
    OPER_NEG,
    OPER_BNEG,
    OPER_LNEG,
    OPER_MUL,
    OPER_DIV,
    OPER_MOD,
    OPER_ADD,
    OPER_SUB,
    OPER_LSHIFT,
    OPER_RSHIFT,
    OPER_LT,
    OPER_GT,
    OPER_LE,
    OPER_GE,
    OPER_EQ,
    OPER_NE,
    OPER_BAND,
    OPER_BXOR,
    OPER_BOR,

    NUM_OPER
};


typedef struct Object Object;


struct Float {
    Object base;
    double value;
};


#define BOOL(cond) ((cond) ? &__spk_x_true : &__spk_x_false)


/*------------------------------------------------------------------------*/
/* method helpers */

static struct Float *newFloat(void) {
    struct Float *result = (struct Float *)malloc(sizeof(struct Float));
    result->base.klass = &__spk_x_Float;
    return result;
}

static Object *Float_unaryOper(struct Float *self, Oper oper) {
    struct Float *result;
    
    result = newFloat();
    switch (oper) {
    case OPER_SUCC: result->value = self->value + 1; break;
    case OPER_PRED: result->value = self->value - 1; break;
    case OPER_POS:  result->value = +self->value;    break;
    case OPER_NEG:  result->value = -self->value;    break;
    default: return 0; /* not reached */
    }
    return (Object *)result;
}

static Object *Float_binaryOper(struct Float *self, Object *arg0, Oper oper) {
    struct Float *arg, *result;
    
    arg = CAST(Float, arg0);
    if (!arg) {
        SpikeError(&__spk_sym_typeError);
        return 0;
    }
    result = newFloat();
    switch (oper) {
    case OPER_MUL: result->value = self->value * arg->value;  break;
    case OPER_DIV: result->value = self->value / arg->value;  break;
    case OPER_MOD: result->value = fmod(self->value, arg->value);  break;
    case OPER_ADD: result->value = self->value + arg->value;  break;
    case OPER_SUB: result->value = self->value - arg->value;  break;
    default: return 0; /* not reached */
    }
    return (Object *)result;
}

static Object *Float_binaryLogicalOper(struct Float *self, Object *arg0, Oper oper) {
    struct Float *arg;
    Object *result;
    
    arg = CAST(Float, arg0);
    if (!arg) switch (oper) {
    case OPER_EQ:
        /* XXX: 0.0 == 0 */
        return &__spk_x_false;
    case OPER_NE:
        return &__spk_x_true;
    default:
        SpikeError(&__spk_sym_typeError); /* XXX */
        return 0;
    }
    
    switch (oper) {
    case OPER_LT: result = BOOL(self->value < arg->value);  break;
    case OPER_GT: result = BOOL(self->value > arg->value);  break;
    case OPER_LE: result = BOOL(self->value <= arg->value); break;
    case OPER_GE: result = BOOL(self->value >= arg->value); break;
    case OPER_EQ: result = BOOL(self->value == arg->value); break;
    case OPER_NE: result = BOOL(self->value != arg->value); break;
    default: return 0; /* not reached */
    }
    return result;
}


/*------------------------------------------------------------------------*/
/* methods -- operators */

/* OPER_SUCC */
Object *Float_succ(struct Float *self) {
    return Float_unaryOper(self, OPER_SUCC);
}

/* OPER_PRED */
Object *Float_pred(struct Float *self) {
    return Float_unaryOper(self, OPER_PRED);
}

/* OPER_POS */
Object *Float_pos(struct Float *self) {
    return Float_unaryOper(self, OPER_POS);
}

/* OPER_NEG */
Object *Float_neg(struct Float *self) {
    return Float_unaryOper(self, OPER_NEG);
}

/* OPER_MUL */
Object *Float_mul(struct Float *self, Object *arg0) {
    return Float_binaryOper(self, arg0, OPER_MUL);
}

/* OPER_DIV */
Object *Float_div(struct Float *self, Object *arg0) {
    return Float_binaryOper(self, arg0, OPER_DIV);
}

/* OPER_DIV */
Object *Float_mod(struct Float *self, Object *arg0) {
    return Float_binaryOper(self, arg0, OPER_MOD);
}

/* OPER_ADD */
Object *Float_add(struct Float *self, Object *arg0) {
    return Float_binaryOper(self, arg0, OPER_ADD);
}

/* OPER_SUB */
Object *Float_sub(struct Float *self, Object *arg0) {
    return Float_binaryOper(self, arg0, OPER_SUB);
}

/* OPER_LT */
Object *Float_lt(struct Float *self, Object *arg0) {
    return Float_binaryLogicalOper(self, arg0, OPER_LT);
}

/* OPER_GT */
Object *Float_gt(struct Float *self, Object *arg0) {
    return Float_binaryLogicalOper(self, arg0, OPER_GT);
}

/* OPER_LE */
Object *Float_le(struct Float *self, Object *arg0) {
    return Float_binaryLogicalOper(self, arg0, OPER_LE);
}

/* OPER_GE */
Object *Float_ge(struct Float *self, Object *arg0) {
    return Float_binaryLogicalOper(self, arg0, OPER_GE);
}

/* OPER_EQ */
Object *Float_eq(struct Float *self, Object *arg0) {
    return Float_binaryLogicalOper(self, arg0, OPER_EQ);
}

/* OPER_NE */
Object *Float_ne(struct Float *self, Object *arg0) {
    return Float_binaryLogicalOper(self, arg0, OPER_NE);
}
