
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

static SpkUnknown *Metaclass_new(SpkUnknown *_self, SpkUnknown *name, SpkUnknown *arg1) {
    /* Answer a new class. */
    SpkMetaclass *self;
    SpkClass *newClass;
    
    self = (SpkMetaclass *)_self;
    if (!SpkHost_IsSymbol(name)) {
        Spk_Halt(Spk_HALT_TYPE_ERROR, "a symbol is required");
        return 0;
    }
    newClass = (SpkClass *)SpkObject_New(Spk_CLASS(Class));
    if (!newClass)
        return 0;
    Spk_INCREF(name);
    newClass->name = name;
    return (SpkUnknown *)newClass;
}


/*------------------------------------------------------------------------*/
/* class template */

typedef struct SpkMetaclassSubclass {
    SpkMetaclass base;
    SpkUnknown *variables[1]; /* stretchy */
} SpkMetaclassSubclass;

static SpkMethodTmpl methods[] = {
    { "new",   SpkNativeCode_METH_ATTR | SpkNativeCode_ARGS_0, &Metaclass_new   },
    { 0, 0, 0}
};

SpkClassTmpl Spk_ClassMetaclassTmpl = {
    Spk_HEART_CLASS_TMPL(Metaclass, Behavior), {
        /*accessors*/ 0,
        methods,
        /*lvalueMethods*/ 0,
        offsetof(SpkMetaclassSubclass, variables)
    }, /*meta*/ {
    }
};


/*------------------------------------------------------------------------*/
/* C API */

SpkMetaclass *SpkMetaclass_New(SpkMetaclass *superMeta, size_t instVarCount)
{
    SpkMetaclass *newMetaclass;
    
    newMetaclass = (SpkMetaclass *)SpkObject_New(Spk_CLASS(Metaclass));
    SpkBehavior_Init((SpkBehavior *)newMetaclass, (SpkBehavior *)superMeta,
                     0, instVarCount);
    newMetaclass->thisClass = 0;
    return newMetaclass;
}

SpkMetaclass *SpkMetaclass_FromTemplate(SpkBehaviorTmpl *template,
                                        SpkMetaclass *superMeta,
                                        struct SpkModule *module)
{
    SpkMetaclass *newMetaclass;
    
    newMetaclass = (SpkMetaclass *)SpkObject_New(Spk_CLASS(Metaclass));
    SpkBehavior_InitFromTemplate((SpkBehavior *)newMetaclass, template, (SpkBehavior *)superMeta, module);
    SpkBehavior_AddMethodsFromTemplate((SpkBehavior *)newMetaclass, template);
    newMetaclass->thisClass = 0;
    return newMetaclass;
}
