
#ifndef __obj_h__
#define __obj_h__


#include <stddef.h>


typedef struct Object {
    size_t refCount;
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


Object *Spk_alloc(size_t);
void Spk_dealloc(Object *);


extern struct Behavior *ClassObject, *ClassVariableObject;
extern struct SpkClassTmpl ClassObjectTmpl, ClassVariableObjectTmpl;


#define Spk_INCREF(op) (((Object *)(op))->refCount++)
#define Spk_DECREF(op) if (--(op)->refCount > 0) ; else Spk_dealloc((Object *)(op))
#define Spk_XINCREF(op) if (!(op)) ; else Spk_INCREF(op)
#define Spk_XDECREF(op) if (!(op)) ; else Spk_DECREF(op)

#define Spk_CLEAR(op) \
    do {                                        \
        if (op) {                               \
            Object *tmp = (Object *)(op);       \
            (op) = NULL;                        \
            Spk_DECREF(tmp);                    \
        }                                       \
    } while (0)


#define SpkVariableObject_ITEM_BASE(op) (((char *)op) + ((VariableObject *)op)->base.klass->instanceSize)


#endif /* __obj_h__ */
