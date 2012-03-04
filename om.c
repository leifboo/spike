
#include "om.h"

#include "behavior.h"
#include "native.h"
#include "obj.h"
#include <stdlib.h>


Unknown *ObjMem_Alloc(size_t size) {
    Unknown *op;
    
    op = (Unknown *)malloc(size);
    if (!op) {
        Halt(HALT_MEMORY_ERROR, "out of memory");
        return 0;
    }
    op->refCount = 1;
    return op;
}

void ObjMem_Dealloc(Unknown *current) {
    Unknown *l;
    
    l = 0;
    while (1) {
        (*((Object *)current)->klass->dealloc)((Object *)current, &l);
        free(current);
        current = l;
        if (!current)
            break;
        l = current->next;
    }
}
