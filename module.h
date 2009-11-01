
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
    /* C code */
    SpkClassTmpl moduleClass;
    size_t literalCount;
    SpkLiteralTmpl *literalTable;
    /* bytecode */
    SpkClassTmplList classList;
    SpkUnknown **literals;
} SpkModuleTmpl;


typedef struct SpkModule {
    SpkObject base;
} SpkModule;


typedef struct SpkModuleClass {
    SpkClass base;
    size_t literalCount;
    SpkUnknown **literals;
    SpkModuleTmpl *tmpl;
} SpkModuleClass;


#define SpkModule_VARIABLES(op) ((SpkUnknown **)((char *)(op) + ((SpkObject *)(op))->klass->instVarOffset) + Spk_CLASS(Module)->instVarBaseIndex)
#define SpkModule_LITERALS(op) (((SpkModuleClass *)(op->base.klass))->literals)


extern struct SpkClassTmpl Spk_ClassModuleTmpl;


void SpkModule_InitLiteralsFromTemplate(SpkBehavior *, SpkModuleTmpl *);
SpkModuleClass *SpkModuleClass_New(SpkModuleTmpl *);


#endif /* __spk_module_h__ */
