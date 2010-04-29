
#include "om.h"

#include "behavior.h"
#include "native.h"
#include "obj.h"
#include <stdlib.h>


SpkUnknown *SpkObjMem_Alloc(size_t size) {
    SpkUnknown *op;
    
    op = (SpkUnknown *)malloc(size);
    if (!op) {
        Spk_Halt(Spk_HALT_MEMORY_ERROR, "out of memory");
        return 0;
    }
    op->refCount = 1;
    return op;
}

void SpkObjMem_Dealloc(SpkUnknown *current) {
    SpkUnknown *l;
    
    l = 0;
    while (1) {
        (*((SpkObject *)current)->klass->dealloc)((SpkObject *)current, &l);
        free(current);
        current = l;
        if (!current)
            break;
        l = current->next;
    }
}
