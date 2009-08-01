
#include "class.h"

#include "host.h"
#include "interp.h"
#include "metaclass.h"
#include <assert.h>
#include <stdlib.h>


struct SpkBehavior *Spk_ClassClass;


/*------------------------------------------------------------------------*/
/* class template */

static SpkAccessorTmpl accessors[] = {
    { "name", Spk_T_OBJECT, offsetof(SpkClass, name), SpkAccessor_READ },
    { 0, 0, 0, 0 }
};

SpkClassTmpl Spk_ClassClassTmpl = {
    "Class", {
        accessors,
        /*methods*/ 0,
        /*lvalueMethods*/ 0,
        offsetof(SpkClassSubclass, variables)
    }, /*meta*/ {
    }
};


/*------------------------------------------------------------------------*/
/* C API */

SpkClass *SpkClass_New(SpkUnknown *name, SpkBehavior *superclass,
                       size_t instVarCount, size_t classVarCount)
{
    SpkMetaclass *newMetaclass, *superMeta;
    SpkClass *newClass;
    
    superMeta = Spk_CAST(Metaclass, superclass->base.klass);
    assert(superMeta);
    newMetaclass = SpkMetaclass_New(superMeta, classVarCount);
    
    newClass = (SpkClass *)SpkObject_New((SpkBehavior *)newMetaclass);
    SpkBehavior_Init((SpkBehavior *)newClass, superclass, 0, instVarCount);
    Spk_INCREF(name);
    newClass->name = name;
    
    assert(!newMetaclass->thisClass);
    newMetaclass->thisClass = newClass;  Spk_INCREF(newClass);
    
    Spk_DECREF(newMetaclass);
    
    return newClass;
}

SpkClass *SpkClass_EmptyFromTemplate(SpkClassTmpl *template,
                                     SpkBehavior *superclass,
                                     struct SpkModule *module)
{
    SpkMetaclass *newMetaclass, *superMeta;
    SpkClass *newClass;
    
    /* It is too early to call SpkMetaclass_FromTemplate(). */
    superMeta = Spk_CAST(Metaclass, superclass->base.klass);
    assert(superMeta);
    newMetaclass = (SpkMetaclass *)SpkObject_New(Spk_ClassMetaclass);
    SpkBehavior_InitFromTemplate((SpkBehavior *)newMetaclass, &template->metaclass, (SpkBehavior *)superMeta, module);
    
    newClass = (SpkClass *)SpkObject_New((SpkBehavior *)newMetaclass);
    SpkClass_InitFromTemplate(newClass, template, superclass, module);
    
    newMetaclass->thisClass = newClass;  Spk_INCREF(newClass);
    
    Spk_DECREF(newMetaclass);
    
    return newClass;
}

SpkClass *SpkClass_FromTemplate(SpkClassTmpl *template,
                                SpkBehavior *superclass,
                                struct SpkModule *module)
{
    SpkMetaclass *newMetaclass, *superMeta;
    SpkClass *newClass;
    
    superMeta = Spk_CAST(Metaclass, superclass->base.klass);
    assert(superMeta);
    newMetaclass = SpkMetaclass_FromTemplate(&template->metaclass, superMeta, module);
    
    newClass = (SpkClass *)SpkObject_New((SpkBehavior *)newMetaclass);
    SpkClass_InitFromTemplate(newClass, template, superclass, module);
    
    assert(!newMetaclass->thisClass);
    newMetaclass->thisClass = newClass;  Spk_INCREF(newClass);
    
    SpkBehavior_AddMethodsFromTemplate((SpkBehavior *)newClass, &template->thisClass);
    
    Spk_DECREF(newMetaclass);
    
    return newClass;
}

void SpkClass_InitFromTemplate(SpkClass *self,
                               SpkClassTmpl *template,
                               SpkBehavior *superclass,
                               struct SpkModule *module)
{
    SpkBehavior_InitFromTemplate((SpkBehavior *)self,
                                 &template->thisClass,
                                 superclass,
                                 module);
    self->name = SpkHost_SymbolFromString(template->name);
}
