
#ifndef __spk_om_h__
#define __spk_om_h__


#include <stddef.h>


typedef struct SpkUnknown {
    size_t refCount;
} SpkUnknown;


SpkUnknown *SpkObjMem_Alloc(size_t);
void SpkObjMem_Dealloc(SpkUnknown *);


#define Spk_INCREF(op) (((SpkUnknown *)(op))->refCount++)
#define Spk_DECREF(op) if (--((SpkUnknown *)(op))->refCount > 0) ; else SpkObjMem_Dealloc((SpkUnknown *)(op))
#define Spk_XINCREF(op) if (!(op)) ; else Spk_INCREF(op)
#define Spk_XDECREF(op) if (!(op)) ; else Spk_DECREF(op)

#define Spk_CLEAR(op) \
    do {                                             \
        if (op) {                                    \
            SpkUnknown *tmp = (SpkUnknown *)(op);    \
            (op) = NULL;                             \
            Spk_DECREF(tmp);                         \
        }                                            \
    } while (0)


#endif /* __spk_om_h__ */
