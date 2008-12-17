
#ifndef __module_h__
#define __module_h__


#include "obj.h"


struct Symbol;


typedef struct Module {
    VariableObject base;
    struct IdentityDictionary *globals;
    struct Behavior *firstClass;
} Module;

typedef struct ModuleSubclass {
    Module base;
    Object *variables[1]; /* stretchy */
} ModuleSubclass;

#define SpkModule_VARIABLES(op) ((Object **)SpkVariableObject_ITEM_BASE(op))


extern struct Behavior *Spk_ClassModule;
extern struct SpkClassTmpl Spk_ClassModuleTmpl;
extern Module *Spk_builtInModule;


Module *SpkModule_new(unsigned int, struct IdentityDictionary *);
Object *SpkModule_lookupSymbol(Module *, struct Symbol *);


#endif /* __module_h__ */
