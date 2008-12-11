
#ifndef __array_h__
#define __array_h__


#include "obj.h"

#include <stdarg.h>


typedef VariableObject Array;
typedef VariableObjectSubclass ArraySubclass;


extern struct Behavior *ClassArray;
extern struct SpkClassTmpl ClassArrayTmpl;


Array *SpkArray_new(size_t);
Array *SpkArray_withArguments(Object **, size_t, Array *, size_t);
Array *SpkArray_fromVAList(va_list);
size_t SpkArray_size(Array *);
Object *SpkArray_item(Array *, long);


#endif /* __array_h__ */
