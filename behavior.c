
#include "behavior.h"

#include "cgen.h"
#include "class.h"
#include "heart.h"
#include "host.h"
#include "interp.h"
#include "module.h"
#include "native.h"
#include "rodata.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>


/*------------------------------------------------------------------------*/
/* class template */

typedef struct SpkBehaviorSubclass {
    SpkBehavior base;
    SpkUnknown *variables[1]; /* stretchy */
} SpkBehaviorSubclass;

static SpkAccessorTmpl accessors[] = {
    { "superclass", Spk_T_OBJECT, offsetof(SpkBehavior, superclass), SpkAccessor_READ },
    { 0 }
};

static SpkMethodTmpl methods[] = {
    { 0 }
};

SpkClassTmpl Spk_ClassBehaviorTmpl = {
    Spk_HEART_CLASS_TMPL(Behavior, Object), {
        accessors,
        methods,
        /*lvalueMethods*/ 0,
        offsetof(SpkBehaviorSubclass, variables)
    }, /*meta*/ {
        0
    }
};


/*------------------------------------------------------------------------*/
/* C API */

void SpkBehavior_Init(SpkBehavior *self, SpkBehavior *superclass, SpkModule *module, size_t instVarCount) {
    size_t i;
    SpkMethodNamespace ns;
    
    Spk_INCREF(superclass);
    Spk_XINCREF(module);
    self->superclass = superclass;
    self->module = module;
    
    for (ns = 0; ns < Spk_NUM_METHOD_NAMESPACES; ++ns) {
        self->methodDict[ns] = SpkHost_NewSymbolDict();
    }
    for (i = 0; i < Spk_NUM_OPER; ++i) {
        self->operTable[i] = 0;
    }
    for (i = 0; i < Spk_NUM_CALL_OPER; ++i) {
        self->operCallTable[i] = 0;
    }
    self->assignInd = 0;
    self->assignIndex = 0;
    
    /* low-level hooks */
    self->zero = superclass->zero;
    self->dealloc = superclass->dealloc;
    self->traverse = superclass->traverse;
    
    /* temporary */
    self->next = 0;
    self->nextInScope = 0;
    self->nestedClassList.first = 0;
    self->nestedClassList.last = 0;
    
    /* memory layout of instances */
    self->instVarOffset = superclass->instVarOffset;
    self->instVarBaseIndex = superclass->instVarBaseIndex + superclass->instVarCount;
    self->instVarCount = instVarCount;
    self->instanceSize = superclass->instanceSize + self->instVarCount*sizeof(SpkUnknown *);
    self->itemSize = superclass->itemSize;
    
    return;
}

void SpkBehavior_InitFromTemplate(SpkBehavior *self, SpkBehaviorTmpl *tmpl, SpkBehavior *superclass, SpkModule *module) {
    
    SpkBehavior_Init(self, superclass, module, 0);
    
    if (tmpl->instVarOffset != 0) {
        assert(tmpl->instVarOffset >= self->instVarOffset);
        self->instanceSize += tmpl->instVarOffset - self->instVarOffset;
        self->instVarOffset = tmpl->instVarOffset;
    }
    
    if (tmpl->itemSize != 0) {
        assert(self->itemSize == 0 ||
               self->itemSize == tmpl->itemSize);
        self->itemSize = tmpl->itemSize;
    }
    
    if (tmpl->zero)
        self->zero = tmpl->zero;
    if (tmpl->dealloc)
        self->dealloc = tmpl->dealloc;
    if (tmpl->traverse)
        self->traverse = *tmpl->traverse;
}

static SpkMethod *methodFromTemplate(SpkMethodTmpl *methodTmpl) {
    SpkMethod *method;
    
    if (methodTmpl->bytecode) {
        method = SpkMethod_New(methodTmpl->bytecodeSize);
        memcpy(SpkMethod_OPCODES(method),
               methodTmpl->bytecode,
               methodTmpl->bytecodeSize);
        method->nativeCode = methodTmpl->nativeCode;
    } else {
        method = Spk_NewNativeMethod(methodTmpl->nativeFlags, methodTmpl->nativeCode);
    }
    return method;
}

void SpkBehavior_AddMethodsFromTemplate(SpkBehavior *self, SpkBehaviorTmpl *tmpl) {
    
    if (tmpl->accessors) {
        SpkAccessorTmpl *accessorTmpl;
        
        for (accessorTmpl = tmpl->accessors; accessorTmpl->name; ++accessorTmpl) {
            SpkUnknown *messageSelector;
            SpkMethod *method;
            
            messageSelector = SpkHost_SymbolFromCString(accessorTmpl->name);
            if (accessorTmpl->flags & SpkAccessor_READ) {
                method = SpkCodeGen_NewNativeAccessor(SpkAccessor_READ,
                                                      accessorTmpl->type,
                                                      accessorTmpl->offset);
                SpkBehavior_InsertMethod(self, Spk_METHOD_NAMESPACE_RVALUE, messageSelector, method);
                Spk_DECREF(method);
            }
            
            if (accessorTmpl->flags & SpkAccessor_WRITE) {
                method = SpkCodeGen_NewNativeAccessor(SpkAccessor_WRITE,
                                                      accessorTmpl->type,
                                                      accessorTmpl->offset);
                SpkBehavior_InsertMethod(self, Spk_METHOD_NAMESPACE_LVALUE, messageSelector, method);
                Spk_DECREF(method);
            }
            
            Spk_DECREF(messageSelector);
        }
    }
    
    if (tmpl->methods) {
        SpkMethodTmpl *methodTmpl;
        
        for (methodTmpl = tmpl->methods; methodTmpl->name; ++methodTmpl) {
            SpkUnknown *messageSelector;
            SpkMethod *method;
            
            messageSelector = Spk_ParseSelector(methodTmpl->name);
            method = methodFromTemplate(methodTmpl);
            SpkBehavior_InsertMethod(self, Spk_METHOD_NAMESPACE_RVALUE, messageSelector, method);
            Spk_DECREF(method);
            Spk_DECREF(messageSelector);
        }
    }
    if (tmpl->lvalueMethods) {
        SpkMethodTmpl *methodTmpl;
        
        for (methodTmpl = tmpl->lvalueMethods; methodTmpl->name; ++methodTmpl) {
            SpkUnknown *messageSelector;
            SpkMethod *method;
            
            messageSelector = Spk_ParseSelector(methodTmpl->name);
            method = methodFromTemplate(methodTmpl);
            SpkBehavior_InsertMethod(self, Spk_METHOD_NAMESPACE_LVALUE, messageSelector, method);
            Spk_DECREF(method);
            Spk_DECREF(messageSelector);
        }
    }
}

static void maybeAccelerateMethod(SpkBehavior *self, SpkMethodNamespace ns, SpkUnknown *messageSelector, SpkMethod *method) {
    const char *name;
    SpkOper oper;
    
    name = SpkHost_SelectorAsCString(messageSelector);
    if (!name) {
        return;
    }
    if (name[0] == '_' && name[1] == '_') {
        /* special selector */
        switch (ns) {
        case Spk_METHOD_NAMESPACE_RVALUE:
            for (oper = 0; oper < Spk_NUM_CALL_OPER; ++oper) {
                if (messageSelector == *Spk_operCallSelectors[oper].selector) {
                    Spk_INCREF(method);
                    Spk_XDECREF(self->operCallTable[oper]);
                    self->operCallTable[oper] = method;
                    break;
                }
            }
            if (oper >= Spk_NUM_CALL_OPER) {
                for (oper = 0; oper < Spk_NUM_OPER; ++oper) {
                    if (messageSelector == *Spk_operSelectors[oper].selector) {
                        Spk_INCREF(method);
                        Spk_XDECREF(self->operTable[oper]);
                        self->operTable[oper] = method;
                        break;
                    }
                }
            }
            break;
        case Spk_METHOD_NAMESPACE_LVALUE:
            if (messageSelector == *Spk_operSelectors[Spk_OPER_IND].selector) {
                Spk_INCREF(method);
                Spk_XDECREF(self->assignInd);
                self->assignInd = method;
            } else if (messageSelector == *Spk_operCallSelectors[Spk_OPER_INDEX].selector) {
                Spk_INCREF(method);
                Spk_XDECREF(self->assignIndex);
                self->assignIndex = method;
            }
            break;
        }
    }
}

void SpkBehavior_InsertMethod(SpkBehavior *self, SpkMethodNamespace ns, SpkUnknown *messageSelector, SpkMethod *method) {
    SpkHost_DefineSymbol(self->methodDict[ns], messageSelector, (SpkUnknown *)method);
    maybeAccelerateMethod(self, ns, messageSelector, method);
}

SpkMethod *SpkBehavior_LookupMethod(SpkBehavior *self, SpkMethodNamespace ns, SpkUnknown *messageSelector) {
    SpkUnknown *value;
    
    value = SpkHost_SymbolValue(self->methodDict[ns], messageSelector);
    if (value) {
        return Spk_CAST(Method, value);
    }
    return 0;
}

SpkUnknown *SpkBehavior_FindSelectorOfMethod(SpkBehavior *self, SpkMethod *method) {
    SpkMethodNamespace ns;
    SpkUnknown *messageSelector;
    
    for (ns = 0; ns < Spk_NUM_METHOD_NAMESPACES; ++ns) {
        messageSelector = SpkHost_FindSymbol(self->methodDict[ns], (SpkUnknown *)method);
        if (messageSelector) {
            return messageSelector;
        }
    }
    return 0;
}

const char *SpkBehavior_NameAsCString(SpkBehavior *self) {
    SpkClass *c;
    SpkUnknown *name;
    const char *result;
    
    c = Spk_CAST(Class, self);
    if (!c)
        return "<unknown>";
    
    name = SpkClass_Name(c);
    result = SpkHost_SymbolAsCString(name);
    Spk_DECREF(name);
    return result;
}
