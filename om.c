
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

void SpkObjMem_Dealloc(SpkUnknown *op) {
    /* cf. "A Space-efficient Reference-counting Collector", pp. 678-681 */
    SpkObject *prior, *current, *klass;
    SpkObject **var;
    SpkObject sentinel;
    
    /* Using a sentinel instead of null makes SpkTraverse hooks easier
       to write. */
    sentinel.klass = 0;
    prior = &sentinel;
    current = (SpkObject *)op;
 dealloc:
    (*current->klass->traverse.init)(current);
    while (1) {
        while ((var = (SpkObject **)(*current->klass->traverse.current)(current))) {
            if (*var && --(*var)->base.refCount == 0) {
                SpkObject *tmp = *var;
                *var = prior;
                prior = current;
                current = tmp;
                goto dealloc;
            }
            (*current->klass->traverse.next)(current);
        }
        (*current->klass->dealloc)(current);
        klass = (SpkObject *)current->klass;
        free(current);
        if (--klass->base.refCount == 0) {
            current = klass;
            goto dealloc;
        }
        current = prior;
        if (current == &sentinel)
            break;
        /* resume where we left off */
        var = (SpkObject **)(*current->klass->traverse.current)(current);
        prior = *var;
        (*current->klass->traverse.next)(current);
    }
}
