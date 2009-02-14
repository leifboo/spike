
#include "behavior.h"

#include "class.h"
#include "dict.h"
#include "interp.h"
#include "module.h"
#include "native.h"
#include "sym.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


Behavior *Spk_ClassBehavior;


SpecialSelector Spk_operSelectors[NUM_OPER] = {
    { "__succ__",   0, 0 },
    { "__pred__",   0, 0 },
    { "__addr__",   0, 0 }, /* "&x" -- not implemented */
    { "__ind__",    0, 0 }, /* "*x" -- not implemented */
    { "__pos__",    0, 0 },
    { "__neg__",    0, 0 },
    { "__bneg__",   0, 0 },
    { "__lneg__",   0, 0 },
    { "__mul__",    1, 0 },
    { "__div__",    1, 0 },
    { "__mod__",    1, 0 },
    { "__add__",    1, 0 },
    { "__sub__",    1, 0 },
    { "__lshift__", 1, 0 },
    { "__rshift__", 1, 0 },
    { "__lt__",     1, 0 },
    { "__gt__",     1, 0 },
    { "__le__",     1, 0 },
    { "__ge__",     1, 0 },
    { "__eq__",     1, 0 },
    { "__ne__",     1, 0 },
    { "__band__",   1, 0 },
    { "__bxor__",   1, 0 },
    { "__bor__",    1, 0 }
};

SpecialSelector Spk_operCallSelectors[NUM_CALL_OPER] = {
    { "__apply__",  0, 0 },
    { "__index__",  0, 0 }
};


/*------------------------------------------------------------------------*/
/* methods */

static Object *Behavior_print(Object *self, Object *arg0, Object *arg1) {
    printf("<Behavior object at %p>", self);
    return Spk_void;
}


/*------------------------------------------------------------------------*/
/* class template */

static SpkAccessorTmpl accessors[] = {
    { "superclass", Spk_T_OBJECT, offsetof(Behavior, superclass), SpkAccessor_READ },
    { 0, 0, 0, 0 }
};

static SpkMethodTmpl methods[] = {
    { "print", SpkNativeCode_METH_ATTR | SpkNativeCode_ARGS_0, &Behavior_print },
    { 0, 0, 0}
};

SpkClassTmpl Spk_ClassBehaviorTmpl = {
    "Behavior",
    offsetof(BehaviorSubclass, variables),
    sizeof(Behavior),
    0,
    accessors,
    methods
};


/*------------------------------------------------------------------------*/
/* C API */

Behavior *SpkBehavior_new(void) {
    Behavior *newBehavior;
    
    newBehavior = (Behavior *)malloc(sizeof(Behavior));
    newBehavior->base.klass = Spk_ClassBehavior;
    return newBehavior;
}
    
Behavior *SpkBehavior_fromTemplate(SpkClassTmpl *template, Behavior *superclass, Module *module) {
    Behavior *newBehavior;
    
    newBehavior = SpkBehavior_new();
    SpkBehavior_initFromTemplate(newBehavior, template, superclass, module);
    return newBehavior;
}

void SpkBehavior_init(Behavior *self, Behavior *superclass, Module *module, size_t instVarCount) {
    size_t i;
    MethodNamespace namespace;
    
    self->superclass = superclass;
    self->module = module;
    self->ns[METHOD_NAMESPACE_RVALUE].methodDict = SpkIdentityDictionary_new();
    self->ns[METHOD_NAMESPACE_LVALUE].methodDict = SpkIdentityDictionary_new();
    
    if (superclass) {
        
        SpkBehavior_inheritOperators(self);
        
        /* low-level hooks */
        self->zero = superclass->zero;
        self->dealloc = superclass->dealloc;
        
        /* memory layout of instances */
        self->traverse = superclass->traverse;
        self->instVarCount = instVarCount;
        self->instVarOffset = superclass->instanceSize;
        self->instanceSize = self->instVarOffset + self->instVarCount*sizeof(Object *);
        self->itemSize = superclass->itemSize;
        
    } else {
        
        for (namespace = 0; namespace < NUM_METHOD_NAMESPACES; ++namespace) {
            for (i = 0; i < NUM_OPER; ++i) {
                self->ns[namespace].operTable[i].method = 0;
                self->ns[namespace].operTable[i].methodClass = 0;
            }
            for (i = 0; i < NUM_CALL_OPER; ++i) {
                self->ns[namespace].operCallTable[i].method = 0;
                self->ns[namespace].operCallTable[i].methodClass = 0;
            }
        }
        
        /* low-level hooks */
        self->zero = 0;
        self->dealloc = 0;
        
        /* memory layout of instances */
        self->traverse.init = 0;
        self->traverse.current = 0;
        self->traverse.next = 0;
        self->instVarCount = instVarCount;
        self->instVarOffset = offsetof(ObjectSubclass, variables);
        self->instanceSize = self->instVarOffset + self->instVarCount*sizeof(Object *);
        self->itemSize = 0;
        
    }
    
    /* static chain */
    self->outer = 0;
    self->outerClass = 0;
        
    /* temporary */
    self->next = 0;
    self->nextInScope = 0;
    self->nestedClassList.first = 0;
    self->nestedClassList.last = 0;
    
    return;
}

void SpkBehavior_initFromTemplate(Behavior *self, SpkClassTmpl *template, Behavior *superclass, Module *module) {
    
    assert(Spk_operSelectors[0].messageSelector && "use of 'initFromTemplate' cannot precede initialization of class Symbol");
    
    SpkBehavior_init(self, superclass, module, 0);
    
    if (template->zero)
        self->zero = template->zero;
    if (template->dealloc)
        self->dealloc = template->dealloc;
    if (template->traverse)
        self->traverse = *template->traverse;
    self->instVarOffset = template->instVarOffset;
    self->instanceSize = template->instanceSize;
    self->itemSize = template->itemSize;
}

void SpkBehavior_addMethodsFromTemplate(Behavior *self, SpkClassTmpl *template) {
    
    if (template->accessors) {
        SpkAccessorTmpl *accessorTmpl;
        
        for (accessorTmpl = template->accessors; accessorTmpl->name; ++accessorTmpl) {
            Symbol *messageSelector;
            Method *method;
        
            messageSelector = SpkSymbol_get(accessorTmpl->name);
            if (accessorTmpl->flags & SpkAccessor_READ) {
                method = Spk_newNativeReadAccessor(accessorTmpl->type, accessorTmpl->offset);
                SpkBehavior_insertMethod(self, METHOD_NAMESPACE_RVALUE, messageSelector, method);
            }
            
            if (accessorTmpl->flags & SpkAccessor_WRITE) {
                method = Spk_newNativeWriteAccessor(accessorTmpl->type, accessorTmpl->offset);
                SpkBehavior_insertMethod(self, METHOD_NAMESPACE_LVALUE, messageSelector, method);
            }
        }
    }
    
    if (template->methods) {
        SpkMethodTmpl *methodTmpl;
        
        for (methodTmpl = template->methods; methodTmpl->name; ++methodTmpl) {
            Symbol *messageSelector;
            Method *method;
            
            messageSelector = SpkSymbol_get(methodTmpl->name);
            method = Spk_newNativeMethod(methodTmpl->flags, methodTmpl->code);
            SpkBehavior_insertMethod(self, METHOD_NAMESPACE_RVALUE, messageSelector, method);
        }
    }
    if (template->lvalueMethods) {
        SpkMethodTmpl *methodTmpl;
        
        for (methodTmpl = template->lvalueMethods; methodTmpl->name; ++methodTmpl) {
            Symbol *messageSelector;
            Method *method;
            
            messageSelector = SpkSymbol_get(methodTmpl->name);
            method = Spk_newNativeMethod(methodTmpl->flags, methodTmpl->code);
            SpkBehavior_insertMethod(self, METHOD_NAMESPACE_LVALUE, messageSelector, method);
        }
    }
}

static void maybeAccelerateMethod(Behavior *self, MethodNamespace namespace, Symbol *messageSelector, Method *method) {    
    char *name;
    Oper operator;
    
    name = messageSelector->str;
    if (name[0] == '_' && name[1] == '_') {
        /* special selector */
        for (operator = 0; operator < NUM_CALL_OPER; ++operator) {
            if (messageSelector == Spk_operCallSelectors[operator].messageSelector) {
                self->ns[namespace].operCallTable[operator].method = method;
                self->ns[namespace].operCallTable[operator].methodClass = self;
                break;
            }
        }
        if (operator >= NUM_CALL_OPER) {
            for (operator = 0; operator < NUM_OPER; ++operator) {
                if (messageSelector == Spk_operSelectors[operator].messageSelector) {
                    self->ns[namespace].operTable[operator].method = method;
                    self->ns[namespace].operTable[operator].methodClass = self;
                    break;
                }
            }
            assert(operator < NUM_OPER);
        }
    }
}

void SpkBehavior_inheritOperators(Behavior *self) {
    size_t i;
    MethodNamespace namespace;
    Behavior *superclass;
    
    superclass = self->superclass;
    if (!superclass)
        return;
    
    for (namespace = 0; namespace < NUM_METHOD_NAMESPACES; ++namespace) {
        for (i = 0; i < NUM_OPER; ++i) {
            self->ns[namespace].operTable[i].method = superclass->ns[namespace].operTable[i].method;
            self->ns[namespace].operTable[i].methodClass = superclass->ns[namespace].operTable[i].methodClass;
        }
        for (i = 0; i < NUM_CALL_OPER; ++i) {
            self->ns[namespace].operCallTable[i].method = superclass->ns[namespace].operCallTable[i].method;
            self->ns[namespace].operCallTable[i].methodClass = superclass->ns[namespace].operCallTable[i].methodClass;
        }
    }
    
    /* re-insert our own methods */
    for (namespace = 0; namespace < NUM_METHOD_NAMESPACES; ++namespace) {
        IdentityDictionary *methodDict = self->ns[namespace].methodDict;
        for (i = 0; i < methodDict->size; ++i) {
            Method *method = (Method *)methodDict->valueArray[i];;
            if (method) {
                Symbol *messageSelector = (Symbol *)methodDict->keyArray[i];
                maybeAccelerateMethod(self, namespace, messageSelector, method);
            }
        }
    }
}

void SpkBehavior_insertMethod(Behavior *self, MethodNamespace namespace, Symbol *messageSelector, Method *method) {
    SpkIdentityDictionary_atPut(self->ns[namespace].methodDict, (Object *)messageSelector, (Object *)method);
    maybeAccelerateMethod(self, namespace, messageSelector, method);
}

Method *SpkBehavior_lookupMethod(Behavior *self, MethodNamespace namespace, Symbol *messageSelector) {
    return (Method *)SpkIdentityDictionary_at(self->ns[namespace].methodDict, (Object *)messageSelector);
}

Symbol *SpkBehavior_findSelectorOfMethod(Behavior *self, Method *meth) {
    MethodNamespace namespace;
    Symbol *selector;
    
    for (namespace = 0; namespace < NUM_METHOD_NAMESPACES; ++namespace) {
        selector = (Symbol *)SpkIdentityDictionary_keyAtValue(self->ns[namespace].methodDict, (Object *)meth);
        if (selector)
            return selector;
    }
    return 0;
}

char *SpkBehavior_name(Behavior *self) {
    Class *c;
    
    c = Spk_CAST(Class, self);
    if (c) {
        return c->name->str;
    }
    return "<unknown>";
}
