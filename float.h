
#ifndef __spk_float_h__
#define __spk_float_h__


#include "obj.h"


typedef struct SpkFloat SpkFloat;


extern struct SpkBehavior *Spk_ClassFloat;
extern struct SpkClassTmpl Spk_ClassFloatTmpl;


SpkFloat *SpkFloat_FromCDouble(double);
double SpkFloat_AsCDouble(SpkFloat *);


#endif /* __spk_float_h__ */
