
#include "behavior.h"

#include "dict.h"
#include "interp.h"
#include "module.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


Behavior *ClassBehavior;


SpecialSelector specialSelectors[NUM_OPER] = {
    { "__item__",   1, 0 }, /* XXX: this is two operators */
    { "__addr__",   0, 0 }, /* "&x" -- not implemented */
    { "__ind__",    0, 0 }, /* "*x" -- not implemented */
    { "__pos__",    0, 0 },
    { "__neg__",    0, 0 },
    { "__bneg__",   0, 0 },
    { "__not__",    0, 0 },
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
    { "__bor__",    1, 0 },
};


/*------------------------------------------------------------------------*/
/* methods */

static Object *Behavior_new(Object *_self, Object *arg0, Object *arg1) {
    /* Answer a new instance of the receiver. */
    Behavior *self;
    ObjectSubclass *newObject;
    size_t i;
    
    self = (Behavior *)_self;
    newObject = (ObjectSubclass *)malloc(self->instanceSize);
    newObject->base.refCount = 1;
    newObject->base.klass = self;
    for (i = 0; i < self->instVarCount; ++i) {
        newObject->variables[i] = 0;
    }
    return (Object *)newObject;
}

static Object *Behavior_print(Object *self, Object *arg0, Object *arg1) {
    printf("<Behavior object at %p>", self);
}


/*------------------------------------------------------------------------*/
/* class template */

static SpkMethodTmpl methods[] = {
    { "new",   SpkNativeCode_ARGS_0 | SpkNativeCode_CALLABLE, &Behavior_new   },
    { "print", SpkNativeCode_ARGS_0 | SpkNativeCode_CALLABLE, &Behavior_print },
    { 0, 0, 0}
};

static SpkClassTmpl tmpl = {
    offsetof(BehaviorSubclass, variables),
    sizeof(Behavior),
    0,
    methods
};


/*------------------------------------------------------------------------*/
/* C API */

void SpkClassBehavior_init(void) {
    ClassBehavior = SpkBehavior_new(ClassObject, 0 /*builtInModule*/, 0);
    
    /* Behavior is an instance of itself. */
    ClassBehavior->base.klass = ClassBehavior;
}

void SpkClassBehavior_init2(void) {
    Oper operator;
    
    for (operator = 0; operator < NUM_OPER; ++operator) {
        specialSelectors[operator].messageSelector = SpkSymbol_get(specialSelectors[operator].messageSelectorStr);
    }
    
    ClassBehavior->module = builtInModule;
    ClassBehavior->methodDict->base.klass = ClassIdentityDictionary;
    SpkBehavior_initFromTemplate(ClassBehavior, &tmpl);
}

Behavior *SpkBehavior_new(Behavior *superclass, struct Module *module, size_t instVarCount) {
    Behavior *newBehavior;
    size_t i;
    
    newBehavior = (Behavior *)malloc(sizeof(Behavior));
    newBehavior->base.klass = ClassBehavior;
    newBehavior->superclass = superclass;
    newBehavior->module = module;
    newBehavior->methodDict = SpkIdentityDictionary_new();
    
    if (superclass) {
        for (i = 0; i < sizeof(newBehavior->operTable)/sizeof(newBehavior->operTable[0]); ++i) {
            newBehavior->operTable[i].method = superclass->operTable[i].method;
            newBehavior->operTable[i].methodClass = superclass->operTable[i].methodClass;
        }
        newBehavior->operCall.method = superclass->operCall.method;
        newBehavior->operCall.methodClass = superclass->operCall.methodClass;
    } else {
        for (i = 0; i < sizeof(newBehavior->operTable)/sizeof(newBehavior->operTable[0]); ++i) {
            newBehavior->operTable[i].method = 0;
            newBehavior->operTable[i].methodClass = 0;
        }
        newBehavior->operCall.method = 0;
        newBehavior->operCall.methodClass = 0;
    }
    
    /* temporary */
    newBehavior->next = 0;
    
    /* memory layout of instances */
    newBehavior->instVarCount = instVarCount;
    newBehavior->instVarOffset = offsetof(ObjectSubclass, variables);
    newBehavior->instanceSize = newBehavior->instVarOffset + instVarCount*sizeof(Object *);
    
    return newBehavior;
}

Behavior *SpkBehavior_fromTemplate(SpkClassTmpl *template, Behavior *superclass, struct Module *module) {
    Behavior *newBehavior;
    
    newBehavior = SpkBehavior_new(superclass, module, 0);
    SpkBehavior_initFromTemplate(newBehavior, template);
    return newBehavior;
}

void SpkBehavior_initFromTemplate(Behavior *self, SpkClassTmpl *template) {
    SpkMethodTmpl *methodTmpl;
    
    assert(specialSelectors[0].messageSelector && "use of 'initFromTemplate' cannot precede initialization of class Symbol");
    
    self->instVarOffset = template->instVarOffset;
    self->instanceSize = template->instanceSize;
    
    for (methodTmpl = template->methods; methodTmpl->name; ++methodTmpl) {
        Symbol *messageSelector;
        Method *method;
        
        messageSelector = SpkSymbol_get(methodTmpl->name);
        method = SpkMethod_newNative(methodTmpl->flags, methodTmpl->code);
        SpkBehavior_insertMethod(self, messageSelector, method);
        
        if (methodTmpl->name[0] == '_' && methodTmpl->name[1] == '_') {
            /* special selector */
            if (strcmp(methodTmpl->name, "__call__") == 0) {
                self->operCall.method = method;
                self->operCall.methodClass = self;
            } else {
                Oper operator;
                
                for (operator = 0; operator < NUM_OPER; ++operator) {
                    if (messageSelector == specialSelectors[operator].messageSelector) {
                        self->operTable[operator].method = method;
                        self->operTable[operator].methodClass = self;
                        break;
                    }
                }
                assert(operator < NUM_OPER);
            }
        }
    }
}

void SpkBehavior_insertMethod(Behavior *self, Symbol *messageSelector, Method *method) {
    SpkIdentityDictionary_atPut(self->methodDict, (Object *)messageSelector, (Object *)method);
}

Method *SpkBehavior_lookupMethod(Behavior *self, Symbol *messageSelector) {
    return (Method *)SpkIdentityDictionary_at(self->methodDict, (Object *)messageSelector);
}

Symbol *SpkBehavior_findSelectorOfMethod(Behavior *self, Method *meth) {
    return (Symbol *)SpkIdentityDictionary_keyAtValue(self->methodDict, (Object *)meth);
}
