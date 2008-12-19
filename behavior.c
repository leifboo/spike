
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
    { "__apply__",     0, 0 },
    { "__item__",      0, 0 },
    { "__setItem__",   0, 0 }
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
    
    self->superclass = superclass;
    self->module = module;
    self->methodDict = SpkIdentityDictionary_new();
    
    if (superclass) {
        
        for (i = 0; i < NUM_OPER; ++i) {
            self->operTable[i].method = superclass->operTable[i].method;
            self->operTable[i].methodClass = superclass->operTable[i].methodClass;
        }
        for (i = 0; i < NUM_CALL_OPER; ++i) {
            self->operCallTable[i].method = superclass->operCallTable[i].method;
            self->operCallTable[i].methodClass = superclass->operCallTable[i].methodClass;
        }
        
        /* memory layout of instances */
        self->traverse = superclass->traverse;
        self->instVarCount = instVarCount;
        self->instVarOffset = superclass->instanceSize;
        self->instanceSize = self->instVarOffset + self->instVarCount*sizeof(Object *);
        self->itemSize = superclass->itemSize;
        
    } else {
        
        for (i = 0; i < NUM_OPER; ++i) {
            self->operTable[i].method = 0;
            self->operTable[i].methodClass = 0;
        }
        for (i = 0; i < NUM_CALL_OPER; ++i) {
            self->operCallTable[i].method = 0;
            self->operCallTable[i].methodClass = 0;
        }
        
        /* memory layout of instances */
        self->traverse.init = 0;
        self->traverse.current = 0;
        self->traverse.next = 0;
        self->instVarCount = instVarCount;
        self->instVarOffset = offsetof(ObjectSubclass, variables);
        self->instanceSize = self->instVarOffset + self->instVarCount*sizeof(Object *);
        self->itemSize = 0;
        
    }
    
    /* temporary */
    self->next = 0;
    
    return;
}

void SpkBehavior_initFromTemplate(Behavior *self, SpkClassTmpl *template, Behavior *superclass, Module *module) {
    
    assert(Spk_operSelectors[0].messageSelector && "use of 'initFromTemplate' cannot precede initialization of class Symbol");
    
    SpkBehavior_init(self, superclass, module, 0);
    
    if (template->traverse) {
        self->traverse = *template->traverse;
    }
    self->instVarOffset = template->instVarOffset;
    self->instanceSize = template->instanceSize;
    self->itemSize = template->itemSize;
}

void SpkBehavior_addMethodsFromTemplate(Behavior *self, SpkClassTmpl *template) {
    
    if (template->accessors) {
        SpkAccessorTmpl *accessorTmpl;
        
        for (accessorTmpl = template->accessors; accessorTmpl->name; ++accessorTmpl) {
            Symbol *name, *messageSelector;
            Method *method;
        
            name = SpkSymbol_get(accessorTmpl->name);
            if (accessorTmpl->flags & SpkAccessor_READ) {
                messageSelector = name;
                method = Spk_newNativeReadAccessor(accessorTmpl->type, accessorTmpl->offset);
                SpkBehavior_insertMethod(self, messageSelector, method);
            }
            
            if (accessorTmpl->flags & SpkAccessor_WRITE) {
                messageSelector = SpkBehavior_mangledSetAccessorName(name);
                method = Spk_newNativeWriteAccessor(accessorTmpl->type, accessorTmpl->offset);
                SpkBehavior_insertMethod(self, messageSelector, method);
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
            SpkBehavior_insertMethod(self, messageSelector, method);
        }
    }
}

void SpkBehavior_insertMethod(Behavior *self, Symbol *messageSelector, Method *method) {
    char *name;
    Oper operator;
    
    SpkIdentityDictionary_atPut(self->methodDict, (Object *)messageSelector, (Object *)method);
    
    name = messageSelector->str;
    if (name[0] == '_' && name[1] == '_' && strncmp(name, "__set__", 7) != 0) {
        /* special selector */
        for (operator = 0; operator < NUM_CALL_OPER; ++operator) {
            if (messageSelector == Spk_operCallSelectors[operator].messageSelector) {
                self->operCallTable[operator].method = method;
                self->operCallTable[operator].methodClass = self;
                break;
            }
        }
        if (operator >= NUM_CALL_OPER) {
            for (operator = 0; operator < NUM_OPER; ++operator) {
                if (messageSelector == Spk_operSelectors[operator].messageSelector) {
                    self->operTable[operator].method = method;
                    self->operTable[operator].methodClass = self;
                    break;
                }
            }
            assert(operator < NUM_OPER);
        }
    }
}

Method *SpkBehavior_lookupMethod(Behavior *self, Symbol *messageSelector) {
    return (Method *)SpkIdentityDictionary_at(self->methodDict, (Object *)messageSelector);
}

Symbol *SpkBehavior_findSelectorOfMethod(Behavior *self, Method *meth) {
    return (Symbol *)SpkIdentityDictionary_keyAtValue(self->methodDict, (Object *)meth);
}

char *SpkBehavior_name(Behavior *self) {
    Class *c;
    
    c = Spk_CAST(Class, self);
    if (c) {
        return c->name->str;
    }
    return "<unknown>";
}

Symbol *SpkBehavior_mangledSetAccessorName(Symbol *orig) {
    static const char *mangle = "__set__%s";
    size_t len;
    char *buffer;
    Symbol *mangled;
    
    len = strlen(mangle) - 2 + strlen(orig->str);
    buffer = (char *)malloc(len + 1);
    sprintf(buffer, mangle, orig->str);
    mangled = SpkSymbol_get(buffer);
    free(buffer);
    return mangled;
}
