
#ifndef __class_h__
#define __class_h__


#include "behavior.h"


typedef struct ClassTmpl {
    /* C code */
    const char *name;
    size_t classVarOffset;
    size_t superclassVarOffset;
    BehaviorTmpl thisClass;
    BehaviorTmpl metaclass;
    /* bytecode */
    struct ClassTmpl *next, *nextInScope;
    Unknown *symbol;
    size_t classVarIndex;
    size_t superclassVarIndex;
    Unknown *superclassName; /* XXX: kludge for C bytecode gen */
} ClassTmpl;


typedef struct Class {
    Behavior base;
    Unknown *name;
} Class;


extern struct ClassTmpl ClassClassTmpl;


Class *Class_New(Unknown *name, Behavior *superclass, size_t instVarCount, size_t classVarCount);
Class *Class_EmptyFromTemplate(ClassTmpl *tmpl, Behavior *superclass, Behavior *Metaclass, struct Module *module);
Class *Class_FromTemplate(ClassTmpl *tmpl, Behavior *superclass, struct Module *module);
void Class_InitFromTemplate(Class *self, ClassTmpl *tmpl, Behavior *superclass, struct Module *module);
Unknown *Class_Name(Class *);


#define CLASS_TMPL(n, s, m) #n, offsetof(m, n), offsetof(m, s)


#endif /* __class_h__ */
