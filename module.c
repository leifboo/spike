
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
    tmp = Spk_CallAttr(Spk_GLOBAL(theInterpreter), (SpkUnknown *)self, Spk__predef, 0);
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


#ifdef MALTIPY
static SpkUnknown *Module__initPythonModule(SpkUnknown *_self, SpkUnknown *module, SpkUnknown *arg1) {
    SpkObject *self;
    SpkUnknown *args, *methodDict, *selector, *value, *thunk;
    int pos;
    
    self = (SpkObject *)_self;

    args = Spk_emptyArgs;
    Spk_INCREF(args);
    
    methodDict = self->klass->methodDict[Spk_METHOD_NAMESPACE_RVALUE];
    pos = 0;
    while (PyDict_Next(methodDict, &pos, &selector, &value)) {
        thunk = Spk_SendMessage(Spk_GLOBAL(theInterpreter), _self, Spk_METHOD_NAMESPACE_RVALUE, selector, args);
        if (!thunk) {
            goto error;
        }
        PyObject_SetAttr(module, selector, thunk);
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
    { "_init", SpkNativeCode_METH_ATTR | SpkNativeCode_ARGS_0, &Module__init },
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
