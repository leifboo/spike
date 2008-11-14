
#include "class.h"

#include "interp.h"
#include "metaclass.h"
#include <stdio.h>
#include <stdlib.h>


struct Metaclass *ClassClass;


/*------------------------------------------------------------------------*/
/* methods */

static Object *Class_new(Object *_self, Object *arg0, Object *arg1) {
    /* Answer a new instance of the receiver. */
    Class *self;
    ObjectSubclass *newObject;
    size_t i;
    
    self = (Class *)_self;
    newObject = (ObjectSubclass *)malloc(self->base.instanceSize);
    newObject->base.refCount = 1;
    newObject->base.klass = (Behavior *)self;
    for (i = 0; i < self->base.instVarCount; ++i) {
        newObject->variables[i] = Spk_uninit;
    }
    return (Object *)newObject;
}

static Object *Class_print(Object *_self, Object *arg0, Object *arg1) {
    Class *self;
    
    self = (Class *)_self;
    printf("<class %s>", self->name->str);
    return Spk_void;
}


/*------------------------------------------------------------------------*/
/* class template */

static SpkMethodTmpl methods[] = {
    { "new",   SpkNativeCode_ARGS_0 | SpkNativeCode_CALLABLE, &Class_new   },
    { "print", SpkNativeCode_ARGS_0 | SpkNativeCode_CALLABLE, &Class_print },
    { 0, 0, 0}
};

SpkClassTmpl ClassClassTmpl = {
    offsetof(ClassSubclass, variables),
    sizeof(Class),
    0,
    methods
};


/*------------------------------------------------------------------------*/
/* C API */

Class *SpkClass_new(Symbol *name) {
    Class *newClass;
    
    newClass = (Class *)malloc(ClassClass->base.instanceSize);
    newClass->base.base.klass = (Behavior *)ClassClass;
    newClass->name = name;
    return newClass;
}

void SpkClass_initFromTemplate(Class *self,
                               SpkClassTmpl *template,
                               Behavior *superclass,
                               struct Module *module)
{
    SpkBehavior_initFromTemplate((Behavior *)self,
                                 template,
                                 superclass,
                                 module);
    self->name = SpkSymbol_get("Foo" /*template->name*/ );
}
