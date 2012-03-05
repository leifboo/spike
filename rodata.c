
#include "rodata.h"

#include "array.h"
#include "int.h"
#include "kwsel.h"
#include "native.h"
#include "obj.h"
#include "str.h"
#include "sym.h"

#include <stddef.h>
#include <string.h>


typedef struct { Symbol **p; const char *s; }            SymbolTableEntry;
typedef SymbolTableEntry                                 SelectorTableEntry;
typedef struct { Integer **p; long i; }                  IntegerTableEntry;
typedef struct { String **p; const char *s; size_t n; }  StringTableEntry;


/* symbols */
Symbol *self;


/* selectors */
Symbol *__add__;
Symbol *__addr__;
Symbol *__apply__;
Symbol *__band__;
Symbol *__bneg__;
Symbol *__bor__;
Symbol *__bxor__;
Symbol *__div__;
Symbol *__eq__;
Symbol *__ge__;
Symbol *__gt__;
Symbol *__ind__;
Symbol *__index__;
Symbol *__le__;
Symbol *__lneg__;
Symbol *__lshift__;
Symbol *__lt__;
Symbol *__mod__;
Symbol *__mul__;
Symbol *__ne__;
Symbol *__neg__;
Symbol *__pos__;
Symbol *__pred__;
Symbol *__rshift__;
Symbol *__str__;
Symbol *__sub__;
Symbol *__succ__;
Symbol *_import;
Symbol *_init;
Symbol *_predef;
Symbol *_thunk;
Symbol *badExpr;
Symbol *blockCopy;
Symbol *cannotReenterBlock;
Symbol *cannotReturn;
Symbol *compoundExpression;
Symbol *doesNotUnderstand;
Symbol *failOnError;
Symbol *importModule;
Symbol *importPackage;
Symbol *init;
Symbol *_main;
Symbol *mustBeBoolean;
Symbol *mustBeSymbol;
Symbol *mustBeTuple;
Symbol *new;
Symbol *noRunnableFiber;
Symbol *numArgs;
Symbol *printString;
Symbol *recursiveDoesNotUnderstand;
Symbol *redefinedSymbol;
Symbol *source;
Symbol *undefinedSymbol;
Symbol *unknownOpcode;
Symbol *wrongNumberOfArguments;

/* selectors -- class names */
Symbol *Boolean;
Symbol *False;
Symbol *True;

/* selectors -- parser */
Symbol *concat;
Symbol *declSpecs;
Symbol *exprAnd;
Symbol *exprAssign;
Symbol *exprAttr;
Symbol *exprAttrVar;
Symbol *exprBinaryOp;
Symbol *exprBlock;
Symbol *exprCall;
Symbol *exprCompound;
Symbol *exprCond;
Symbol *exprId;
Symbol *exprKeyword;
Symbol *exprLiteral;
Symbol *exprName;
Symbol *exprNI;
Symbol *exprOr;
Symbol *exprPostOp;
Symbol *exprPreOp;
Symbol *exprUnaryOp;
Symbol *isSpec;
Symbol *left;
Symbol *next;
Symbol *nextArg;
Symbol *operAdd;
Symbol *operAddr;
Symbol *operApply;
Symbol *operBAnd;
Symbol *operBNeg;
Symbol *operBOr;
Symbol *operBXOr;
Symbol *operDiv;
Symbol *operEq;
Symbol *operGE;
Symbol *operGT;
Symbol *operInd;
Symbol *operIndex;
Symbol *operLE;
Symbol *operLNeg;
Symbol *operLShift;
Symbol *operLT;
Symbol *operMod;
Symbol *operMul;
Symbol *operNE;
Symbol *operNeg;
Symbol *operPos;
Symbol *operPred;
Symbol *operRShift;
Symbol *operSub;
Symbol *operSucc;
Symbol *stmtBreak;
Symbol *stmtCompound;
Symbol *stmtContinue;
Symbol *stmtDefClass;
Symbol *stmtDefMethod;
Symbol *stmtDefModule;
Symbol *stmtDefSpec;
Symbol *stmtDefVar;
Symbol *stmtDoWhile;
Symbol *stmtExpr;
Symbol *stmtFor;
Symbol *stmtIfElse;
Symbol *stmtPragmaSource;
Symbol *stmtReturn;
Symbol *stmtWhile;
Symbol *stmtYield;
Symbol *symbolNodeForSymbol;

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
Integer *zero;
Integer *one;


/* strings */
String *emptyString; static char strEmptyString[] = "";
String *unknownSelector; static char strUnknownSelector[] = "<unknown>";
String *unknownSourcePathname; static char strUnknownSourcePathname[] = "<filename>";


/* arguments */
Array *emptyArgs;


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
        *t->p = Symbol_FromCString(t->s);
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
        *t->p = Integer_FromCLong(t->i);
        if (!*t->p) {
            return -1;
        }
    }
    return 0;
}

static int initStrings(StringTableEntry *t) {
    for ( ; t->p; ++t) {
        *t->p = String_FromCStringAndLength(t->s, t->n - 1);
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
    
    emptyArgs = Array_New(0);
    if (!emptyArgs) {
        return -1;
    }
    
    return 0;
}

Symbol *ParseSelector(const char *methodName) {
    Symbol *selector;
    
    if (strchr(methodName, ':')) {
        /* keyword message */
        Symbol *kw;
        Unknown *builder;
        const char *begin, *end;
        
        if (strchr(methodName, ' ') /*XXX*/ ||
            methodName[strlen(methodName) - 1] != ':') {
            Halt(HALT_ASSERTION_ERROR, "invalid method name");
            return 0;
        }
        builder = NewKeywordSelectorBuilder();
        for (begin = end = methodName; *end; begin = end) {
            while (*end != ':')
                ++end;
            kw = Symbol_FromCStringAndLength(begin, end - begin);
            AppendKeyword(&builder, kw);
            ++end; /* skip ':' */
        }
        selector = GetKeywordSelector(builder, 0);
        
    } else {
        selector = Symbol_FromCString(methodName);
    }
    
    return selector;
}
