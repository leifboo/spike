
#include "class.h"

#include "array.h"
#include "int.h"
#include "interp.h"
#include "metaclass.h"
#include "sym.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


struct Behavior *Spk_ClassClass;


/*------------------------------------------------------------------------*/
/* methods */

static Object *Class_new(Object *_self, Object *arg0, Object *arg1) {
    /* Answer a new instance of the receiver. */
    Class *self;
    Behavior *poly;
    Array *args;
    ObjectSubclass *newObject;
    Object *nItemsObj; Integer *nItemsInt; long nItems;
    size_t i, itemArraySize;
    
    self = (Class *)_self;
    assert(args = Spk_CAST(Array, arg0)); /* XXX */
    
    /* Add-hoc class polymorphism, in lieu of metaclasses. */
    for (poly = (Behavior *)self;
         poly != Spk_ClassObject && poly != Spk_ClassVariableObject;
         poly = poly->superclass)
        ;
    if (poly == Spk_ClassObject) {
        assert(SpkArray_size(args) == 0 && "wrong number of arguments to 'new'");
        nItems = 0;
    } else if (poly == Spk_ClassVariableObject) {
        assert(SpkArray_size(args) == 1 && "wrong number of arguments to 'new'");
        assert(self->base.itemSize > 0);
        nItemsObj = SpkArray_item(args, 0);
        assert(nItemsInt = Spk_CAST(Integer, nItemsObj));
        nItems = SpkInteger_asLong(nItemsInt);
        assert(nItems >= 0); /* XXX */
    } else {
        assert(0 && "not reached");
    }
    itemArraySize = (size_t)nItems * self->base.itemSize;
    
    newObject = (ObjectSubclass *)malloc(self->base.instanceSize + itemArraySize);
    newObject->base.refCount = 1;
    newObject->base.klass = (Behavior *)self;
    for (i = 0; i < self->base.instVarCount; ++i) {
        newObject->variables[i] = Spk_uninit;
    }
    if (poly == Spk_ClassVariableObject) {
        ((VariableObject *)newObject)->size = (size_t)nItems;
        memset(SpkVariableObject_ITEM_BASE(newObject), 0, itemArraySize);
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

static SpkAccessorTmpl accessors[] = {
    { "name", Spk_T_OBJECT, offsetof(Class, name), SpkAccessor_READ },
    { 0, 0, 0, 0 }
};

static SpkMethodTmpl methods[] = {
    { "new", SpkNativeCode_METH_ATTR | SpkNativeCode_ARGS_ARRAY, &Class_new },
    { "print", SpkNativeCode_METH_ATTR | SpkNativeCode_ARGS_0, &Class_print },
    { 0, 0, 0}
};

SpkClassTmpl Spk_ClassClassTmpl = {
    "Class",
    offsetof(ClassSubclass, variables),
    sizeof(Class),
    0,
    accessors,
    methods
};


/*------------------------------------------------------------------------*/
/* C API */

Class *SpkClass_new(Symbol *name) {
    Class *newClass;
    
    newClass = (Class *)malloc(Spk_ClassClass->instanceSize);
    newClass->base.base.klass = Spk_ClassClass;
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
    self->name = SpkSymbol_get(template->name);
}

void SpkClass_addMethodsFromTemplate(Class *self, SpkClassTmpl *template) {
    SpkBehavior_addMethodsFromTemplate((Behavior *)self, template);
}
