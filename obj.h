
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


void SpkClassObject_init(void);
void SpkClassObject_init2(void);


#endif /* __obj_h__ */
