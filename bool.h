
#ifndef __spk_bool_h__
#define __spk_bool_h__


#include "obj.h"


#ifndef MALTIPY

int SpkBoolean_Boot(void);

extern struct SpkBehavior *Spk_ClassBoolean, *Spk_ClassFalse, *Spk_ClassTrue;

#endif /* !MALTIPY */


extern SpkUnknown *Spk_false, *Spk_true;


#endif /* __spk_bool_h__ */
