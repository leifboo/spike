
#include "metaclass.h"

#include "class.h"
#include "interp.h"
#include "sym.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>


Metaclass *ClassMetaclass;


/*------------------------------------------------------------------------*/
/* methods */

static Object *Metaclass_new(Object *_self, Object *_name, Object *arg1) {
    /* Answer a new class. */
    Metaclass *self; Symbol *name;
    Class *newClass;
    size_t i;
    
    self = (Metaclass *)_self;
    assert(_name->klass == ClassSymbol);
    name = (Symbol *)_name;
    newClass = (Class *)malloc(ClassClass->base.instanceSize);
    newClass->base.base.refCount = 1;
    newClass->base.base.klass = (Behavior *)self;
    newClass->name = name;
    return (Object *)newClass;
}

static Object *Metaclass_print(Object *self, Object *arg0, Object *arg1) {
    printf("<Metaclass object at %p>", self);
    return Spk_void;
}


/*------------------------------------------------------------------------*/
/* class template */

static SpkMethodTmpl methods[] = {
    { "new",   SpkNativeCode_ARGS_0 | SpkNativeCode_CALLABLE, &Metaclass_new   },
    { "print", SpkNativeCode_ARGS_0 | SpkNativeCode_CALLABLE, &Metaclass_print },
    { 0, 0, 0}
};

SpkClassTmpl ClassMetaclassTmpl = {
    "Metaclass",
    offsetof(MetaclassSubclass, variables),
    sizeof(Metaclass),
    0,
    0,
    methods
};
