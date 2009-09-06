
#ifndef __spk_array_h__
#define __spk_array_h__


#include "obj.h"

#include <stdarg.h>


typedef SpkVariableObject SpkArray;


extern struct SpkBehavior *Spk_ClassArray;
extern struct SpkClassTmpl Spk_ClassArrayTmpl;


SpkArray *SpkArray_New(size_t);
SpkArray *SpkArray_WithArguments(SpkUnknown **, size_t, SpkArray *, size_t);
SpkArray *SpkArray_FromVAList(va_list);
size_t SpkArray_Size(SpkArray *);
SpkUnknown *SpkArray_GetItem(SpkArray *, size_t);
SpkUnknown *SpkArray_SetItem(SpkArray *, size_t, SpkUnknown *);


#endif /* __spk_array_h__ */
