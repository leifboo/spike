
#include "rtl.h"


Method *SpikeLookupMethod(Behavior *behavior, unsigned int ns, Symbol *selector)
{
    Array *methodTable;
    struct Pair { Symbol *selector; Method *method; } *p, *end;
    
    methodTable = behavior->methodTable[ns];
    if (!methodTable)
        return 0;
    
    p = (struct Pair *)methodTable->item;
    end = p + methodTable->size / 2;
    
    for ( ; p < end; ++p)
        if (p->selector == selector)
            return p->method;
    
    return 0;
}

