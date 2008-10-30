
#ifndef __obj_h__
#define __obj_h__


typedef struct Object {
    int refCount;
    struct Behavior *klass;
} Object;

typedef struct ObjectSubclass {
    Object base;
    Object *variables[1]; /* stretchy */
} ObjectSubclass;


extern struct Behavior *ClassObject;
extern struct SpkClassTmpl ClassObjectTmpl;


#endif /* __obj_h__ */
