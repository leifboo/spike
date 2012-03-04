
#ifndef __float_h__
#define __float_h__


#include "obj.h"


typedef struct Float Float;


extern struct ClassTmpl ClassFloatTmpl;


Float *Float_FromCDouble(double);
double Float_AsCDouble(Float *);


#endif /* __float_h__ */
