
#include "metaclass.h"

#include "behavior.h"
#include "class.h"
#include "heart.h"
#include "host.h"
#include "interp.h"
#include <assert.h>
#include <stdlib.h>


/*------------------------------------------------------------------------*/
/* methods */

static Unknown *Metaclass_new(Unknown *_self, Unknown *name, Unknown *arg1) {
    /* Answer a new class. */
    Metaclass *self;
    Class *newClass;
    
    self = (Metaclass *)_self;
    if (!Host_IsSymbol(name)) {
        Halt(HALT_TYPE_ERROR, "a symbol is required");
        return 0;
    }
    newClass = (Class *)Object_New(CLASS(Class));
    if (!newClass)
        return 0;
    newClass->name = name;
    return (Unknown *)newClass;
}


/*------------------------------------------------------------------------*/
/* class template */

typedef struct MetaclassSubclass {
    Metaclass base;
    Unknown *variables[1]; /* stretchy */
} MetaclassSubclass;

static MethodTmpl methods[] = {
    { "new",   NativeCode_ARGS_0, &Metaclass_new   },
    { 0 }
};

ClassTmpl ClassMetaclassTmpl = {
    HEART_CLASS_TMPL(Metaclass, Behavior), {
        /*accessors*/ 0,
        methods,
        /*lvalueMethods*/ 0,
        offsetof(MetaclassSubclass, variables)
    }, /*meta*/ {
        0
    }
};


/*------------------------------------------------------------------------*/
/* C API */

Metaclass *Metaclass_New(Metaclass *superMeta, size_t instVarCount)
{
    Metaclass *newMetaclass;
    
    newMetaclass = (Metaclass *)Object_New(CLASS(Metaclass));
    Behavior_Init((Behavior *)newMetaclass, (Behavior *)superMeta,
                     0, instVarCount);
    newMetaclass->thisClass = 0;
    return newMetaclass;
}

Metaclass *Metaclass_FromTemplate(BehaviorTmpl *tmpl,
                                        Metaclass *superMeta,
                                        struct Module *module)
{
    Metaclass *newMetaclass;
    
    newMetaclass = (Metaclass *)Object_New(CLASS(Metaclass));
    Behavior_InitFromTemplate((Behavior *)newMetaclass, tmpl, (Behavior *)superMeta, module);
    Behavior_AddMethodsFromTemplate((Behavior *)newMetaclass, tmpl);
    newMetaclass->thisClass = 0;
    return newMetaclass;
}
