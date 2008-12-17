
#include "obj.h"

#include "behavior.h"
#include "bool.h"
#include "interp.h"
#include "native.h"
#include <stdio.h>
#include <stdlib.h>


Behavior *ClassObject, *ClassVariableObject;


/*------------------------------------------------------------------------*/
/* methods */

static Object *Object_eq(Object *self, Object *arg0, Object *arg1) {
    return self == arg0 ? Spk_true : Spk_false;
}

static Object *Object_ne(Object *self, Object *arg0, Object *arg1) {
    Object *temp;
    
    temp = Spk_oper(theInterpreter, self, OPER_EQ, arg0, 0);
    temp = Spk_oper(theInterpreter, temp, OPER_LNEG, 0);
    return temp;
}

static Object *Object_print(Object *self, Object *arg0, Object *arg1) {
    printf("<%s instance at %p>", SpkBehavior_name(self->klass), self);
    return Spk_void;
}


/*------------------------------------------------------------------------*/
/* memory layout of instances */

static void Object_traverse_init(Object *self) {
    self->refCount = 0;
}

static Object **Object_traverse_current(Object *self) {
    if (self->refCount >= self->klass->instVarCount)
        return 0;
    return (Object **)(self->klass->instVarOffset + self->refCount * sizeof(Object *));
}

static void Object_traverse_next(Object *self) {
    ++self->refCount;
}


/*------------------------------------------------------------------------*/
/* class templates */

static SpkAccessorTmpl ObjectAccessors[] = {
    { "class", Spk_T_OBJECT, offsetof(Object, klass), SpkAccessor_READ },
    { 0, 0, 0, 0 }
};

static SpkMethodTmpl ObjectMethods[] = {
    { "__eq__", SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &Object_eq },
    { "__ne__", SpkNativeCode_BINARY_OPER, &Object_ne },
    { "print", SpkNativeCode_METH_ATTR | SpkNativeCode_ARGS_0, &Object_print },
    { 0, 0, 0}
};

static traverse_t ObjectTraverse = {
    &Object_traverse_init,
    &Object_traverse_current,
    &Object_traverse_next,
};

SpkClassTmpl ClassObjectTmpl = {
    "Object",
    offsetof(ObjectSubclass, variables),
    sizeof(Object),
    0,
    ObjectAccessors,
    ObjectMethods,
    &ObjectTraverse
};


static SpkMethodTmpl VariableObjectMethods[] = {
    { 0, 0, 0}
};

SpkClassTmpl ClassVariableObjectTmpl = {
    "VariableObject",
    offsetof(VariableObjectSubclass, variables),
    sizeof(VariableObject),
    0,
    0,
    VariableObjectMethods
};


/*------------------------------------------------------------------------*/
/* object memory */

Object *Spk_alloc(size_t size) {
    Object *newObject;
    
    newObject = (Object *)malloc(size);
    newObject->refCount = 1;
    return newObject;
}

void Spk_dealloc(Object *obj) {
    /* cf. "A Space-efficient Reference-counting Collector", pp. 678-681 */
    Object *prior, *current, *klass;
    Object **var;
    
    prior = 0;
    current = obj;
 dealloc:
    (*current->klass->traverse.init)(current);
    while (1) {
        while ((var = (*current->klass->traverse.current)(current))) {
            if (--(*var)->refCount == 0) {
                Object *tmp = *var;
                *var = prior;
                prior = current;
                current = tmp;
                goto dealloc;
            }
            (*current->klass->traverse.next)(current);
        }
        klass = (Object *)current->klass;
        free(current);
        if (--klass->refCount == 0) {
            current = klass;
            goto dealloc;
        }
        current = prior;
        if (!current)
            break;
        /* resume where we left off */
        var = (*current->klass->traverse.current)(current);
        prior = *var;
        (*current->klass->traverse.next)(current);
    }
}
