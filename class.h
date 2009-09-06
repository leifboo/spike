
#ifndef __spk_class_h__
#define __spk_class_h__


#include "behavior.h"


typedef struct SpkClassTmpl {
    const char *name;
    SpkBehaviorTmpl thisClass;
    SpkBehaviorTmpl metaclass;
} SpkClassTmpl;


typedef struct SpkClass {
    SpkBehavior base;
    SpkUnknown *name;
} SpkClass;


extern struct SpkBehavior *Spk_ClassClass;
extern struct SpkClassTmpl Spk_ClassClassTmpl;


SpkClass *SpkClass_New(SpkUnknown *name, SpkBehavior *superclass, size_t instVarCount, size_t classVarCount);
SpkClass *SpkClass_EmptyFromTemplate(SpkClassTmpl *template, SpkBehavior *superclass, struct SpkModule *module);
SpkClass *SpkClass_FromTemplate(SpkClassTmpl *template, SpkBehavior *superclass, struct SpkModule *module);
void SpkClass_InitFromTemplate(SpkClass *self, SpkClassTmpl *template, SpkBehavior *superclass, struct SpkModule *module);
SpkUnknown *SpkClass_Name(SpkClass *);


#endif /* __spk_class_h__ */
