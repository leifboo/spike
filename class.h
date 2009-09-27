
#ifndef __spk_class_h__
#define __spk_class_h__


#include "behavior.h"


typedef struct SpkClassTmpl {
    const char *name;
    size_t classVarOffset;
    size_t superclassVarOffset;
    SpkBehaviorTmpl thisClass;
    SpkBehaviorTmpl metaclass;
} SpkClassTmpl;


typedef struct SpkClass {
    SpkBehavior base;
    SpkUnknown *name;
} SpkClass;


extern struct SpkClassTmpl Spk_ClassClassTmpl;


SpkClass *SpkClass_New(SpkUnknown *name, SpkBehavior *superclass, size_t instVarCount, size_t classVarCount);
SpkClass *SpkClass_EmptyFromTemplate(SpkClassTmpl *tmpl, SpkBehavior *superclass, SpkBehavior *Metaclass, struct SpkModule *module);
SpkClass *SpkClass_FromTemplate(SpkClassTmpl *tmpl, SpkBehavior *superclass, struct SpkModule *module);
void SpkClass_InitFromTemplate(SpkClass *self, SpkClassTmpl *tmpl, SpkBehavior *superclass, struct SpkModule *module);
SpkUnknown *SpkClass_Name(SpkClass *);


#define Spk_CLASS_TMPL(n, s, m) #n, offsetof(m, n), offsetof(m, s)


#endif /* __spk_class_h__ */
