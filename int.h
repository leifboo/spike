
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


extern struct Behavior *Spk_ClassInteger;
extern struct SpkClassTmpl Spk_ClassIntegerTmpl;


Integer *SpkInteger_fromLong(long);
long SpkInteger_asLong(Integer *);


#endif /* __int_h__ */
