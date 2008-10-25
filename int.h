
#ifndef __int_h__
#define __int_h__


#include "obj.h"


typedef struct Integer {
    Object base;
    long int value;
} Integer;

typedef struct IntegerSubclass {
    Integer base;
    Object *variables[1]; /* stretchy */
} IntegerSubclass;


extern struct Behavior *ClassInteger;


void SpkClassInteger_init(void);
Integer *SpkInteger_fromLong(long);
long SpkInteger_asLong(Integer *);


#endif /* __int_h__ */
