
#include "class.h"

#include "heart.h"
#include "host.h"
#include "interp.h"
#include "metaclass.h"
#include "str.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*------------------------------------------------------------------------*/
/* methods */

static SpkUnknown *Class_printString(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    static const char *format = "<class %s>";
    const char *name;
    SpkString *result;
    size_t len;
    char *str;
    
    name = SpkBehavior_NameAsCString((SpkBehavior *)self);
    len = strlen(format) + strlen(name);
    result = SpkString_FromCStringAndLength(0, len);
    if (!result)
        return 0;
    str = SpkString_AsCString(result);
    sprintf(str, format, name);
    result->size = strlen(str) + 1;
    return (SpkUnknown *)result;
}

static SpkUnknown *Class_subclass(SpkUnknown *_self, SpkUnknown *args, SpkUnknown *arg1) {
    SpkBehavior *self;
    SpkUnknown *name, *instVarCount, *classVarCount;
    
    self = (SpkBehavior *)_self;
    
    if (!Spk_IsArgs(args)) {
        Spk_Halt(Spk_HALT_TYPE_ERROR, "argument list expected");
        goto unwind;
    }
    if (Spk_ArgsSize(args) != 3) {
        Spk_Halt(Spk_HALT_TYPE_ERROR, "wrong number of arguments");
        goto unwind;
    }
    
    /* subclass:instVarCount:classVarCount: */
    name = Spk_GetArg(args, 0);
    instVarCount = Spk_GetArg(args, 1);
    classVarCount = Spk_GetArg(args, 2);
    
    if (!SpkHost_IsSymbol(name)) {
        Spk_Halt(Spk_HALT_TYPE_ERROR, "a symbol is required");
        goto unwind;
    }
    if (!SpkHost_IsInteger(instVarCount) ||
        !SpkHost_IsInteger(classVarCount)) {
        Spk_Halt(Spk_HALT_TYPE_ERROR, "an integer is required");
        goto unwind;
    }
    
    /* XXX: module? */
    return (SpkUnknown *)SpkClass_New(
        name, self,
        (size_t)SpkHost_IntegerAsCLong(instVarCount),
        (size_t)SpkHost_IntegerAsCLong(classVarCount)
        );

 unwind:
    return 0;
}


/*------------------------------------------------------------------------*/
/* class tmpl */

typedef struct SpkClassSubclass {
    SpkClass base;
    SpkUnknown *variables[1]; /* stretchy */
} SpkClassSubclass;

static SpkAccessorTmpl accessors[] = {
    { "name", Spk_T_OBJECT, offsetof(SpkClass, name), SpkAccessor_READ },
    { 0 }
};

static SpkMethodTmpl methods[] = {
    { "printString", SpkNativeCode_ARGS_0, &Class_printString },
    { "subclass:instVarCount:classVarCount:",
      SpkNativeCode_ARGS_ARRAY, &Class_subclass },
    { 0 }
};

SpkClassTmpl Spk_ClassClassTmpl = {
    Spk_HEART_CLASS_TMPL(Class, Behavior), {
        accessors,
        methods,
        /*lvalueMethods*/ 0,
        offsetof(SpkClassSubclass, variables)
    }, /*meta*/ {
        0
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
    newMetaclass->thisClass = newClass;  Spk_INCWREF(newClass);
    
    Spk_DECREF(newMetaclass);
    
    return newClass;
}

SpkClass *SpkClass_EmptyFromTemplate(SpkClassTmpl *tmpl,
                                     SpkBehavior *superclass,
                                     SpkBehavior *Metaclass,
                                     struct SpkModule *module)
{
    SpkMetaclass *newMetaclass, *superMeta;
    SpkClass *newClass;
    
    /* It is too early to call SpkMetaclass_FromTemplate(). */
    superMeta = (SpkMetaclass *)superclass->base.klass;
    newMetaclass = (SpkMetaclass *)SpkObject_New(Metaclass);
    SpkBehavior_InitFromTemplate((SpkBehavior *)newMetaclass, &tmpl->metaclass, (SpkBehavior *)superMeta, module);
    
    newClass = (SpkClass *)SpkObject_New((SpkBehavior *)newMetaclass);
    SpkClass_InitFromTemplate(newClass, tmpl, superclass, module);
    
    newMetaclass->thisClass = newClass;  Spk_INCWREF(newClass);
    
    Spk_DECREF(newMetaclass);
    
    return newClass;
}

SpkClass *SpkClass_FromTemplate(SpkClassTmpl *tmpl,
                                SpkBehavior *superclass,
                                struct SpkModule *module)
{
    SpkMetaclass *newMetaclass, *superMeta;
    SpkClass *newClass;
    
    superMeta = Spk_CAST(Metaclass, superclass->base.klass);
    assert(superMeta);
    newMetaclass = SpkMetaclass_FromTemplate(&tmpl->metaclass, superMeta, module);
    
    newClass = (SpkClass *)SpkObject_New((SpkBehavior *)newMetaclass);
    SpkClass_InitFromTemplate(newClass, tmpl, superclass, module);
    
    assert(!newMetaclass->thisClass);
    newMetaclass->thisClass = newClass;  Spk_INCWREF(newClass);
    
    SpkBehavior_AddMethodsFromTemplate((SpkBehavior *)newClass, &tmpl->thisClass);
    
    Spk_DECREF(newMetaclass);
    
    return newClass;
}

void SpkClass_InitFromTemplate(SpkClass *self,
                               SpkClassTmpl *tmpl,
                               SpkBehavior *superclass,
                               struct SpkModule *module)
{
    SpkBehavior_InitFromTemplate((SpkBehavior *)self,
                                 &tmpl->thisClass,
                                 superclass,
                                 module);
    self->name = SpkHost_SymbolFromCString(tmpl->name);
}

SpkUnknown *SpkClass_Name(SpkClass *self) {
    Spk_INCREF(self->name);
    return self->name;
}
