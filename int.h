
#ifndef __int_h__
#define __int_h__


#include "obj.h"


typedef struct Integer Integer;


extern struct ClassTmpl ClassIntegerTmpl;


int IsInteger(Unknown *);

Integer *Integer_FromCLong(long);
Integer *Integer_FromCPtrdiff(ptrdiff_t);

/***/long Integer_AsCLong(Integer *);
ptrdiff_t Integer_AsCPtrdiff(Integer *);


#endif /* __int_h__ */
