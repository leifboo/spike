
#ifndef __spk_module_h__
#define __spk_module_h__


#include "class.h"


enum {
    Spk_LITERAL_SYMBOL,
    Spk_LITERAL_INTEGER,
    Spk_LITERAL_FLOAT,
    Spk_LITERAL_CHAR,
    Spk_LITERAL_STRING
};


typedef struct SpkLiteralTmpl {
    unsigned int kind;
    int intValue;
    double floatValue;
    const char *stringValue;
} SpkLiteralTmpl;


typedef struct SpkModuleTmpl {
    SpkClassTmpl moduleClass;
    size_t literalCount;
    SpkLiteralTmpl *literals;
} SpkModuleTmpl;


typedef struct SpkModule {
    SpkObject base;
    size_t literalCount;
    SpkUnknown **literals;
    struct SpkBehavior *firstClass;
} SpkModule;


#define SpkModule_VARIABLES(op) ((SpkUnknown **)((char *)(op) + ((SpkObject *)(op))->klass->instVarOffset) + Spk_CLASS(Module)->instVarBaseIndex)
#define SpkModule_LITERALS(op) (((SpkModule *)(op))->literals)


extern struct SpkClassTmpl Spk_ClassModuleTmpl;


SpkModule *SpkModule_New(struct SpkBehavior *);
void SpkModule_InitFromTemplate(SpkBehavior *, SpkModuleTmpl *, SpkBehavior *);
void SpkModule_InitLiteralsFromTemplate(SpkModule *, SpkModuleTmpl *);


#endif /* __spk_module_h__ */
