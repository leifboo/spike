
#ifndef __spk_int_h__
#define __spk_int_h__


#include "obj.h"


typedef struct SpkInteger SpkInteger;


extern struct SpkBehavior *Spk_ClassInteger;
extern struct SpkClassTmpl Spk_ClassIntegerTmpl;


SpkInteger *SpkInteger_FromCLong(long);
SpkInteger *SpkInteger_FromCPtrdiff(ptrdiff_t);
long SpkInteger_AsCLong(SpkInteger *);
ptrdiff_t SpkInteger_AsCPtrdiff(SpkInteger *);


#endif /* __spk_int_h__ */
