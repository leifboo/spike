
#include "rodata.h"

#include "host.h"
#include "native.h"
#include "obj.h"
#include <stddef.h>
#include <string.h>


typedef struct { Unknown **p; const char *s; }           SymbolTableEntry;
typedef SymbolTableEntry                                    SelectorTableEntry;
typedef struct { Unknown **p; long i; }                  IntegerTableEntry;
typedef struct { Unknown **p; const char *s; size_t n; } StringTableEntry;


/* symbols */
Unknown *self;


/* selectors */
Unknown *__add__;
Unknown *__addr__;
Unknown *__apply__;
Unknown *__band__;
Unknown *__bneg__;
Unknown *__bor__;
Unknown *__bxor__;
Unknown *__div__;
Unknown *__eq__;
Unknown *__ge__;
Unknown *__gt__;
Unknown *__ind__;
Unknown *__index__;
Unknown *__le__;
Unknown *__lneg__;
Unknown *__lshift__;
Unknown *__lt__;
Unknown *__mod__;
Unknown *__mul__;
Unknown *__ne__;
Unknown *__neg__;
Unknown *__pos__;
Unknown *__pred__;
Unknown *__rshift__;
Unknown *__str__;
Unknown *__sub__;
Unknown *__succ__;
Unknown *_import;
Unknown *_init;
Unknown *_predef;
Unknown *_thunk;
Unknown *badExpr;
Unknown *blockCopy;
Unknown *cannotReenterBlock;
Unknown *cannotReturn;
Unknown *compoundExpression;
Unknown *doesNotUnderstand;
Unknown *failOnError;
Unknown *importModule;
Unknown *importPackage;
Unknown *init;
Unknown *_main;
Unknown *mustBeBoolean;
Unknown *mustBeSymbol;
Unknown *mustBeTuple;
Unknown *new;
Unknown *noRunnableFiber;
Unknown *numArgs;
Unknown *printString;
Unknown *recursiveDoesNotUnderstand;
Unknown *redefinedSymbol;
Unknown *source;
Unknown *undefinedSymbol;
Unknown *unknownOpcode;
Unknown *wrongNumberOfArguments;

/* selectors -- class names */
Unknown *Boolean;
Unknown *False;
Unknown *True;

/* selectors -- parser */
Unknown *concat;
Unknown *declSpecs;
Unknown *exprAnd;
Unknown *exprAssign;
Unknown *exprAttr;
Unknown *exprAttrVar;
Unknown *exprBinaryOp;
Unknown *exprBlock;
Unknown *exprCall;
Unknown *exprCompound;
Unknown *exprCond;
Unknown *exprId;
Unknown *exprKeyword;
Unknown *exprLiteral;
Unknown *exprName;
Unknown *exprNI;
Unknown *exprOr;
Unknown *exprPostOp;
Unknown *exprPreOp;
Unknown *exprUnaryOp;
Unknown *isSpec;
Unknown *left;
Unknown *next;
Unknown *nextArg;
Unknown *operAdd;
Unknown *operAddr;
Unknown *operApply;
Unknown *operBAnd;
Unknown *operBNeg;
Unknown *operBOr;
Unknown *operBXOr;
Unknown *operDiv;
Unknown *operEq;
Unknown *operGE;
Unknown *operGT;
Unknown *operInd;
Unknown *operIndex;
Unknown *operLE;
Unknown *operLNeg;
Unknown *operLShift;
Unknown *operLT;
Unknown *operMod;
Unknown *operMul;
Unknown *operNE;
Unknown *operNeg;
Unknown *operPos;
Unknown *operPred;
Unknown *operRShift;
Unknown *operSub;
Unknown *operSucc;
Unknown *stmtBreak;
Unknown *stmtCompound;
Unknown *stmtContinue;
Unknown *stmtDefClass;
Unknown *stmtDefMethod;
Unknown *stmtDefModule;
Unknown *stmtDefSpec;
Unknown *stmtDefVar;
Unknown *stmtDoWhile;
Unknown *stmtExpr;
Unknown *stmtFor;
Unknown *stmtIfElse;
Unknown *stmtPragmaSource;
Unknown *stmtReturn;
Unknown *stmtWhile;
Unknown *stmtYield;
Unknown *symbolNodeForSymbol;

SpecialSelector operSelectors[NUM_OPER] = {
    { &__succ__,   0 },
    { &__pred__,   0 },
    { &__addr__,   0 }, /* "&x" -- not implemented */
    { &__ind__,    0 }, /* "*x" -- not implemented */
    { &__pos__,    0 },
    { &__neg__,    0 },
    { &__bneg__,   0 },
    { &__lneg__,   0 },
    { &__mul__,    1 },
    { &__div__,    1 },
    { &__mod__,    1 },
    { &__add__,    1 },
    { &__sub__,    1 },
    { &__lshift__, 1 },
    { &__rshift__, 1 },
    { &__lt__,     1 },
    { &__gt__,     1 },
    { &__le__,     1 },
    { &__ge__,     1 },
    { &__eq__,     1 },
    { &__ne__,     1 },
    { &__band__,   1 },
    { &__bxor__,   1 },
    { &__bor__,    1 }
};

SpecialSelector operCallSelectors[NUM_CALL_OPER] = {
    { &__apply__,  0 },
    { &__index__,  0 }
};


/* integers */
Unknown *zero;
Unknown *one;


/* strings */
Unknown *emptyString; static char strEmptyString[] = "";
Unknown *unknownSelector; static char strUnknownSelector[] = "<unknown>";
Unknown *unknownSourcePathname; static char strUnknownSourcePathname[] = "<filename>";


/* arguments */
Unknown *emptyArgs;


static SymbolTableEntry symbolTable[] = {
    { &self, "self" },
    { 0 }
};

static SymbolTableEntry selectorTable[] = {
    { &__add__,                       "__add__"                     },
    { &__addr__,                      "__addr__"                    },
    { &__apply__,                     "__apply__"                   },
    { &__band__,                      "__band__"                    },
    { &__bneg__,                      "__bneg__"                    },
    { &__bor__,                       "__bor__"                     },
    { &__bxor__,                      "__bxor__"                    },
    { &__div__,                       "__div__"                     },
    { &__eq__,                        "__eq__"                      },
    { &__ge__,                        "__ge__"                      },
    { &__gt__,                        "__gt__"                      },
    { &__ind__,                       "__ind__"                     },
    { &__index__,                     "__index__"                   },
    { &__le__,                        "__le__"                      },
    { &__lneg__,                      "__lneg__"                    },
    { &__lshift__,                    "__lshift__"                  },
    { &__lt__,                        "__lt__"                      },
    { &__mod__,                       "__mod__"                     },
    { &__mul__,                       "__mul__"                     },
    { &__ne__,                        "__ne__"                      },
    { &__neg__,                       "__neg__"                     },
    { &__pos__,                       "__pos__"                     },
    { &__pred__,                      "__pred__"                    },
    { &__rshift__,                    "__rshift__"                  },
    { &__str__,                       "__str__"                     },
    { &__sub__,                       "__sub__"                     },
    { &__succ__,                      "__succ__"                    },
    { &_import,                       "_import"                     },
    { &_init,                         "_init"                       },
    { &_predef,                       "_predef"                     },
    { &_thunk,                        "_thunk"                      },
    { &badExpr,                       "badExpr:"                    },
    { &blockCopy,                     "blockCopy"                   },
    { &cannotReenterBlock,            "cannotReenterBlock"          },
    { &cannotReturn,                  "cannotReturn"                },
    { &compoundExpression,            "compoundExpression"          },
    { &doesNotUnderstand,             "doesNotUnderstand:"          },
    { &failOnError,                   "failOnError"                 },
    { &importModule,                  "importModule"                },
    { &importPackage,                 "importPackage"               },
    { &init,                          "init"                        },
    { &_main,                         "main"                        },
    { &mustBeBoolean,                 "mustBeBoolean"               },
    { &mustBeSymbol,                  "mustBeSymbol"                },
    { &mustBeTuple,                   "mustBeTuple"                 },
    { &new,                           "new"                         },
    { &noRunnableFiber,               "noRunnableFiber"             },
    { &numArgs,                       "numArgs"                     },
    { &printString,                   "printString"                 },
    { &recursiveDoesNotUnderstand,    "recursiveDoesNotUnderstand:" },
    { &redefinedSymbol,               "redefinedSymbol:"            },
    { &source,                        "source"                      },
    { &undefinedSymbol,               "undefinedSymbol:"            },
    { &unknownOpcode,                 "unknownOpcode"               },
    { &wrongNumberOfArguments,        "wrongNumberOfArguments"      },
    
    { &Boolean,                       "Boolean"                     },
    { &False,                         "False"                       },
    { &True,                          "True"                        },
    
    { &concat,                        "concat"                      },
    { &declSpecs,                     "declSpecs"                   },
    { &exprAnd,                       "exprAnd"                     },
    { &exprAssign,                    "exprAssign"                  },
    { &exprAttr,                      "exprAttr"                    },
    { &exprAttrVar,                   "exprAttrVar"                 },
    { &exprBinaryOp,                  "exprBinaryOp"                },
    { &exprBlock,                     "exprBlock"                   },
    { &exprCall,                      "exprCall"                    },
    { &exprCompound,                  "exprCompound"                },
    { &exprCond,                      "exprCond"                    },
    { &exprId,                        "exprId"                      },
    { &exprKeyword,                   "exprKeyword"                 },
    { &exprLiteral,                   "exprLiteral"                 },
    { &exprName,                      "exprName"                    },
    { &exprNI,                        "exprNI"                      },
    { &exprOr,                        "exprOr"                      },
    { &exprPostOp,                    "exprPostOp"                  },
    { &exprPreOp,                     "exprPreOp"                   },
    { &exprUnaryOp,                   "exprUnaryOp"                 },
    { &isSpec,                        "isSpec"                      },
    { &left,                          "left"                        },
    { &next,                          "next"                        },
    { &nextArg,                       "nextArg"                     },
    { &operAdd,                       "operAdd"                     },
    { &operAddr,                      "operAddr"                    },
    { &operApply,                     "operApply"                   },
    { &operBAnd,                      "operBAnd"                    },
    { &operBNeg,                      "operBNeg"                    },
    { &operBOr,                       "operBOr"                     },
    { &operBXOr,                      "operBXOr"                    },
    { &operDiv,                       "operDiv"                     },
    { &operEq,                        "operEq"                      },
    { &operGE,                        "operGE"                      },
    { &operGT,                        "operGT"                      },
    { &operInd,                       "operInd"                     },
    { &operIndex,                     "operIndex"                   },
    { &operLE,                        "operLE"                      },
    { &operLNeg,                      "operLNeg"                    },
    { &operLShift,                    "operLShift"                  },
    { &operLT,                        "operLT"                      },
    { &operMod,                       "operMod"                     },
    { &operMul,                       "operMul"                     },
    { &operNE,                        "operNE"                      },
    { &operNeg,                       "operNeg"                     },
    { &operPos,                       "operPos"                     },
    { &operPred,                      "operPred"                    },
    { &operRShift,                    "operRShift"                  },
    { &operSub,                       "operSub"                     },
    { &operSucc,                      "operSucc"                    },
    { &stmtBreak,                     "stmtBreak"                   },
    { &stmtCompound,                  "stmtCompound"                },
    { &stmtContinue,                  "stmtContinue"                },
    { &stmtDefClass,                  "stmtDefClass"                },
    { &stmtDefMethod,                 "stmtDefMethod"               },
    { &stmtDefModule,                 "stmtDefModule"               },
    { &stmtDefSpec,                   "stmtDefSpec"                 },
    { &stmtDefVar,                    "stmtDefVar"                  },
    { &stmtDoWhile,                   "stmtDoWhile"                 },
    { &stmtExpr,                      "stmtExpr"                    },
    { &stmtFor,                       "stmtFor"                     },
    { &stmtIfElse,                    "stmtIfElse"                  },
    { &stmtPragmaSource,              "stmtPragmaSource"            },
    { &stmtReturn,                    "stmtReturn"                  },
    { &stmtWhile,                     "stmtWhile"                   },
    { &stmtYield,                     "stmtYield"                   },
    { &symbolNodeForSymbol,           "symbolNodeForSymbol:"        },
    
    { 0 }
};

static IntegerTableEntry integerTable[] = {
    { &zero, 0 },
    { &one, 1 },
    { 0 }
};

static StringTableEntry stringTable[] = {
    { &emptyString, strEmptyString, sizeof(strEmptyString) },
    { &unknownSelector, strUnknownSelector, sizeof(strUnknownSelector) },
    { &unknownSourcePathname, strUnknownSourcePathname, sizeof(strUnknownSourcePathname) },
    { 0 }
};


static int initSymbols(SymbolTableEntry *t) {
    for ( ; t->p; ++t) {
        *t->p = Host_SymbolFromCString(t->s);
        if (!*t->p) {
            return -1;
        }
    }
    return 0;
}

static int initSelectors(SelectorTableEntry *t) {
    for ( ; t->p; ++t) {
        *t->p = ParseSelector(t->s);
        if (!*t->p) {
            return -1;
        }
    }
    return 0;
}

static int initIntegers(IntegerTableEntry *t) {
    for ( ; t->p; ++t) {
        *t->p = Host_IntegerFromCLong(t->i);
        if (!*t->p) {
            return -1;
        }
    }
    return 0;
}

static int initStrings(StringTableEntry *t) {
    for ( ; t->p; ++t) {
        *t->p = Host_StringFromCStringAndLength(t->s, t->n - 1);
        if (!*t->p) {
            return -1;
        }
    }
    return 0;
}


int InitSymbols(void) {
    if (initSymbols(symbolTable) < 0)       { return -1; }
    if (initSelectors(selectorTable) < 0)   { return -1; }
    return 0;
}

int InitReadOnlyData(void) {
    if (initIntegers(integerTable) < 0)     { return -1; }
    if (initStrings(stringTable) < 0)       { return -1; }
    
    emptyArgs = Host_EmptyArgs();
    if (!emptyArgs) {
        return -1;
    }
    
    return 0;
}

Unknown *ParseSelector(const char *methodName) {
    Unknown *selector;
    
    if (strchr(methodName, ':')) {
        /* keyword message */
        Unknown *kw, *builder;
        const char *begin, *end;
        
        if (strchr(methodName, ' ') /*XXX*/ ||
            methodName[strlen(methodName) - 1] != ':') {
            Halt(HALT_ASSERTION_ERROR, "invalid method name");
            return 0;
        }
        builder = Host_NewKeywordSelectorBuilder();
        for (begin = end = methodName; *end; begin = end) {
            while (*end != ':')
                ++end;
            kw = Host_SymbolFromCStringAndLength(begin, end - begin);
            Host_AppendKeyword(&builder, kw);
            DECREF(kw);
            ++end; /* skip ':' */
        }
        selector = Host_GetKeywordSelector(builder, 0);
        DECREF(builder);
        
    } else {
        selector = Host_SymbolFromCString(methodName);
    }
    
    return selector;
}


static void releaseSymbols(SymbolTableEntry *t) {
    for ( ; t->p; ++t)
        CLEAR(*t->p);
}

static void releaseSelectors(SelectorTableEntry *t) {
    for ( ; t->p; ++t)
        CLEAR(*t->p);
}

static void releaseIntegers(IntegerTableEntry *t) {
    for ( ; t->p; ++t)
        CLEAR(*t->p);
}

static void releaseStrings(StringTableEntry *t) {
    for ( ; t->p; ++t)
        CLEAR(*t->p);
}


void ReleaseSymbols(void) {
    releaseSelectors(selectorTable);
    releaseSymbols(symbolTable);
    return;
}

void ReleaseReadOnlyData(void) {
    DECREF(emptyArgs);
    releaseStrings(stringTable);
    releaseIntegers(integerTable);
}
