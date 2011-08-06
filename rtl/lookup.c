
#include "rtl.h"

#include <stdio.h>


extern Behavior Metaclass;


#define LOOKUP_DEBUG 0


Method *SpikeLookupMethod(Behavior *behavior, unsigned int ns, Symbol *selector)
{
    Array *methodTable;
    struct Pair { Symbol *selector; Method *method; } *p, *end;
    
#if LOOKUP_DEBUG
    fprintf(stderr,
            "@@@ lookup %s in %s\n",
            (char *)selector->str,
            behavior->base.klass == &Metaclass
            ? "metaclass"
            : (*((Symbol **)(behavior + 1)))->str);
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

