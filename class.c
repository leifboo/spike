
#include "class.h"

#include "host.h"
#include "interp.h"
#include "metaclass.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


struct SpkBehavior *Spk_ClassClass;


/*------------------------------------------------------------------------*/
/* methods */

static SpkUnknown *Class_printString(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    static const char *format = "<class %s>";
    const char *name;
    SpkUnknown *result;
    size_t len;
    
    name = SpkBehavior_NameAsCString((SpkBehavior *)self);
    len = strlen(format) + strlen(name);
    result = SpkHost_StringFromCStringAndLength(0, len);
    if (!result)
        return 0;
    sprintf(SpkHost_StringAsCString(result), format, name);
    return result;
}


/*------------------------------------------------------------------------*/
/* class template */

typedef struct SpkClassSubclass {
    SpkClass base;
    SpkUnknown *variables[1]; /* stretchy */
} SpkClassSubclass;

static SpkAccessorTmpl accessors[] = {
    { "name", Spk_T_OBJECT, offsetof(SpkClass, name), SpkAccessor_READ },
    { 0, 0, 0, 0 }
};

static SpkMethodTmpl methods[] = {
    { "printString", SpkNativeCode_ARGS_0, &Class_printString },
    { 0, 0, 0 }
};

SpkClassTmpl Spk_ClassClassTmpl = {
    "Class", {
        accessors,
        methods,
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
    self->name = SpkHost_SymbolFromCString(template->name);
}

SpkUnknown *SpkClass_Name(SpkClass *self) {
    Spk_INCREF(self->name);
    return self->name;
}
