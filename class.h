
#ifndef __class_h__
#define __class_h__


#include "behavior.h"


typedef struct Class {
    Behavior base;
    Symbol *name;
} Class;

typedef struct ClassSubclass {
    Class base;
    Object *variables[1]; /* stretchy */
} ClassSubclass;


extern struct Metaclass *ClassClass;
extern struct SpkClassTmpl ClassClassTmpl;


void SpkClass_initFromTemplate(Class *self, SpkClassTmpl *template, Behavior *superclass, struct Module *module);


#endif /* __class_h__ */
