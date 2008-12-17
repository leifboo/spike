
#include "metaclass.h"

#include "class.h"
#include "interp.h"
#include "sym.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>


Metaclass *Spk_ClassMetaclass;


/*------------------------------------------------------------------------*/
/* methods */

static Object *Metaclass_new(Object *_self, Object *_name, Object *arg1) {
    /* Answer a new class. */
    Metaclass *self; Symbol *name;
    Class *newClass;
    size_t i;
    
    self = (Metaclass *)_self;
    assert(name = Spk_CAST(Symbol, _name)); /* XXX */
    newClass = (Class *)malloc(Spk_ClassClass->instanceSize);
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
    { "new",   SpkNativeCode_METH_ATTR | SpkNativeCode_ARGS_0, &Metaclass_new   },
    { "print", SpkNativeCode_METH_ATTR | SpkNativeCode_ARGS_0, &Metaclass_print },
    { 0, 0, 0}
};

SpkClassTmpl Spk_ClassMetaclassTmpl = {
    "Metaclass",
    offsetof(MetaclassSubclass, variables),
    sizeof(Metaclass),
    0,
    0,
    methods
};
