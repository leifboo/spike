
#ifndef __bool_h__
#define __bool_h__


#include "interp.h"


typedef Object Boolean;


extern Boolean *Spk_false, *Spk_true;


void SpkClassBoolean_init(void);


#endif /* __bool_h__ */
