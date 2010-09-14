
#ifndef __spk_om_h__
#define __spk_om_h__


#include <stddef.h>


typedef union SpkUnknown {
    size_t refCount;
    union SpkUnknown *next;
} SpkUnknown;


SpkUnknown *SpkObjMem_Alloc(size_t);
void SpkObjMem_Dealloc(SpkUnknown *);


#define Spk_INCREF(op) (((SpkUnknown *)(op))->refCount++)
#define Spk_DECREF(op) if (--((SpkUnknown *)(op))->refCount > 0) ; else SpkObjMem_Dealloc((SpkUnknown *)(op))
#define Spk_LDECREF(op, l) if (--((SpkUnknown *)(op))->refCount > 0) ; else ((SpkUnknown *)(op))->next = *(l), *(l) = ((SpkUnknown *)(op));
#define Spk_XINCREF(op) if (!(op)) ; else Spk_INCREF(op)
#define Spk_XDECREF(op) if (!(op)) ; else Spk_DECREF(op)
#define Spk_XLDECREF(op, l) if (!(op)) ; else Spk_LDECREF(op, l)

#define Spk_CLEAR(op) \
    do {                                             \
        if (op) {                                    \
            SpkUnknown *tmp = (SpkUnknown *)(op);    \
            (op) = NULL;                             \
            Spk_DECREF(tmp);                         \
        }                                            \
    } while (0)


/* weak reference counting -- currently no-ops */
#define Spk_INCWREF(op)
#define Spk_DECWREF(op)


#endif /* __spk_om_h__ */
