
#include "rodata.h"

#include "host.h"
#include "native.h"
#include "obj.h"
#include <stddef.h>
#include <string.h>


typedef struct { SpkUnknown **p; char *s; }           SymbolTableEntry;
typedef SymbolTableEntry                              SelectorTableEntry;
typedef struct { SpkUnknown **p; long i; }            IntegerTableEntry;
typedef struct { SpkUnknown **p; char *s; size_t n; } StringTableEntry;


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

SpkUnknown *Spk_Boolean;
SpkUnknown *Spk_False;
SpkUnknown *Spk_True;

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
    
    { 0 }
};

static IntegerTableEntry integerTable[] = {
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
