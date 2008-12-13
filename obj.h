
#ifndef __obj_h__
#define __obj_h__


#include <stddef.h>


typedef struct Object {
    int refCount;
    struct Behavior *klass;
} Object;

typedef struct ObjectSubclass {
    Object base;
    Object *variables[1]; /* stretchy */
} ObjectSubclass;

typedef struct VariableObject {
    Object base;
    size_t size;
} VariableObject;

typedef struct VariableObjectSubclass {
    VariableObject base;
    Object *variables[1]; /* stretchy */
} VariableObjectSubclass;


extern struct Behavior *ClassObject;
extern struct SpkClassTmpl ClassObjectTmpl;


#define SpkObject_ITEM_BASE(op) (((char *)op) + ((Object *)op)->klass->instanceSize)


#endif /* __obj_h__ */
