
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


extern struct Behavior *ClassObject, *ClassVariableObject;
extern struct SpkClassTmpl ClassObjectTmpl, ClassVariableObjectTmpl;


#define SpkVariableObject_ITEM_BASE(op) (((char *)op) + ((VariableObject *)op)->base.klass->instanceSize)


#endif /* __obj_h__ */
