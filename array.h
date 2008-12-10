
#ifndef __array_h__
#define __array_h__


#include "obj.h"


typedef VariableObject Array;
typedef VariableObjectSubclass ArraySubclass;


extern struct Behavior *ClassArray;
extern struct SpkClassTmpl ClassArrayTmpl;


Array *SpkArray_withArguments(Object **, size_t, Array *, size_t);
size_t SpkArray_size(Array *);
Object *SpkArray_item(Array *, long);


#endif /* __array_h__ */
