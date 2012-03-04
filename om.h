
#ifndef __om_h__
#define __om_h__


#include <stddef.h>


typedef union Unknown {
    size_t refCount;
    union Unknown *next;
} Unknown;


Unknown *ObjMem_Alloc(size_t);
void ObjMem_Dealloc(Unknown *);


#define INCREF(op) (((Unknown *)(op))->refCount++)
#define DECREF(op) if (--((Unknown *)(op))->refCount > 0) ; else ObjMem_Dealloc((Unknown *)(op))
#define LDECREF(op, l) if (--((Unknown *)(op))->refCount > 0) ; else ((Unknown *)(op))->next = *(l), *(l) = ((Unknown *)(op));
#define XINCREF(op) if (!(op)) ; else INCREF(op)
#define XDECREF(op) if (!(op)) ; else DECREF(op)
#define XLDECREF(op, l) if (!(op)) ; else LDECREF(op, l)

#define CLEAR(op) \
    do {                                             \
        if (op) {                                    \
            Unknown *tmp = (Unknown *)(op);    \
            (op) = NULL;                             \
            DECREF(tmp);                         \
        }                                            \
    } while (0)


/* weak reference counting -- currently no-ops */
#define INCWREF(op)
#define DECWREF(op)


#endif /* __om_h__ */
