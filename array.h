
#ifndef __spk_array_h__
#define __spk_array_h__


#include "obj.h"

#include <stdarg.h>


typedef SpkVariableObject SpkArray;
typedef SpkVariableObjectSubclass SpkArraySubclass;


extern struct SpkBehavior *Spk_ClassArray;
extern struct SpkClassTmpl Spk_ClassArrayTmpl;


SpkArray *SpkArray_new(size_t);
SpkArray *SpkArray_withArguments(SpkUnknown **, size_t, SpkArray *, size_t);
SpkArray *SpkArray_fromVAList(va_list);
size_t SpkArray_size(SpkArray *);
SpkUnknown *SpkArray_item(SpkArray *, long);
SpkUnknown *SpkArray_setItem(SpkArray *, long, SpkUnknown *);


#endif /* __spk_array_h__ */
