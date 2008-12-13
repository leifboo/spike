
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


struct Metaclass *ClassClass;


/*------------------------------------------------------------------------*/
/* methods */

static Object *Class_name(Object *self, Object *arg0, Object *arg1) {
    return (Object *)((Class *)self)->name;
}

#if 0 /* XXX: the calling convention doesn't support this yet */
static Object *Class_new(Object *_self, Object *arg0, Object *arg1) {
    /* Answer a new instance of the receiver. */
    Class *self;
    Array *args;
    ObjectSubclass *newObject;
    Object *nItemsObj; long nItems;
    size_t i, itemArraySize;
    
    self = (Class *)_self;
    assert(arg0->klass == ClassArray); /* XXX */
    args = (Array *)arg0;
    
    switch (SpkArray_size(args)) {
    case 0:
        assert(self->base.itemSize == 0 && "object size expected");
        nItems = 0;
        break;
    case 1:
        assert(self->base.itemSize > 0 && "variable size class expected");
        nItemsObj = SpkArray_item(args, 0);
        assert(nItemsObj->klass == ClassInteger);
        nItems = SpkInteger_asLong((Integer *)nItemsObj);
        assert(nItems >= 0); /* XXX */
        break;
    default:
        assert(0 && "wrong number of arguments"); /* XXX */
    }
    itemArraySize = (size_t)nItems * self->base.itemSize;
    
    newObject = (ObjectSubclass *)malloc(self->base.instanceSize + itemArraySize);
    newObject->base.refCount = 1;
    newObject->base.klass = (Behavior *)self;
    for (i = 0; i < self->base.instVarCount; ++i) {
        newObject->variables[i] = Spk_uninit;
    }
    ((VariableObject *)newObject)->size = (size_t)nItems;
    memset(SpkObject_ITEM_BASE(newObject), 0, itemArraySize);
    return (Object *)newObject;
}
#else
static Object *Class_new(Object *_self, Object *arg0, Object *arg1) {
    /* Answer a new instance of the receiver. */
    Class *self;
    ObjectSubclass *newObject;
    size_t i;

    self = (Class *)_self;
    assert(self->base.itemSize == 0 && "call new_ instead of new");
    newObject = (ObjectSubclass *)malloc(self->base.instanceSize);
    newObject->base.refCount = 1;
    newObject->base.klass = (Behavior *)self;
    for (i = 0; i < self->base.instVarCount; ++i) {
        newObject->variables[i] = Spk_uninit;
    }
    return (Object *)newObject;
}

static Object *Class_new_(Object *_self, Object *arg0, Object *arg1) {
    /* Answer a new instance of the receiver. */
    Class *self;
    VariableObjectSubclass *newObject;
    long nItems;
    size_t i, itemArraySize;
    
    self = (Class *)_self;
    assert(self->base.itemSize > 0 && "variable size class expected");
    assert(arg0->klass == ClassInteger);
    nItems = SpkInteger_asLong((Integer *)arg0);
    assert(nItems >= 0); /* XXX */
    itemArraySize = (size_t)nItems * self->base.itemSize;
    
    newObject = (VariableObjectSubclass *)malloc(self->base.instanceSize + itemArraySize);
    newObject->base.base.refCount = 1;
    newObject->base.base.klass = (Behavior *)self;
    for (i = 0; i < self->base.instVarCount; ++i) {
        newObject->variables[i] = Spk_uninit;
    }
    newObject->base.size = (size_t)nItems;
    memset(SpkObject_ITEM_BASE((VariableObject *)newObject), 0, itemArraySize);
    return (Object *)newObject;
}
#endif

static Object *Class_print(Object *_self, Object *arg0, Object *arg1) {
    Class *self;
    
    self = (Class *)_self;
    printf("<class %s>", self->name->str);
    return Spk_void;
}


/*------------------------------------------------------------------------*/
/* class template */

static SpkMethodTmpl methods[] = {
    { "name", SpkNativeCode_LEAF, &Class_name },
    { "new", SpkNativeCode_METH_ATTR | SpkNativeCode_ARGS_0, &Class_new },
    { "new_", SpkNativeCode_METH_ATTR | SpkNativeCode_ARGS_1, &Class_new_ },
    { "print", SpkNativeCode_METH_ATTR | SpkNativeCode_ARGS_0, &Class_print },
    { 0, 0, 0}
};

SpkClassTmpl ClassClassTmpl = {
    "Class",
    offsetof(ClassSubclass, variables),
    sizeof(Class),
    0,
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
    self->name = SpkSymbol_get(template->name);
}

void SpkClass_addMethodsFromTemplate(Class *self, SpkClassTmpl *template) {
    SpkBehavior_addMethodsFromTemplate((Behavior *)self, template);
}
