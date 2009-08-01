
#ifndef __spk_metaclass_h__
#define __spk_metaclass_h__


#include "behavior.h"


typedef struct SpkMetaclass {
    SpkBehavior base;
    struct SpkClass *thisClass; /* sole instance */
    /* XXX: Should 'thisClass' be a weak reference? */
} SpkMetaclass;

typedef struct SpkMetaclassSubclass {
    SpkMetaclass base;
    SpkUnknown *variables[1]; /* stretchy */
} SpkMetaclassSubclass;


extern SpkBehavior *Spk_ClassMetaclass;
extern struct SpkClassTmpl Spk_ClassMetaclassTmpl;


SpkMetaclass *SpkMetaclass_New(SpkMetaclass *superMeta, size_t instVarCount);
SpkMetaclass *SpkMetaclass_FromTemplate(SpkBehaviorTmpl *template, SpkMetaclass *superMeta, struct SpkModule *module);


#endif /* __spk_metaclass_h__ */
