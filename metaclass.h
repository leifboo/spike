
#ifndef __spk_metaclass_h__
#define __spk_metaclass_h__


#include "behavior.h"


struct SpkModule;


typedef struct SpkMetaclass {
    SpkBehavior base;
    struct SpkClass *thisClass; /* sole instance */
    /* XXX: Should 'thisClass' be a weak reference? */
} SpkMetaclass;


extern struct SpkClassTmpl Spk_ClassMetaclassTmpl;


SpkMetaclass *SpkMetaclass_New(SpkMetaclass *superMeta, size_t instVarCount);
SpkMetaclass *SpkMetaclass_FromTemplate(struct SpkBehaviorTmpl *template, SpkMetaclass *superMeta, struct SpkModule *module);


#endif /* __spk_metaclass_h__ */
