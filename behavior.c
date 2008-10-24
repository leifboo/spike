
#include "behavior.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dict.h"
#include "interp.h"


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


static Object *print(Object *self) {
    printf("an Object at %p\n", self);
}

static void printName(void) {
    printf("a class");
}


void SpkClassBehavior_init(void) {
}

void SpkClassBehavior_init2(void) {
    Oper operator;
    
    for (operator = 0; operator < NUM_OPER; ++operator) {
        specialSelectors[operator].messageSelector = SpkSymbol_get(specialSelectors[operator].messageSelectorStr);
    }
}

Behavior *SpkBehavior_new(Behavior *superclass, struct Module *module, size_t instVarCount) {
    Behavior *newBehavior;
    size_t i;
    
    newBehavior = (Behavior *)malloc(sizeof(Behavior));
    newBehavior->superclass = superclass;
    newBehavior->module = module;
    newBehavior->methodDict = SpkIdentityDictionary_new();
    
    for (i = 0; i < sizeof(newBehavior->operTable)/sizeof(newBehavior->operTable[0]); ++i) {
        newBehavior->operTable[i].method = 0;
        newBehavior->operTable[i].methodClass = 0;
    }
    newBehavior->operCall.method = 0;
    newBehavior->operCall.methodClass = 0;
    
    /* temporary */
    newBehavior->next = 0;
    newBehavior->printName = &printName;
    newBehavior->print = &print;
    
    /* memory layout of instances */
    newBehavior->instVarOffset = offsetof(ObjectSubclass, variables);
    newBehavior->instanceSize = newBehavior->instVarOffset + instVarCount*sizeof(Object *);
    
    return newBehavior;
}

Behavior *SpkBehavior_fromTemplate(SpkClassTmpl *template, Behavior *superclass, struct Module *module) {
    Behavior *newBehavior;
    SpkMethodTmpl *methodTmpl;
    
    newBehavior = SpkBehavior_new(superclass, module, 0);
    
    for (methodTmpl = template->methods; methodTmpl->name; ++methodTmpl) {
        Symbol *messageSelector;
        Method *method;
        
        messageSelector = SpkSymbol_get(methodTmpl->name);
        method = SpkMethod_newNative(methodTmpl->flags, methodTmpl->code);
        SpkBehavior_insertMethod(newBehavior, messageSelector, method);
        
        if (methodTmpl->name[0] == '_' && methodTmpl->name[1] == '_') {
            /* special selector */
            if (strcmp(methodTmpl->name, "__call__") == 0) {
                newBehavior->operCall.method = method;
                newBehavior->operCall.methodClass = newBehavior;
            } else {
                Oper operator;
                
                for (operator = 0; operator < NUM_OPER; ++operator) {
                    if (messageSelector == specialSelectors[operator].messageSelector) {
                        newBehavior->operTable[operator].method = method;
                        newBehavior->operTable[operator].methodClass = newBehavior;
                        break;
                    }
                }
                assert(operator < NUM_OPER);
            }
        }
    }
    
    return newBehavior;
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
