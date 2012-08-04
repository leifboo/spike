
#include "rtl.h"

#include <stdio.h>
#include <stdlib.h>


struct Pair { struct Symbol *selector; struct Method *method; };


extern struct Behavior Array, Message, Metaclass;


struct Method *SpikeLookupMethod(
    struct Behavior *behavior,
    unsigned int ns,
    struct Symbol *selector
    )
{
    struct Array *methodTable;
    struct Pair *p, *end;
    
#if LOOKUP_DEBUG
    fprintf(stderr,
            "@@@ lookup %s in %s\n",
            (char *)selector->str,
            behavior->base.klass == &Metaclass
            ? "metaclass"
            : (*((struct Symbol **)(behavior + 1)))->str);
#endif
    
    methodTable = behavior->methodTable[ns];
    
#if LOOKUP_DEBUG
    if (!methodTable) {
        fprintf(stderr, "@@@ no table\n");
        return 0;
    }
#else
    if (!methodTable)
        return 0;
#endif
    
    p = (struct Pair *)methodTable->item;
    end = p + methodTable->size / 2;
    
#if LOOKUP_DEBUG
    fprintf(stderr, "@@@ %lu entries\n", (unsigned long)(end - p));
    for ( ; p < end; ++p) {
        if (p->selector == selector) {
            fprintf(stderr, "@@@ found %s\n", (char *)selector->str);
            return p->method;
        }
        fprintf(stderr, "\t@@@ %s != %s\n", (char *)p->selector->str, (char *)selector->str);
    }
#else
    for ( ; p < end; ++p)
        if (p->selector == selector)
            return p->method;
#endif
    
    return 0;
}


struct Symbol *SpikeFindSelectorOfMethod(
    struct Behavior *behavior,
    struct Method *method
    )
{
    struct Array *methodTable;
    struct Pair *p, *end;
    int ns;
    
    for (ns = 0; ns < 2 /*NUM_METHOD_NAMESPACES*/; ++ns) {
        
        methodTable = behavior->methodTable[ns];
        if (!methodTable)
            continue;
        
        p = (struct Pair *)methodTable->item;
        end = p + methodTable->size / 2;
        
        for ( ; p < end; ++p)
            if (p->method == method)
                return p->selector;
    }
    
    return 0;
}


struct Object *SpikeCast(struct Behavior *target, struct Object *object) {
    struct Behavior *c;
    
    if (!object)
        return 0;
    
    for (c = object->klass; c; c = c->superclass) {
        if (c == target)
            return object;
    }
    
    return 0;
}


struct Message *SpikeCreateActualMessage(
    unsigned int ns,
    struct Symbol *selector,
    size_t argumentCount,
    struct Object **arg
    )
{
    struct Array *arguments;
    struct Message *message;
    size_t i;
    
    arguments = (struct Array *)calloc(1, offsetof(struct Array, item[argumentCount]));
    message = (struct Message *)calloc(1, sizeof(struct Message));
    
    arguments->base.klass = &Array;
    arguments->size = argumentCount;
    
    /* copy & reverse arguments from stack */
    for (i = 0; i < argumentCount; ++i)
        arguments->item[i] = arg[argumentCount - i - 1];
    
    message->base.klass = &Message;
    message->ns = (ns << 2) | 2; /* box */
    message->selector = selector;
    message->arguments = arguments;
    
    return message;
}

