
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

static Unknown *Class_printString(Unknown *self, Unknown *arg0, Unknown *arg1) {
    static const char *format = "<class %s>";
    const char *name;
    String *result;
    size_t len;
    char *str;
    
    name = Behavior_NameAsCString((Behavior *)self);
    len = strlen(format) + strlen(name);
    result = String_FromCStringAndLength(0, len);
    if (!result)
        return 0;
    str = String_AsCString(result);
    sprintf(str, format, name);
    result->size = strlen(str) + 1;
    return (Unknown *)result;
}

static Unknown *Class_subclass(Unknown *_self, Unknown *args, Unknown *arg1) {
    Behavior *self;
    Unknown *name, *instVarCount, *classVarCount;
    
    self = (Behavior *)_self;
    
    if (!IsArgs(args)) {
        Halt(HALT_TYPE_ERROR, "argument list expected");
        goto unwind;
    }
    if (ArgsSize(args) != 3) {
        Halt(HALT_TYPE_ERROR, "wrong number of arguments");
        goto unwind;
    }
    
    /* subclass:instVarCount:classVarCount: */
    name = GetArg(args, 0);
    instVarCount = GetArg(args, 1);
    classVarCount = GetArg(args, 2);
    
    if (!Host_IsSymbol(name)) {
        Halt(HALT_TYPE_ERROR, "a symbol is required");
        goto unwind;
    }
    if (!Host_IsInteger(instVarCount) ||
        !Host_IsInteger(classVarCount)) {
        Halt(HALT_TYPE_ERROR, "an integer is required");
        goto unwind;
    }
    
    /* XXX: module? */
    return (Unknown *)Class_New(
        name, self,
        (size_t)Host_IntegerAsCLong(instVarCount),
        (size_t)Host_IntegerAsCLong(classVarCount)
        );

 unwind:
    return 0;
}


/*------------------------------------------------------------------------*/
/* class tmpl */

typedef struct ClassSubclass {
    Class base;
    Unknown *variables[1]; /* stretchy */
} ClassSubclass;

static AccessorTmpl accessors[] = {
    { "name", T_OBJECT, offsetof(Class, name), Accessor_READ },
    { 0 }
};

static MethodTmpl methods[] = {
    { "printString", NativeCode_ARGS_0, &Class_printString },
    { "subclass:instVarCount:classVarCount:",
      NativeCode_ARGS_ARRAY, &Class_subclass },
    { 0 }
};

ClassTmpl ClassClassTmpl = {
    HEART_CLASS_TMPL(Class, Behavior), {
        accessors,
        methods,
        /*lvalueMethods*/ 0,
        offsetof(ClassSubclass, variables)
    }, /*meta*/ {
        0
    }
};


/*------------------------------------------------------------------------*/
/* C API */

Class *Class_New(Unknown *name, Behavior *superclass,
                       size_t instVarCount, size_t classVarCount)
{
    Metaclass *newMetaclass, *superMeta;
    Class *newClass;
    
    superMeta = CAST(Metaclass, superclass->base.klass);
    assert(superMeta);
    newMetaclass = Metaclass_New(superMeta, classVarCount);
    
    newClass = (Class *)Object_New((Behavior *)newMetaclass);
    Behavior_Init((Behavior *)newClass, superclass, 0, instVarCount);
    INCREF(name);
    newClass->name = name;
    
    assert(!newMetaclass->thisClass);
    newMetaclass->thisClass = newClass;  INCWREF(newClass);
    
    DECREF(newMetaclass);
    
    return newClass;
}

Class *Class_EmptyFromTemplate(ClassTmpl *tmpl,
                                     Behavior *superclass,
                                     Behavior *metaclass,
                                     struct Module *module)
{
    Metaclass *newMetaclass, *superMeta;
    Class *newClass;
    
    /* It is too early to call Metaclass_FromTemplate(). */
    superMeta = (Metaclass *)superclass->base.klass;
    newMetaclass = (Metaclass *)Object_New(metaclass);
    Behavior_InitFromTemplate((Behavior *)newMetaclass, &tmpl->metaclass, (Behavior *)superMeta, module);
    
    newClass = (Class *)Object_New((Behavior *)newMetaclass);
    Class_InitFromTemplate(newClass, tmpl, superclass, module);
    
    newMetaclass->thisClass = newClass;  INCWREF(newClass);
    
    DECREF(newMetaclass);
    
    return newClass;
}

Class *Class_FromTemplate(ClassTmpl *tmpl,
                                Behavior *superclass,
                                struct Module *module)
{
    Metaclass *newMetaclass, *superMeta;
    Class *newClass;
    
    superMeta = CAST(Metaclass, superclass->base.klass);
    assert(superMeta);
    newMetaclass = Metaclass_FromTemplate(&tmpl->metaclass, superMeta, module);
    
    newClass = (Class *)Object_New((Behavior *)newMetaclass);
    Class_InitFromTemplate(newClass, tmpl, superclass, module);
    
    assert(!newMetaclass->thisClass);
    newMetaclass->thisClass = newClass;  INCWREF(newClass);
    
    Behavior_AddMethodsFromTemplate((Behavior *)newClass, &tmpl->thisClass);
    
    DECREF(newMetaclass);
    
    return newClass;
}

void Class_InitFromTemplate(Class *self,
                               ClassTmpl *tmpl,
                               Behavior *superclass,
                               struct Module *module)
{
    Behavior_InitFromTemplate((Behavior *)self,
                                 &tmpl->thisClass,
                                 superclass,
                                 module);
    self->name = Host_SymbolFromCString(tmpl->name);
}

Unknown *Class_Name(Class *self) {
    INCREF(self->name);
    return self->name;
}
