
#ifndef __metaclass_h__
#define __metaclass_h__


#include "behavior.h"


struct Module;


typedef struct Metaclass {
    Behavior base;
    struct Class *thisClass; /* sole instance */
    /* XXX: Should 'thisClass' be a weak reference? */
} Metaclass;


extern struct ClassTmpl ClassMetaclassTmpl;


Metaclass *Metaclass_New(Metaclass *superMeta, size_t instVarCount);
Metaclass *Metaclass_FromTemplate(struct BehaviorTmpl *tmpl, Metaclass *superMeta, struct Module *module);


#endif /* __metaclass_h__ */
