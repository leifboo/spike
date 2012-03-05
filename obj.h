
#ifndef __obj_h__
#define __obj_h__


#include "om.h"
#include <stddef.h>


typedef struct Object {
    Unknown base;
    struct Behavior *klass;
} Object;

typedef struct ObjectSubclass {
    Object base;
    Unknown *variables[1]; /* stretchy */
} ObjectSubclass;

typedef struct VariableObject {
    Object base;
    size_t size;
} VariableObject;

typedef struct VariableObjectSubclass {
    VariableObject base;
    Unknown *variables[1]; /* stretchy */
} VariableObjectSubclass;


Object *Cast(struct Behavior *, Unknown *);

Object *Object_New(struct Behavior *);
Object *Object_NewVar(struct Behavior *, size_t);

Unknown *ObjectAsString(Unknown *);
void PrintObject(Unknown *, /*FILE*/ void *);


extern struct ClassTmpl ClassObjectTmpl, ClassVariableObjectTmpl;


#define VariableObject_ITEM_BASE(op) (((char *)op) + ((VariableObject *)op)->base.klass->instanceSize)

#define CAST(c, op) ((c *)Cast(CLASS(c), (Unknown *)(op)))


#endif /* __obj_h__ */
