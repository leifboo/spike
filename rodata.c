
#include "rodata.h"

#include "host.h"
#include "native.h"
#include "obj.h"
#include <stddef.h>
#include <string.h>


typedef struct { SpkUnknown **p; const char *s; }           SymbolTableEntry;
typedef SymbolTableEntry                                    SelectorTableEntry;
typedef struct { SpkUnknown **p; long i; }                  IntegerTableEntry;
typedef struct { SpkUnknown **p; const char *s; size_t n; } StringTableEntry;


/* symbols */
SpkUnknown *Spk_self;


/* selectors */
SpkUnknown *Spk___add__;
SpkUnknown *Spk___addr__;
SpkUnknown *Spk___apply__;
SpkUnknown *Spk___band__;
SpkUnknown *Spk___bneg__;
SpkUnknown *Spk___bor__;
SpkUnknown *Spk___bxor__;
SpkUnknown *Spk___div__;
SpkUnknown *Spk___eq__;
SpkUnknown *Spk___ge__;
SpkUnknown *Spk___gt__;
SpkUnknown *Spk___ind__;
SpkUnknown *Spk___index__;
SpkUnknown *Spk___le__;
SpkUnknown *Spk___lneg__;
SpkUnknown *Spk___lshift__;
SpkUnknown *Spk___lt__;
SpkUnknown *Spk___mod__;
SpkUnknown *Spk___mul__;
SpkUnknown *Spk___ne__;
SpkUnknown *Spk___neg__;
SpkUnknown *Spk___pos__;
SpkUnknown *Spk___pred__;
SpkUnknown *Spk___rshift__;
SpkUnknown *Spk___str__;
SpkUnknown *Spk___sub__;
SpkUnknown *Spk___succ__;
SpkUnknown *Spk__import;
SpkUnknown *Spk__init;
SpkUnknown *Spk__predef;
SpkUnknown *Spk__thunk;
SpkUnknown *Spk_badExpr;
SpkUnknown *Spk_blockCopy;
SpkUnknown *Spk_cannotReenterBlock;
SpkUnknown *Spk_cannotReturn;
SpkUnknown *Spk_compoundExpression;
SpkUnknown *Spk_doesNotUnderstand;
SpkUnknown *Spk_failOnError;
SpkUnknown *Spk_importModule;
SpkUnknown *Spk_importPackage;
SpkUnknown *Spk_init;
SpkUnknown *Spk_main;
SpkUnknown *Spk_mustBeBoolean;
SpkUnknown *Spk_mustBeSymbol;
SpkUnknown *Spk_mustBeTuple;
SpkUnknown *Spk_new;
SpkUnknown *Spk_noRunnableFiber;
SpkUnknown *Spk_numArgs;
SpkUnknown *Spk_printString;
SpkUnknown *Spk_recursiveDoesNotUnderstand;
SpkUnknown *Spk_redefinedSymbol;
SpkUnknown *Spk_source;
SpkUnknown *Spk_undefinedSymbol;
SpkUnknown *Spk_unknownOpcode;
SpkUnknown *Spk_wrongNumberOfArguments;

/* selectors -- class names */
SpkUnknown *Spk_Boolean;
SpkUnknown *Spk_False;
SpkUnknown *Spk_True;

/* selectors -- parser */
SpkUnknown *Spk_concat;
SpkUnknown *Spk_declSpecs;
SpkUnknown *Spk_exprAnd;
SpkUnknown *Spk_exprAssign;
SpkUnknown *Spk_exprAttr;
SpkUnknown *Spk_exprAttrVar;
SpkUnknown *Spk_exprBinaryOp;
SpkUnknown *Spk_exprBlock;
SpkUnknown *Spk_exprCall;
SpkUnknown *Spk_exprCompound;
SpkUnknown *Spk_exprCond;
SpkUnknown *Spk_exprId;
SpkUnknown *Spk_exprKeyword;
SpkUnknown *Spk_exprLiteral;
SpkUnknown *Spk_exprName;
SpkUnknown *Spk_exprNI;
SpkUnknown *Spk_exprOr;
SpkUnknown *Spk_exprPostOp;
SpkUnknown *Spk_exprPreOp;
SpkUnknown *Spk_exprUnaryOp;
SpkUnknown *Spk_isSpec;
SpkUnknown *Spk_left;
SpkUnknown *Spk_next;
SpkUnknown *Spk_nextArg;
SpkUnknown *Spk_operAdd;
SpkUnknown *Spk_operAddr;
SpkUnknown *Spk_operApply;
SpkUnknown *Spk_operBAnd;
SpkUnknown *Spk_operBNeg;
SpkUnknown *Spk_operBOr;
SpkUnknown *Spk_operBXOr;
SpkUnknown *Spk_operDiv;
SpkUnknown *Spk_operEq;
SpkUnknown *Spk_operGE;
SpkUnknown *Spk_operGT;
SpkUnknown *Spk_operInd;
SpkUnknown *Spk_operIndex;
SpkUnknown *Spk_operLE;
SpkUnknown *Spk_operLNeg;
SpkUnknown *Spk_operLShift;
SpkUnknown *Spk_operLT;
SpkUnknown *Spk_operMod;
SpkUnknown *Spk_operMul;
SpkUnknown *Spk_operNE;
SpkUnknown *Spk_operNeg;
SpkUnknown *Spk_operPos;
SpkUnknown *Spk_operPred;
SpkUnknown *Spk_operRShift;
SpkUnknown *Spk_operSub;
SpkUnknown *Spk_operSucc;
SpkUnknown *Spk_stmtBreak;
SpkUnknown *Spk_stmtCompound;
SpkUnknown *Spk_stmtContinue;
SpkUnknown *Spk_stmtDefClass;
SpkUnknown *Spk_stmtDefMethod;
SpkUnknown *Spk_stmtDefModule;
SpkUnknown *Spk_stmtDefSpec;
SpkUnknown *Spk_stmtDefVar;
SpkUnknown *Spk_stmtDoWhile;
SpkUnknown *Spk_stmtExpr;
SpkUnknown *Spk_stmtFor;
SpkUnknown *Spk_stmtIfElse;
SpkUnknown *Spk_stmtPragmaSource;
SpkUnknown *Spk_stmtReturn;
SpkUnknown *Spk_stmtWhile;
SpkUnknown *Spk_stmtYield;
SpkUnknown *Spk_symbolNodeForSymbol;

SpkSpecialSelector Spk_operSelectors[Spk_NUM_OPER] = {
    { &Spk___succ__,   0 },
    { &Spk___pred__,   0 },
    { &Spk___addr__,   0 }, /* "&x" -- not implemented */
    { &Spk___ind__,    0 }, /* "*x" -- not implemented */
    { &Spk___pos__,    0 },
    { &Spk___neg__,    0 },
    { &Spk___bneg__,   0 },
    { &Spk___lneg__,   0 },
    { &Spk___mul__,    1 },
    { &Spk___div__,    1 },
    { &Spk___mod__,    1 },
    { &Spk___add__,    1 },
    { &Spk___sub__,    1 },
    { &Spk___lshift__, 1 },
    { &Spk___rshift__, 1 },
    { &Spk___lt__,     1 },
    { &Spk___gt__,     1 },
    { &Spk___le__,     1 },
    { &Spk___ge__,     1 },
    { &Spk___eq__,     1 },
    { &Spk___ne__,     1 },
    { &Spk___band__,   1 },
    { &Spk___bxor__,   1 },
    { &Spk___bor__,    1 }
};

SpkSpecialSelector Spk_operCallSelectors[Spk_NUM_CALL_OPER] = {
    { &Spk___apply__,  0 },
    { &Spk___index__,  0 }
};


/* integers */
SpkUnknown *Spk_0;
SpkUnknown *Spk_1;


/* strings */
SpkUnknown *Spk_emptyString; static char emptyString[] = "";
SpkUnknown *Spk_unknownSelector; static char unknownSelector[] = "<unknown>";
SpkUnknown *Spk_unknownSourcePathname; static char unknownSourcePathname[] = "<filename>";


/* arguments */
SpkUnknown *Spk_emptyArgs;


static SymbolTableEntry symbolTable[] = {
    { &Spk_self, "self" },
    { 0 }
};

static SymbolTableEntry selectorTable[] = {
    { &Spk___add__,                       "__add__"                     },
    { &Spk___addr__,                      "__addr__"                    },
    { &Spk___apply__,                     "__apply__"                   },
    { &Spk___band__,                      "__band__"                    },
    { &Spk___bneg__,                      "__bneg__"                    },
    { &Spk___bor__,                       "__bor__"                     },
    { &Spk___bxor__,                      "__bxor__"                    },
    { &Spk___div__,                       "__div__"                     },
    { &Spk___eq__,                        "__eq__"                      },
    { &Spk___ge__,                        "__ge__"                      },
    { &Spk___gt__,                        "__gt__"                      },
    { &Spk___ind__,                       "__ind__"                     },
    { &Spk___index__,                     "__index__"                   },
    { &Spk___le__,                        "__le__"                      },
    { &Spk___lneg__,                      "__lneg__"                    },
    { &Spk___lshift__,                    "__lshift__"                  },
    { &Spk___lt__,                        "__lt__"                      },
    { &Spk___mod__,                       "__mod__"                     },
    { &Spk___mul__,                       "__mul__"                     },
    { &Spk___ne__,                        "__ne__"                      },
    { &Spk___neg__,                       "__neg__"                     },
    { &Spk___pos__,                       "__pos__"                     },
    { &Spk___pred__,                      "__pred__"                    },
    { &Spk___rshift__,                    "__rshift__"                  },
    { &Spk___str__,                       "__str__"                     },
    { &Spk___sub__,                       "__sub__"                     },
    { &Spk___succ__,                      "__succ__"                    },
    { &Spk__import,                       "_import"                     },
    { &Spk__init,                         "_init"                       },
    { &Spk__predef,                       "_predef"                     },
    { &Spk__thunk,                        "_thunk"                      },
    { &Spk_badExpr,                       "badExpr:"                    },
    { &Spk_blockCopy,                     "blockCopy"                   },
    { &Spk_cannotReenterBlock,            "cannotReenterBlock"          },
    { &Spk_cannotReturn,                  "cannotReturn"                },
    { &Spk_compoundExpression,            "compoundExpression"          },
    { &Spk_doesNotUnderstand,             "doesNotUnderstand:"          },
    { &Spk_failOnError,                   "failOnError"                 },
    { &Spk_importModule,                  "importModule"                },
    { &Spk_importPackage,                 "importPackage"               },
    { &Spk_init,                          "init"                        },
    { &Spk_main,                          "main"                        },
    { &Spk_mustBeBoolean,                 "mustBeBoolean"               },
    { &Spk_mustBeSymbol,                  "mustBeSymbol"                },
    { &Spk_mustBeTuple,                   "mustBeTuple"                 },
    { &Spk_new,                           "new"                         },
    { &Spk_noRunnableFiber,               "noRunnableFiber"             },
    { &Spk_numArgs,                       "numArgs"                     },
    { &Spk_printString,                   "printString"                 },
    { &Spk_recursiveDoesNotUnderstand,    "recursiveDoesNotUnderstand:" },
    { &Spk_redefinedSymbol,               "redefinedSymbol:"            },
    { &Spk_source,                        "source"                      },
    { &Spk_undefinedSymbol,               "undefinedSymbol:"            },
    { &Spk_unknownOpcode,                 "unknownOpcode"               },
    { &Spk_wrongNumberOfArguments,        "wrongNumberOfArguments"      },
    
    { &Spk_Boolean,                       "Boolean"                     },
    { &Spk_False,                         "False"                       },
    { &Spk_True,                          "True"                        },
    
    { &Spk_concat,                        "concat"                      },
    { &Spk_declSpecs,                     "declSpecs"                   },
    { &Spk_exprAnd,                       "exprAnd"                     },
    { &Spk_exprAssign,                    "exprAssign"                  },
    { &Spk_exprAttr,                      "exprAttr"                    },
    { &Spk_exprAttrVar,                   "exprAttrVar"                 },
    { &Spk_exprBinaryOp,                  "exprBinaryOp"                },
    { &Spk_exprBlock,                     "exprBlock"                   },
    { &Spk_exprCall,                      "exprCall"                    },
    { &Spk_exprCompound,                  "exprCompound"                },
    { &Spk_exprCond,                      "exprCond"                    },
    { &Spk_exprId,                        "exprId"                      },
    { &Spk_exprKeyword,                   "exprKeyword"                 },
    { &Spk_exprLiteral,                   "exprLiteral"                 },
    { &Spk_exprName,                      "exprName"                    },
    { &Spk_exprNI,                        "exprNI"                      },
    { &Spk_exprOr,                        "exprOr"                      },
    { &Spk_exprPostOp,                    "exprPostOp"                  },
    { &Spk_exprPreOp,                     "exprPreOp"                   },
    { &Spk_exprUnaryOp,                   "exprUnaryOp"                 },
    { &Spk_isSpec,                        "isSpec"                      },
    { &Spk_left,                          "left"                        },
    { &Spk_next,                          "next"                        },
    { &Spk_nextArg,                       "nextArg"                     },
    { &Spk_operAdd,                       "operAdd"                     },
    { &Spk_operAddr,                      "operAddr"                    },
    { &Spk_operApply,                     "operApply"                   },
    { &Spk_operBAnd,                      "operBAnd"                    },
    { &Spk_operBNeg,                      "operBNeg"                    },
    { &Spk_operBOr,                       "operBOr"                     },
    { &Spk_operBXOr,                      "operBXOr"                    },
    { &Spk_operDiv,                       "operDiv"                     },
    { &Spk_operEq,                        "operEq"                      },
    { &Spk_operGE,                        "operGE"                      },
    { &Spk_operGT,                        "operGT"                      },
    { &Spk_operInd,                       "operInd"                     },
    { &Spk_operIndex,                     "operIndex"                   },
    { &Spk_operLE,                        "operLE"                      },
    { &Spk_operLNeg,                      "operLNeg"                    },
    { &Spk_operLShift,                    "operLShift"                  },
    { &Spk_operLT,                        "operLT"                      },
    { &Spk_operMod,                       "operMod"                     },
    { &Spk_operMul,                       "operMul"                     },
    { &Spk_operNE,                        "operNE"                      },
    { &Spk_operNeg,                       "operNeg"                     },
    { &Spk_operPos,                       "operPos"                     },
    { &Spk_operPred,                      "operPred"                    },
    { &Spk_operRShift,                    "operRShift"                  },
    { &Spk_operSub,                       "operSub"                     },
    { &Spk_operSucc,                      "operSucc"                    },
    { &Spk_stmtBreak,                     "stmtBreak"                   },
    { &Spk_stmtCompound,                  "stmtCompound"                },
    { &Spk_stmtContinue,                  "stmtContinue"                },
    { &Spk_stmtDefClass,                  "stmtDefClass"                },
    { &Spk_stmtDefMethod,                 "stmtDefMethod"               },
    { &Spk_stmtDefModule,                 "stmtDefModule"               },
    { &Spk_stmtDefSpec,                   "stmtDefSpec"                 },
    { &Spk_stmtDefVar,                    "stmtDefVar"                  },
    { &Spk_stmtDoWhile,                   "stmtDoWhile"                 },
    { &Spk_stmtExpr,                      "stmtExpr"                    },
    { &Spk_stmtFor,                       "stmtFor"                     },
    { &Spk_stmtIfElse,                    "stmtIfElse"                  },
    { &Spk_stmtPragmaSource,              "stmtPragmaSource"            },
    { &Spk_stmtReturn,                    "stmtReturn"                  },
    { &Spk_stmtWhile,                     "stmtWhile"                   },
    { &Spk_stmtYield,                     "stmtYield"                   },
    { &Spk_symbolNodeForSymbol,           "symbolNodeForSymbol:"        },
    
    { 0 }
};

static IntegerTableEntry integerTable[] = {
    { &Spk_0, 0 },
    { &Spk_1, 1 },
    { 0 }
};

static StringTableEntry stringTable[] = {
    { &Spk_emptyString, emptyString, sizeof(emptyString) },
    { &Spk_unknownSelector, unknownSelector, sizeof(unknownSelector) },
    { &Spk_unknownSourcePathname, unknownSourcePathname, sizeof(unknownSourcePathname) },
    { 0 }
};


static int initSymbols(SymbolTableEntry *t) {
    for ( ; t->p; ++t) {
        *t->p = SpkHost_SymbolFromCString(t->s);
        if (!*t->p) {
            return -1;
        }
    }
    return 0;
}

static int initSelectors(SelectorTableEntry *t) {
    for ( ; t->p; ++t) {
        *t->p = Spk_ParseSelector(t->s);
        if (!*t->p) {
            return -1;
        }
    }
    return 0;
}

static int initIntegers(IntegerTableEntry *t) {
    for ( ; t->p; ++t) {
        *t->p = SpkHost_IntegerFromCLong(t->i);
        if (!*t->p) {
            return -1;
        }
    }
    return 0;
}

static int initStrings(StringTableEntry *t) {
    for ( ; t->p; ++t) {
        *t->p = SpkHost_StringFromCStringAndLength(t->s, t->n - 1);
        if (!*t->p) {
            return -1;
        }
    }
    return 0;
}


int Spk_InitSymbols(void) {
    if (initSymbols(symbolTable) < 0)       { return -1; }
    if (initSelectors(selectorTable) < 0)   { return -1; }
    return 0;
}

int Spk_InitReadOnlyData(void) {
    if (initIntegers(integerTable) < 0)     { return -1; }
    if (initStrings(stringTable) < 0)       { return -1; }
    
    Spk_emptyArgs = SpkHost_EmptyArgs();
    if (!Spk_emptyArgs) {
        return -1;
    }
    
    return 0;
}

SpkUnknown *Spk_ParseSelector(const char *methodName) {
    SpkUnknown *selector;
    
    if (strchr(methodName, ':')) {
        /* keyword message */
        SpkUnknown *kw, *builder;
        const char *begin, *end;
        
        if (strchr(methodName, ' ') /*XXX*/ ||
            methodName[strlen(methodName) - 1] != ':') {
            Spk_Halt(Spk_HALT_ASSERTION_ERROR, "invalid method name");
            return 0;
        }
        builder = SpkHost_NewKeywordSelectorBuilder();
        for (begin = end = methodName; *end; begin = end) {
            while (*end != ':')
                ++end;
            kw = SpkHost_SymbolFromCStringAndLength(begin, end - begin);
            SpkHost_AppendKeyword(&builder, kw);
            Spk_DECREF(kw);
            ++end; /* skip ':' */
        }
        selector = SpkHost_GetKeywordSelector(builder, 0);
        Spk_DECREF(builder);
        
    } else {
        selector = SpkHost_SymbolFromCString(methodName);
    }
    
    return selector;
}


static void releaseSymbols(SymbolTableEntry *t) {
    for ( ; t->p; ++t)
        Spk_CLEAR(*t->p);
}

static void releaseSelectors(SelectorTableEntry *t) {
    for ( ; t->p; ++t)
        Spk_CLEAR(*t->p);
}

static void releaseIntegers(IntegerTableEntry *t) {
    for ( ; t->p; ++t)
        Spk_CLEAR(*t->p);
}

static void releaseStrings(StringTableEntry *t) {
    for ( ; t->p; ++t)
        Spk_CLEAR(*t->p);
}


void Spk_ReleaseSymbols(void) {
    releaseSelectors(selectorTable);
    releaseSymbols(symbolTable);
    return;
}

void Spk_ReleaseReadOnlyData(void) {
    Spk_DECREF(Spk_emptyArgs);
    releaseStrings(stringTable);
    releaseIntegers(integerTable);
}
