
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
    return op;
}

void ObjMem_Dealloc(Unknown *current) {
    Unknown *l = 0;
    (*((Object *)current)->klass->dealloc)((Object *)current, &l);
    free(current);
}
