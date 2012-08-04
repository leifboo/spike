
#include "rtl.h"

#include <stdio.h>


struct Class {
    struct Behavior base;
    struct Symbol *name;
};


extern struct Behavior Class;


static const char *behaviorNameAsCString(struct Behavior *behavior) {
    struct Class *c = CAST(Class, behavior);
    return c ? c->name->str : "??";
}


/* XXX: This could be written in Spike. */

void SpikePrintStackTrace(struct Context *activeContext, FILE *out) {
    struct Context *context, *home;
    struct Method *method;
    struct Behavior *methodClass;
    struct Symbol *methodSel;
    
    for (context = activeContext; context; context = context->caller) {
        home = context->homeContext;
        method = home->method;
        methodClass = home->methodClass;
        
        /* XXX: Print global function names? */
        if (method && methodClass)
            methodSel = SpikeFindSelectorOfMethod(methodClass, method);
        else
            methodSel = 0;
        
        fprintf(out,
                "%p %p%s%s::%s\n",
                context,
                context->pc,
                (context == home ? " " : " [] in "),
                behaviorNameAsCString(methodClass),
                methodSel ? methodSel->str : "??");
    }
}

