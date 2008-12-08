
#ifndef __float_h__
#define __float_h__


#include "obj.h"


typedef struct Float {
    Object base;
    double value;
} Float;

typedef struct FloatSubclass {
    Float base;
    Object *variables[1]; /* stretchy */
} FloatSubclass;


extern struct Behavior *ClassFloat;
extern struct SpkClassTmpl ClassFloatTmpl;


Float *SpkFloat_fromLiteral(char *, size_t);


#endif /* __float_h__ */
