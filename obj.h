
#ifndef __spk_obj_h__
#define __spk_obj_h__


#include "om.h"
#include <stddef.h>


typedef struct SpkObject {
    SpkUnknown base;
    struct SpkBehavior *klass;
} SpkObject;

typedef struct SpkObjectSubclass {
    SpkObject base;
    SpkUnknown *variables[1]; /* stretchy */
} SpkObjectSubclass;

typedef struct SpkVariableObject {
    SpkObject base;
    size_t size;
} SpkVariableObject;

typedef struct SpkVariableObjectSubclass {
    SpkVariableObject base;
    SpkUnknown *variables[1]; /* stretchy */
} SpkVariableObjectSubclass;


SpkObject *Spk_Cast(struct SpkBehavior *, SpkUnknown *);

SpkObject *SpkObject_New(struct SpkBehavior *);
SpkObject *SpkObject_NewVar(struct SpkBehavior *, size_t);


extern struct SpkClassTmpl Spk_ClassObjectTmpl, Spk_ClassVariableObjectTmpl;


#define SpkVariableObject_ITEM_BASE(op) (((char *)op) + ((SpkVariableObject *)op)->base.klass->instanceSize)

#define Spk_CAST(c, op) ((Spk ## c *)Spk_Cast(Spk_CLASS(c), (SpkUnknown *)(op)))


#endif /* __spk_obj_h__ */
