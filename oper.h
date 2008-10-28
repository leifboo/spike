
#ifndef __oper_h__
#define __oper_h__


typedef enum Oper {
    OPER_INDEX,
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

    NUM_OPER,
    OPER_CALL
} Oper;


#endif /* __oper_h__ */
