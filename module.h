
#ifndef __module_h__
#define __module_h__


#include "obj.h"


typedef struct Module {
    Object base;
    struct Behavior *firstClass;
} Module;

typedef struct ModuleSubclass {
    Module base;
    Object *variables[1]; /* stretchy */
} ModuleSubclass;


extern struct Behavior *ClassModule;
extern struct SpkClassTmpl ClassModuleTmpl;
extern Module *builtInModule;


Module *SpkModule_new(unsigned int);


#endif /* __module_h__ */
