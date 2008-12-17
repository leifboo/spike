
#ifndef __metaclass_h__
#define __metaclass_h__


#include "behavior.h"


typedef struct Metaclass {
    Behavior base;
    /*struct Class *thisClass;*/ /* sole instance */
} Metaclass;

typedef struct MetaclassSubclass {
    Metaclass base;
    Object *variables[1]; /* stretchy */
} MetaclassSubclass;


extern Metaclass *Spk_ClassMetaclass;
extern struct SpkClassTmpl Spk_ClassMetaclassTmpl;


#endif /* __metaclass_h__ */
