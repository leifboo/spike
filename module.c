
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


/*------------------------------------------------------------------------*/
/* methods */

#ifdef MALTIPY
static SpkUnknown *Module__initPythonModule(SpkUnknown *_self, SpkUnknown *module, SpkUnknown *arg1) {
    SpkObject *self;
    SpkUnknown *args, *methodDict, *messageSelector, *value, *thunk;
    int pos;
    
    self = (SpkObject *)_self;

    args = Spk_emptyArgs;
    Spk_INCREF(args);
    
    methodDict = self->klass->methodDict[Spk_METHOD_NAMESPACE_RVALUE];
    pos = 0;
    while (PyDict_Next(methodDict, &pos, &messageSelector, &value)) {
        thunk = Spk_SendMessage(Spk_GLOBAL(theInterpreter), _self, Spk_METHOD_NAMESPACE_RVALUE, messageSelector, args);
        if (!thunk) {
            goto error;
        }
        PyObject_SetAttr(module, messageSelector, thunk);
    }
    Spk_DECREF(args);
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);

 error:
    Spk_DECREF(args);
    return 0;
}
#endif /* MALTIPY */


/*------------------------------------------------------------------------*/
/* low-level hooks */

static void Module_zero(SpkObject *_self) {
    SpkModule *self;
    
    self = (SpkModule *)_self;
    (*Spk_CLASS(Module)->superclass->zero)(_self);
    self->firstClass = 0;
}

static void ModuleClass_zero(SpkObject *_self) {
    SpkModuleClass *self;
    
    self = (SpkModuleClass *)_self;
    (*Spk_CLASS(Class)->zero)(_self);
    self->literalCount = 0;
    self->literals = 0;
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
#ifdef MALTIPY
    { "_initPythonModule", SpkNativeCode_METH_ATTR | SpkNativeCode_ARGS_1, &Module__initPythonModule },
#endif
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
        switch (tmpl->literals[i].kind) {
        case Spk_LITERAL_SYMBOL:
            literal = (SpkUnknown *)SpkSymbol_FromCString(tmpl->literals[i].stringValue);
            break;
        case Spk_LITERAL_INTEGER:
            literal = (SpkUnknown *)SpkInteger_FromCLong(tmpl->literals[i].intValue);
            break;
        case Spk_LITERAL_FLOAT:
            literal = (SpkUnknown *)SpkFloat_FromCDouble(tmpl->literals[i].floatValue);
            break;
        case Spk_LITERAL_CHAR:
            literal = (SpkUnknown *)SpkChar_FromCChar(tmpl->literals[i].intValue);
            break;
        case Spk_LITERAL_STRING:
            literal = (SpkUnknown *)SpkString_FromCString(tmpl->literals[i].stringValue);
            break;
        }
        self->literals[i] = literal;
    }
}
