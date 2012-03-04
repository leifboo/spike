
#ifndef __module_h__
#define __module_h__


#include "class.h"


enum {
    LITERAL_SYMBOL,
    LITERAL_INTEGER,
    LITERAL_FLOAT,
    LITERAL_CHAR,
    LITERAL_STRING
};


typedef struct LiteralTmpl {
    unsigned int kind;
    int intValue;
    double floatValue;
    const char *stringValue;
} LiteralTmpl;


typedef struct ModuleTmpl {
    /* C code */
    ClassTmpl moduleClass;
    size_t literalCount;
    LiteralTmpl *literalTable;
    /* bytecode */
    ClassTmplList classList;
    Unknown **literals;
} ModuleTmpl;


typedef struct Module {
    Object base;
} Module;


typedef struct ModuleClass {
    Class base;
    size_t literalCount;
    Unknown **literals;
    ModuleTmpl *tmpl;
} ModuleClass;


#define Module_VARIABLES(op) ((Unknown **)((char *)(op) + ((Object *)(op))->klass->instVarOffset) + CLASS(Module)->instVarBaseIndex)
#define Module_LITERALS(op) (((ModuleClass *)(op->base.klass))->literals)


extern struct ClassTmpl ClassModuleTmpl, ClassThunkTmpl;


void Module_InitLiteralsFromTemplate(Behavior *, ModuleTmpl *);
ModuleClass *ModuleClass_New(ModuleTmpl *);


#endif /* __module_h__ */
