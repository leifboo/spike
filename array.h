
#ifndef __array_h__
#define __array_h__


#include "obj.h"

#include <stdarg.h>


typedef VariableObject Array;


extern struct ClassTmpl ClassArrayTmpl;


Array *Array_New(size_t);
Array *Array_WithArguments(Unknown **, size_t, Array *, size_t);
Array *Array_FromVAList(va_list);
size_t Array_Size(Array *);
Unknown *Array_GetItem(Array *, size_t);
Unknown *Array_SetItem(Array *, size_t, Unknown *);
void Array_Sort(Array *, int (*)(Unknown *const *, Unknown *const *));


#endif /* __array_h__ */
