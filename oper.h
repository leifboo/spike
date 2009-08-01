
#ifndef __spk_oper_h__
#define __spk_oper_h__


typedef enum SpkOper {
    Spk_OPER_SUCC,
    Spk_OPER_PRED,
    Spk_OPER_ADDR,
    Spk_OPER_IND,
    Spk_OPER_POS,
    Spk_OPER_NEG,
    Spk_OPER_BNEG,
    Spk_OPER_LNEG,
    Spk_OPER_MUL,
    Spk_OPER_DIV,
    Spk_OPER_MOD,
    Spk_OPER_ADD,
    Spk_OPER_SUB,
    Spk_OPER_LSHIFT,
    Spk_OPER_RSHIFT,
    Spk_OPER_LT,
    Spk_OPER_GT,
    Spk_OPER_LE,
    Spk_OPER_GE,
    Spk_OPER_EQ,
    Spk_OPER_NE,
    Spk_OPER_BAND,
    Spk_OPER_BXOR,
    Spk_OPER_BOR,

    Spk_NUM_OPER
} SpkOper;


typedef enum SpkCallOper {
    Spk_OPER_APPLY,
    Spk_OPER_INDEX,
    
    Spk_NUM_CALL_OPER
} SpkCallOper;


#endif /* __spk_oper_h__ */
