
#include "module.h"

#include "class.h"
#include "heart.h"
#include "rodata.h"

#include "char.h"
#include "float.h"
#include "int.h"
#include "str.h"
#include "sym.h"

#include <stdlib.h>
#include <string.h>


typedef struct Thunk Thunk;


static Thunk *Thunk_New(Unknown *, Unknown *);


/*------------------------------------------------------------------------*/
/* methods */

static Unknown *Module__init(Unknown *_self, Unknown *arg0, Unknown *arg1) {
    Module *self;
    ModuleClass *moduleClass;
    ModuleTmpl *tmpl;
    Unknown **globals;
    ClassTmpl *classTmpl;
    Unknown *tmp;
    
    self = (Module *)_self;
    moduleClass = (ModuleClass *)self->base.klass;
    tmpl = moduleClass->tmpl;
    
    /* XXX: Module inheritance will introduce multiple sets of globals. */
    globals = Module_VARIABLES(self);
    
    /* Initialize predefined variables. */
    tmp = Send(GLOBAL(theInterpreter), (Unknown *)self, _predef, 0);
    if (!tmp)
        return 0;
    DECREF(tmp);
    
    /* Create all classes. */
    for (classTmpl = tmpl->classList.first;
         classTmpl;
         classTmpl = classTmpl->next) {
        
        Unknown **classVar;
        Behavior *superclass;
        
        /* The class list is a preorder traversal of the inheritance graph;
           this guarantees that the superclass is created before its subclasses. */
        superclass = CAST(Behavior, globals[classTmpl->superclassVarIndex]);
        classVar = &globals[classTmpl->classVarIndex];
        *classVar = (Unknown *)Class_FromTemplate(classTmpl, superclass, self);
    }
    
    INCREF(GLOBAL(xvoid));
    return GLOBAL(xvoid);
}


static Unknown *Module__thunk(Unknown *_self, Unknown *arg0, Unknown *arg1) {
    return (Unknown *)Thunk_New(_self, arg0);
}


/*------------------------------------------------------------------------*/
/* low-level hooks */

static void Module_zero(Object *_self) {
    Module *self;
    
    self = (Module *)_self;
    (*CLASS(Module)->superclass->zero)(_self);
    /* XXX: This is the wrong, but necessary. */
    self->base.klass->module = self;
}

static void ModuleClass_zero(Object *_self) {
    ModuleClass *self;
    
    self = (ModuleClass *)_self;
    (*CLASS(Class)->zero)(_self);
    self->literalCount = 0;
    self->literals = 0;
    self->tmpl = 0;
}


/*------------------------------------------------------------------------*/
/* class tmpl */

typedef struct ModuleSubclass {
    Module base;
    Unknown *variables[1]; /* stretchy */
} ModuleSubclass;

typedef struct ModuleClassSubclass {
    ModuleClass base;
    Unknown *variables[1]; /* stretchy */
} ModuleClassSubclass;

static MethodTmpl methods[] = {
    { "_init", NativeCode_ARGS_0, &Module__init },
    { "_thunk", NativeCode_ARGS_1, &Module__thunk },
    { 0 }
};

ClassTmpl ClassModuleTmpl = {
    HEART_CLASS_TMPL(Module, Object), {
        /*accessors*/ 0,
        methods,
        /*lvalueMethods*/ 0,
        offsetof(ModuleSubclass, variables),
        /*itemSize*/ 0,
        &Module_zero
    }, /*meta*/ {
        /*accessors*/ 0,
        /*methods*/ 0,
        /*lvalueMethods*/ 0,
        offsetof(ModuleClassSubclass, variables),
        /*itemSize*/ 0,
        &ModuleClass_zero
    }
};


/*------------------------------------------------------------------------*/
/* C API */

ModuleClass *ModuleClass_New(ModuleTmpl *tmpl) {
    ModuleClass *moduleClass;
    
    moduleClass
        = (ModuleClass *)Class_FromTemplate(&tmpl->moduleClass,
                                                  CLASS(Module),
                                                  0);
    
    moduleClass->literalCount = tmpl->literalCount;
    moduleClass->literals = tmpl->literals;
    moduleClass->tmpl = tmpl;  INCREF(tmpl);
    return moduleClass;
}

void Module_InitLiteralsFromTemplate(Behavior *moduleClass, ModuleTmpl *tmpl) {
    ModuleClass *self;
    size_t i;
    Unknown *literal;
    
    self = (ModuleClass *)Cast(CLASS(Module)->base.klass,
                                      (Unknown *)moduleClass);
    if (!tmpl->literalCount)
        return;
    self->literals = (Unknown **)malloc(tmpl->literalCount * sizeof(Unknown *));
    self->literalCount = tmpl->literalCount;
    for (i = 0; i < self->literalCount; ++i) {
        switch (tmpl->literalTable[i].kind) {
        case LITERAL_SYMBOL:
            literal = (Unknown *)Symbol_FromCString(tmpl->literalTable[i].stringValue);
            break;
        case LITERAL_INTEGER:
            literal = (Unknown *)Integer_FromCLong(tmpl->literalTable[i].intValue);
            break;
        case LITERAL_FLOAT:
            literal = (Unknown *)Float_FromCDouble(tmpl->literalTable[i].floatValue);
            break;
        case LITERAL_CHAR:
            literal = (Unknown *)Char_FromCChar(tmpl->literalTable[i].intValue);
            break;
        case LITERAL_STRING:
            literal = (Unknown *)String_FromCString(tmpl->literalTable[i].stringValue);
            break;
        }
        self->literals[i] = literal;
    }
}


/*------------------------------------------------------------------------*/
/* thunks */

struct Thunk {
    Object base;
    Unknown *receiver;
    Unknown *selector;
};

typedef struct ThunkSubclass {
    Thunk base;
    Unknown *variables[1]; /* stretchy */
} ThunkSubclass;

static Unknown *Thunk_apply(Unknown *_self, Unknown *arg0, Unknown *arg1) {
    Thunk *self = (Thunk *)_self;
    return SendWithArguments(GLOBAL(theInterpreter),
                                 self->receiver,
                                 self->selector,
                                 arg0);
}

static MethodTmpl ThunkMethods[] = {
    { "__apply__", NativeCode_ARGS_ARRAY, &Thunk_apply },
    { 0 }
};

static void Thunk_dealloc(Object *_self, Unknown **l) {
    Thunk *self = (Thunk *)_self;
    LDECREF(self->receiver, l);
    LDECREF(self->selector, l);
    (*CLASS(Thunk)->superclass->dealloc)(_self, l);
}

ClassTmpl ClassThunkTmpl = {
    HEART_CLASS_TMPL(Thunk, Object), {
        /*accessors*/ 0,
        ThunkMethods,
        /*lvalueMethods*/ 0,
        offsetof(ThunkSubclass, variables),
        /*itemSize*/ 0,
        /*zero*/ 0,
        &Thunk_dealloc
    }, /*meta*/ {
        0
    }
};

static Thunk *Thunk_New(
    Unknown *receiver,
    Unknown *selector)
{
    Thunk *newThunk;
    
    newThunk = (Thunk *)Object_New(CLASS(Thunk));
    if (!newThunk)
        return 0;
    newThunk->receiver = receiver;  INCREF(receiver);
    newThunk->selector = selector;  INCREF(selector);
    return newThunk;
}
