
#ifndef __module_h__
#define __module_h__


#include "obj.h"


struct Symbol;


typedef struct Module {
    Object base;
    Object **literals;
    struct Behavior *firstClass;
} Module;

typedef struct ModuleSubclass {
    Module base;
    Object *variables[1]; /* stretchy */
} ModuleSubclass;

#define SpkModule_VARIABLES(op) ((Object **)(((char *)op) + (op)->base.klass->instVarOffset))
#define SpkModule_LITERALS(op) ((op)->literals)


extern struct Behavior *Spk_ClassModule;
extern struct SpkClassTmpl Spk_ClassModuleTmpl;
extern Module *Spk_builtInModule;


Module *SpkModule_new(struct Behavior *);


#endif /* __module_h__ */
