
#ifndef __om_h__
#define __om_h__


#include <stddef.h>


typedef struct Unknown {
} Unknown;


Unknown *ObjMem_Alloc(size_t);
void ObjMem_Dealloc(Unknown *);


#endif /* __om_h__ */
