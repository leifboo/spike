
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


typedef struct SpkThunk SpkThunk;


static SpkThunk *SpkThunk_New(SpkUnknown *, SpkUnknown *);


/*------------------------------------------------------------------------*/
/* methods */

static SpkUnknown *Module__init(SpkUnknown *_self, SpkUnknown *arg0, SpkUnknown *arg1) {
    SpkModule *self;
    SpkModuleClass *moduleClass;
    SpkModuleTmpl *tmpl;
    SpkUnknown **globals;
    SpkClassTmpl *classTmpl;
    SpkUnknown *tmp;
    
    self = (SpkModule *)_self;
    moduleClass = (SpkModuleClass *)self->base.klass;
    tmpl = moduleClass->tmpl;
    
    /* XXX: Module inheritance will introduce multiple sets of globals. */
    globals = SpkModule_VARIABLES(self);
    
    /* Initialize predefined variables. */
    tmp = Spk_Send(Spk_GLOBAL(theInterpreter), (SpkUnknown *)self, Spk__predef, 0);
    if (!tmp)
        return 0;
    Spk_DECREF(tmp);
    
    /* Create all classes. */
    for (classTmpl = tmpl->classList.first;
         classTmpl;
         classTmpl = classTmpl->next) {
        
        SpkUnknown **classVar;
        SpkBehavior *superclass;
        
        /* The class list is a preorder traversal of the inheritance graph;
           this guarantees that the superclass is created before its subclasses. */
        superclass = Spk_CAST(Behavior, globals[classTmpl->superclassVarIndex]);
        classVar = &globals[classTmpl->classVarIndex];
        *classVar = (SpkUnknown *)SpkClass_FromTemplate(classTmpl, superclass, self);
    }
    
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
}


static SpkUnknown *Module__thunk(SpkUnknown *_self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return (SpkUnknown *)SpkThunk_New(_self, arg0);
}


/*------------------------------------------------------------------------*/
/* low-level hooks */

static void Module_zero(SpkObject *_self) {
    SpkModule *self;
    
    self = (SpkModule *)_self;
    (*Spk_CLASS(Module)->superclass->zero)(_self);
    /* XXX: This is the wrong, but necessary. */
    self->base.klass->module = self;
}

static void ModuleClass_zero(SpkObject *_self) {
    SpkModuleClass *self;
    
    self = (SpkModuleClass *)_self;
    (*Spk_CLASS(Class)->zero)(_self);
    self->literalCount = 0;
    self->literals = 0;
    self->tmpl = 0;
}


/*------------------------------------------------------------------------*/
/* class tmpl */

typedef struct SpkModuleSubclass {
    SpkModule base;
    SpkUnknown *variables[1]; /* stretchy */
} SpkModuleSubclass;

typedef struct SpkModuleClassSubclass {
    SpkModuleClass base;
    SpkUnknown *variables[1]; /* stretchy */
} SpkModuleClassSubclass;

static SpkMethodTmpl methods[] = {
    { "_init", SpkNativeCode_ARGS_0, &Module__init },
    { "_thunk", SpkNativeCode_ARGS_1, &Module__thunk },
    { 0 }
};

SpkClassTmpl Spk_ClassModuleTmpl = {
    Spk_HEART_CLASS_TMPL(Module, Object), {
        /*accessors*/ 0,
        methods,
        /*lvalueMethods*/ 0,
        offsetof(SpkModuleSubclass, variables),
        /*itemSize*/ 0,
        &Module_zero
    }, /*meta*/ {
        /*accessors*/ 0,
        /*methods*/ 0,
        /*lvalueMethods*/ 0,
        offsetof(SpkModuleClassSubclass, variables),
        /*itemSize*/ 0,
        &ModuleClass_zero
    }
};


/*------------------------------------------------------------------------*/
/* C API */

SpkModuleClass *SpkModuleClass_New(SpkModuleTmpl *tmpl) {
    SpkModuleClass *moduleClass;
    
    moduleClass
        = (SpkModuleClass *)SpkClass_FromTemplate(&tmpl->moduleClass,
                                                  Spk_CLASS(Module),
                                                  0);
    
    moduleClass->literalCount = tmpl->literalCount;
    moduleClass->literals = tmpl->literals;
    moduleClass->tmpl = tmpl;  Spk_INCREF(tmpl);
    return moduleClass;
}

void SpkModule_InitLiteralsFromTemplate(SpkBehavior *moduleClass, SpkModuleTmpl *tmpl) {
    SpkModuleClass *self;
    size_t i;
    SpkUnknown *literal;
    
    self = (SpkModuleClass *)Spk_Cast(Spk_CLASS(Module)->base.klass,
                                      (SpkUnknown *)moduleClass);
    if (!tmpl->literalCount)
        return;
    self->literals = (SpkUnknown **)malloc(tmpl->literalCount * sizeof(SpkUnknown *));
    self->literalCount = tmpl->literalCount;
    for (i = 0; i < self->literalCount; ++i) {
        switch (tmpl->literalTable[i].kind) {
        case Spk_LITERAL_SYMBOL:
            literal = (SpkUnknown *)SpkSymbol_FromCString(tmpl->literalTable[i].stringValue);
            break;
        case Spk_LITERAL_INTEGER:
            literal = (SpkUnknown *)SpkInteger_FromCLong(tmpl->literalTable[i].intValue);
            break;
        case Spk_LITERAL_FLOAT:
            literal = (SpkUnknown *)SpkFloat_FromCDouble(tmpl->literalTable[i].floatValue);
            break;
        case Spk_LITERAL_CHAR:
            literal = (SpkUnknown *)SpkChar_FromCChar(tmpl->literalTable[i].intValue);
            break;
        case Spk_LITERAL_STRING:
            literal = (SpkUnknown *)SpkString_FromCString(tmpl->literalTable[i].stringValue);
            break;
        }
        self->literals[i] = literal;
    }
}


/*------------------------------------------------------------------------*/
/* thunks */

struct SpkThunk {
    SpkObject base;
    SpkUnknown *receiver;
    SpkUnknown *selector;
};

typedef struct SpkThunkSubclass {
    SpkThunk base;
    SpkUnknown *variables[1]; /* stretchy */
} SpkThunkSubclass;

static SpkUnknown *Thunk_apply(SpkUnknown *_self, SpkUnknown *arg0, SpkUnknown *arg1) {
    SpkThunk *self = (SpkThunk *)_self;
    return Spk_SendWithArguments(Spk_GLOBAL(theInterpreter),
                                 self->receiver,
                                 self->selector,
                                 arg0);
}

static SpkMethodTmpl ThunkMethods[] = {
    { "__apply__", SpkNativeCode_ARGS_ARRAY, &Thunk_apply },
    { 0 }
};

static void Thunk_dealloc(SpkObject *_self, SpkUnknown **l) {
    SpkThunk *self = (SpkThunk *)_self;
    Spk_LDECREF(self->receiver, l);
    Spk_LDECREF(self->selector, l);
    (*Spk_CLASS(Thunk)->superclass->dealloc)(_self, l);
}

SpkClassTmpl Spk_ClassThunkTmpl = {
    Spk_HEART_CLASS_TMPL(Thunk, Object), {
        /*accessors*/ 0,
        ThunkMethods,
        /*lvalueMethods*/ 0,
        offsetof(SpkThunkSubclass, variables),
        /*itemSize*/ 0,
        /*zero*/ 0,
        &Thunk_dealloc
    }, /*meta*/ {
        0
    }
};

static SpkThunk *SpkThunk_New(
    SpkUnknown *receiver,
    SpkUnknown *selector)
{
    SpkThunk *newThunk;
    
    newThunk = (SpkThunk *)SpkObject_New(Spk_CLASS(Thunk));
    if (!newThunk)
        return 0;
    newThunk->receiver = receiver;  Spk_INCREF(receiver);
    newThunk->selector = selector;  Spk_INCREF(selector);
    return newThunk;
}
