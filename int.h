
#ifndef __int_h__
#define __int_h__


#include "interp.h"


typedef struct Integer {
    Object base;
    long int value;
} Integer;

typedef struct IntegerSubclass {
    Integer base;
    Object *variables[1]; /* stretchy */
} IntegerSubclass;


void SpkClassInteger_init(void);
Integer *SpkInteger_fromLong(long);


#endif /* __int_h__ */
