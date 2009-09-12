
#include "behavior.h"

#include "cgen.h"
#include "class.h"
#include "host.h"
#include "interp.h"
#include "module.h"
#include "native.h"
#include "rodata.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>


SpkBehavior *Spk_ClassBehavior;


/*------------------------------------------------------------------------*/
/* class template */

typedef struct SpkBehaviorSubclass {
    SpkBehavior base;
    SpkUnknown *variables[1]; /* stretchy */
} SpkBehaviorSubclass;

static SpkAccessorTmpl accessors[] = {
    { "superclass", Spk_T_OBJECT, offsetof(SpkBehavior, superclass), SpkAccessor_READ },
    { 0, 0, 0, 0 }
};

static SpkMethodTmpl methods[] = {
    { 0, 0, 0}
};

SpkClassTmpl Spk_ClassBehaviorTmpl = {
    "Behavior", {
        accessors,
        methods,
        /*lvalueMethods*/ 0,
        offsetof(SpkBehaviorSubclass, variables)
    }, /*meta*/ {
    }
};


/*------------------------------------------------------------------------*/
/* C API */

void SpkBehavior_Init(SpkBehavior *self, SpkBehavior *superclass, SpkModule *module, size_t instVarCount) {
    size_t i;
    SpkMethodNamespace namespace;
    
    Spk_INCREF(superclass);
    Spk_XINCREF(module);
    self->superclass = superclass;
    self->module = module;
    
    for (namespace = 0; namespace < Spk_NUM_METHOD_NAMESPACES; ++namespace) {
        self->ns[namespace].methodDict = SpkHost_NewSymbolDict();
        for (i = 0; i < Spk_NUM_OPER; ++i) {
            self->ns[namespace].operTable[i] = 0;
        }
        for (i = 0; i < Spk_NUM_CALL_OPER; ++i) {
            self->ns[namespace].operCallTable[i] = 0;
        }
    }
    
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

void SpkBehavior_InitFromTemplate(SpkBehavior *self, SpkBehaviorTmpl *template, SpkBehavior *superclass, SpkModule *module) {
    
    SpkBehavior_Init(self, superclass, module, 0);
    
    if (template->instVarOffset != 0) {
        assert(template->instVarOffset >= self->instVarOffset);
        self->instanceSize += template->instVarOffset - self->instVarOffset;
        self->instVarOffset = template->instVarOffset;
    }
    
    if (template->itemSize != 0) {
        assert(self->itemSize == 0 ||
               self->itemSize == template->itemSize);
        self->itemSize = template->itemSize;
    }
    
    if (template->zero)
        self->zero = template->zero;
    if (template->dealloc)
        self->dealloc = template->dealloc;
    if (template->traverse)
        self->traverse = *template->traverse;
}

void SpkBehavior_AddMethodsFromTemplate(SpkBehavior *self, SpkBehaviorTmpl *template) {
    
    if (template->accessors) {
        SpkAccessorTmpl *accessorTmpl;
        
        for (accessorTmpl = template->accessors; accessorTmpl->name; ++accessorTmpl) {
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
    
    if (template->methods) {
        SpkMethodTmpl *methodTmpl;
        
        for (methodTmpl = template->methods; methodTmpl->name; ++methodTmpl) {
            SpkUnknown *messageSelector;
            SpkMethod *method;
            
            messageSelector = Spk_ParseSelector(methodTmpl->name);
            method = Spk_NewNativeMethod(methodTmpl->flags, methodTmpl->code);
            SpkBehavior_InsertMethod(self, Spk_METHOD_NAMESPACE_RVALUE, messageSelector, method);
            Spk_DECREF(method);
            Spk_DECREF(messageSelector);
        }
    }
    if (template->lvalueMethods) {
        SpkMethodTmpl *methodTmpl;
        
        for (methodTmpl = template->lvalueMethods; methodTmpl->name; ++methodTmpl) {
            SpkUnknown *messageSelector;
            SpkMethod *method;
            
            messageSelector = Spk_ParseSelector(methodTmpl->name);
            method = Spk_NewNativeMethod(methodTmpl->flags, methodTmpl->code);
            SpkBehavior_InsertMethod(self, Spk_METHOD_NAMESPACE_LVALUE, messageSelector, method);
            Spk_DECREF(method);
            Spk_DECREF(messageSelector);
        }
    }
}

static void maybeAccelerateMethod(SpkBehavior *self, SpkMethodNamespace namespace, SpkUnknown *messageSelector, SpkMethod *method) {
    const char *name;
    SpkOper operator;
    
    name = SpkHost_SelectorAsCString(messageSelector);
    if (!name) {
        return;
    }
    if (name[0] == '_' && name[1] == '_') {
        /* special selector */
        for (operator = 0; operator < Spk_NUM_CALL_OPER; ++operator) {
            if (messageSelector == *Spk_operCallSelectors[operator].selector) {
                Spk_INCREF(method);
                Spk_INCREF(self);
                Spk_XDECREF(self->ns[namespace].operCallTable[operator]);
                self->ns[namespace].operCallTable[operator] = method;
                break;
            }
        }
        if (operator >= Spk_NUM_CALL_OPER) {
            for (operator = 0; operator < Spk_NUM_OPER; ++operator) {
                if (messageSelector == *Spk_operSelectors[operator].selector) {
                    Spk_INCREF(method);
                    Spk_INCREF(self);
                    Spk_XDECREF(self->ns[namespace].operTable[operator]);
                    self->ns[namespace].operTable[operator] = method;
                    break;
                }
            }
        }
    }
}

void SpkBehavior_InsertMethod(SpkBehavior *self, SpkMethodNamespace namespace, SpkUnknown *messageSelector, SpkMethod *method) {
    SpkHost_DefineSymbol(self->ns[namespace].methodDict, messageSelector, (SpkUnknown *)method);
    maybeAccelerateMethod(self, namespace, messageSelector, method);
}

SpkMethod *SpkBehavior_LookupMethod(SpkBehavior *self, SpkMethodNamespace namespace, SpkUnknown *messageSelector) {
    SpkUnknown *value;
    
    value = SpkHost_SymbolValue(self->ns[namespace].methodDict, messageSelector);
    if (value) {
        return Spk_CAST(Method, value);
    }
    return 0;
}

SpkUnknown *SpkBehavior_FindSelectorOfMethod(SpkBehavior *self, SpkMethod *method) {
    SpkMethodNamespace namespace;
    SpkUnknown *messageSelector;
    
    for (namespace = 0; namespace < Spk_NUM_METHOD_NAMESPACES; ++namespace) {
        messageSelector = SpkHost_FindSymbol(self->ns[namespace].methodDict, (SpkUnknown *)method);
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
