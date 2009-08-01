
#ifndef __spk_int_h__
#define __spk_int_h__


#include "obj.h"


typedef struct SpkInteger {
    SpkObject base;
    long int value;
} SpkInteger;

typedef struct SpkIntegerSubclass {
    SpkInteger base;
    SpkUnknown *variables[1]; /* stretchy */
} SpkIntegerSubclass;


extern struct SpkBehavior *Spk_ClassInteger;
extern struct SpkClassTmpl Spk_ClassIntegerTmpl;


SpkInteger *SpkInteger_fromLong(long);
long SpkInteger_asLong(SpkInteger *);


#endif /* __spk_int_h__ */
