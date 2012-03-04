
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
/* low-level hooks */

static void Behavior_zero(Object *_self) {
    Behavior *self = (Behavior *)_self;
    MethodNamespace ns;
    size_t i;
    
    (*CLASS(Behavior)->superclass->zero)(_self);
    
    self->superclass = 0;
    self->module = 0;
    
    for (ns = 0; ns < NUM_METHOD_NAMESPACES; ++ns) {
        self->methodDict[ns] = 0;
    }
    for (i = 0; i < NUM_OPER; ++i) {
        self->operTable[i] = 0;
    }
    for (i = 0; i < NUM_CALL_OPER; ++i) {
        self->operCallTable[i] = 0;
    }
    self->assignInd = 0;
    self->assignIndex = 0;
    
    self->zero = 0;
    self->dealloc = 0;
    
    self->instVarOffset = 0;
    self->instVarBaseIndex = 0;
    self->instVarCount = 0;
    self->instanceSize = 0;
    self->itemSize = 0;
}


/*------------------------------------------------------------------------*/
/* class template */

typedef struct BehaviorSubclass {
    Behavior base;
    Unknown *variables[1]; /* stretchy */
} BehaviorSubclass;

static AccessorTmpl accessors[] = {
    { "superclass", T_OBJECT, offsetof(Behavior, superclass), Accessor_READ },
    { 0 }
};

static MethodTmpl methods[] = {
    { 0 }
};

ClassTmpl ClassBehaviorTmpl = {
    HEART_CLASS_TMPL(Behavior, Object), {
        accessors,
        methods,
        /*lvalueMethods*/ 0,
        offsetof(BehaviorSubclass, variables),
        /*itemSize*/ 0,
        &Behavior_zero
    }, /*meta*/ {
        0
    }
};


/*------------------------------------------------------------------------*/
/* C API */

void Behavior_Init(Behavior *self, Behavior *superclass, Module *module, size_t instVarCount) {
    size_t i;
    MethodNamespace ns;
    
    self->superclass = superclass;
    self->module = module;
    
    for (ns = 0; ns < NUM_METHOD_NAMESPACES; ++ns) {
        self->methodDict[ns] = Host_NewSymbolDict();
    }
    for (i = 0; i < NUM_OPER; ++i) {
        self->operTable[i] = 0;
    }
    for (i = 0; i < NUM_CALL_OPER; ++i) {
        self->operCallTable[i] = 0;
    }
    self->assignInd = 0;
    self->assignIndex = 0;
    
    /* low-level hooks */
    self->zero = superclass->zero;
    self->dealloc = superclass->dealloc;
    
    /* memory layout of instances */
    self->instVarOffset = superclass->instVarOffset;
    self->instVarBaseIndex = superclass->instVarBaseIndex + superclass->instVarCount;
    self->instVarCount = instVarCount;
    self->instanceSize = superclass->instanceSize + self->instVarCount*sizeof(Unknown *);
    self->itemSize = superclass->itemSize;
    
    return;
}

void Behavior_InitFromTemplate(Behavior *self, BehaviorTmpl *tmpl, Behavior *superclass, Module *module) {
    
    Behavior_Init(self, superclass, module, tmpl->instVarCount);
    
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
}

static Method *methodFromTemplate(MethodTmpl *methodTmpl) {
    Method *method;
    
    if (methodTmpl->bytecode) {
        method = Method_New(methodTmpl->bytecodeSize);
        memcpy(Method_OPCODES(method),
               methodTmpl->bytecode,
               methodTmpl->bytecodeSize);
        method->nativeCode = methodTmpl->nativeCode;
        
        method->debug.source = methodTmpl->debug.source;
        method->debug.lineCodeTally = methodTmpl->debug.lineCodeTally;
        if (methodTmpl->debug.lineCodes) {
            method->debug.lineCodes
                = (Opcode *)malloc(method->debug.lineCodeTally * sizeof(Opcode));
            memcpy(method->debug.lineCodes,
                   methodTmpl->debug.lineCodes,
                   method->debug.lineCodeTally);
        }
    } else {
        method = NewNativeMethod(methodTmpl->nativeFlags, methodTmpl->nativeCode);
    }
    return method;
}

void Behavior_AddMethodsFromTemplate(Behavior *self, BehaviorTmpl *tmpl) {
    
    if (tmpl->accessors) {
        AccessorTmpl *accessorTmpl;
        
        for (accessorTmpl = tmpl->accessors; accessorTmpl->name; ++accessorTmpl) {
            Unknown *selector;
            Method *method;
            
            selector = Host_SymbolFromCString(accessorTmpl->name);
            if (accessorTmpl->flags & Accessor_READ) {
                method = CodeGen_NewNativeAccessor(Accessor_READ,
                                                      accessorTmpl->type,
                                                      accessorTmpl->offset);
                Behavior_InsertMethod(self, METHOD_NAMESPACE_RVALUE, selector, method);
            }
            
            if (accessorTmpl->flags & Accessor_WRITE) {
                method = CodeGen_NewNativeAccessor(Accessor_WRITE,
                                                      accessorTmpl->type,
                                                      accessorTmpl->offset);
                Behavior_InsertMethod(self, METHOD_NAMESPACE_LVALUE, selector, method);
            }
        }
    }
    
    if (tmpl->methods) {
        MethodTmpl *methodTmpl;
        
        for (methodTmpl = tmpl->methods; methodTmpl->name; ++methodTmpl) {
            Unknown *selector;
            Method *method;
            
            selector = ParseSelector(methodTmpl->name);
            method = methodFromTemplate(methodTmpl);
            Behavior_InsertMethod(self, METHOD_NAMESPACE_RVALUE, selector, method);
        }
    }
    if (tmpl->lvalueMethods) {
        MethodTmpl *methodTmpl;
        
        for (methodTmpl = tmpl->lvalueMethods; methodTmpl->name; ++methodTmpl) {
            Unknown *selector;
            Method *method;
            
            selector = ParseSelector(methodTmpl->name);
            method = methodFromTemplate(methodTmpl);
            Behavior_InsertMethod(self, METHOD_NAMESPACE_LVALUE, selector, method);
        }
    }
    
    if (tmpl->methodList.first) {
        MethodTmpl *methodTmpl;
        
        for (methodTmpl = tmpl->methodList.first;
             methodTmpl;
             methodTmpl = methodTmpl->nextInScope) {
            Method *method;
            
            method = methodFromTemplate(methodTmpl);
            Behavior_InsertMethod(self,
                                     methodTmpl->ns,
                                     methodTmpl->selector,
                                     method);
        }
    }
}

static void maybeAccelerateMethod(Behavior *self, MethodNamespace ns, Unknown *selector, Method *method) {
    const char *name;
    Oper oper;
    
    name = Host_SelectorAsCString(selector);
    if (!name) {
        return;
    }
    if (name[0] == '_' && name[1] == '_') {
        /* special selector */
        switch (ns) {
        case METHOD_NAMESPACE_RVALUE:
            for (oper = 0; oper < NUM_CALL_OPER; ++oper) {
                if (selector == *operCallSelectors[oper].selector) {
                    self->operCallTable[oper] = method;
                    break;
                }
            }
            if (oper >= NUM_CALL_OPER) {
                for (oper = 0; oper < NUM_OPER; ++oper) {
                    if (selector == *operSelectors[oper].selector) {
                        self->operTable[oper] = method;
                        break;
                    }
                }
            }
            break;
        case METHOD_NAMESPACE_LVALUE:
            if (selector == *operSelectors[OPER_IND].selector) {
                self->assignInd = method;
            } else if (selector == *operCallSelectors[OPER_INDEX].selector) {
                self->assignIndex = method;
            }
            break;
        }
    }
}

void Behavior_InsertMethod(Behavior *self, MethodNamespace ns, Unknown *selector, Method *method) {
    Host_DefineSymbol(self->methodDict[ns], selector, (Unknown *)method);
    maybeAccelerateMethod(self, ns, selector, method);
}

Method *Behavior_LookupMethod(Behavior *self, MethodNamespace ns, Unknown *selector) {
    Unknown *value;
    
    value = Host_SymbolValue(self->methodDict[ns], selector);
    if (value) {
        return CAST(Method, value);
    }
    return 0;
}

Unknown *Behavior_FindSelectorOfMethod(Behavior *self, Method *method) {
    MethodNamespace ns;
    Unknown *selector;
    
    for (ns = 0; ns < NUM_METHOD_NAMESPACES; ++ns) {
        selector = Host_FindSymbol(self->methodDict[ns], (Unknown *)method);
        if (selector) {
            return selector;
        }
    }
    return 0;
}

const char *Behavior_NameAsCString(Behavior *self) {
    Class *c;
    Unknown *name;
    const char *result;
    
    c = CAST(Class, self);
    if (!c)
        return "<unknown>";
    
    name = Class_Name(c);
    result = Host_SymbolAsCString(name);
    return result;
}
