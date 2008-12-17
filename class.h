
#ifndef __class_h__
#define __class_h__


#include "behavior.h"


typedef struct Class {
    Behavior base;
    struct Symbol *name;
} Class;

typedef struct ClassSubclass {
    Class base;
    Object *variables[1]; /* stretchy */
} ClassSubclass;


extern struct Behavior *Spk_ClassClass;
extern struct SpkClassTmpl Spk_ClassClassTmpl;


Class *SpkClass_new(struct Symbol *name);
void SpkClass_initFromTemplate(Class *self, SpkClassTmpl *template, Behavior *superclass, struct Module *module);
void SpkClass_addMethodsFromTemplate(Class *self, SpkClassTmpl *template);


#endif /* __class_h__ */
