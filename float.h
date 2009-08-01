
#ifndef __spk_float_h__
#define __spk_float_h__


#include "obj.h"


typedef struct SpkFloat {
    SpkObject base;
    double value;
} SpkFloat;

typedef struct SpkFloatSubclass {
    SpkFloat base;
    SpkUnknown *variables[1]; /* stretchy */
} SpkFloatSubclass;


extern struct SpkBehavior *Spk_ClassFloat;
extern struct SpkClassTmpl Spk_ClassFloatTmpl;


SpkFloat *SpkFloat_fromCDouble(double);
double SpkFloat_asDouble(SpkFloat *);


#endif /* __spk_float_h__ */
