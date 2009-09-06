
#include "module.h"

#include "class.h"
#include "rodata.h"
#include <stdlib.h>


SpkBehavior *Spk_ClassModule;
SpkModule *Spk_builtInModule;


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
    
    methodDict = self->klass->ns[Spk_METHOD_NAMESPACE_RVALUE].methodDict;
    pos = 0;
    while (PyDict_Next(methodDict, &pos, &messageSelector, &value)) {
        thunk = Spk_SendMessage(theInterpreter, _self, Spk_METHOD_NAMESPACE_RVALUE, messageSelector, args);
        if (!thunk) {
            goto error;
        }
        PyObject_SetAttr(module, messageSelector, thunk);
    }
    Spk_DECREF(args);
    Spk_INCREF(Spk_void);
    return Spk_void;

 error:
    Spk_DECREF(args);
    return 0;
}
#endif /* MALTIPY */


/*------------------------------------------------------------------------*/
/* class template */

typedef struct SpkModuleSubclass {
    SpkModule base;
    SpkUnknown *variables[1]; /* stretchy */
} SpkModuleSubclass;

static SpkMethodTmpl methods[] = {
#ifdef MALTIPY
    { "_initPythonModule", SpkNativeCode_METH_ATTR | SpkNativeCode_ARGS_1, &Module__initPythonModule },
#endif
    { 0, 0, 0}
};

SpkClassTmpl Spk_ClassModuleTmpl = {
    "Module", {
        /*accessors*/ 0,
        methods,
        /*lvalueMethods*/ 0,
        offsetof(SpkModuleSubclass, variables)
    }, /*meta*/ {
    }
};


/*------------------------------------------------------------------------*/
/* C API */

SpkModule *SpkModule_New(SpkBehavior *moduleSubclass) {
    SpkModule *newModule;
    
    newModule = (SpkModule *)SpkObject_New(moduleSubclass);
    newModule->literalCount = 0;
    newModule->literals = 0;
    newModule->firstClass = 0;
    return newModule;
}
