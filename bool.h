
#ifndef __spk_bool_h__
#define __spk_bool_h__


#include "obj.h"


#ifndef MALTIPY

#if 0
typedef SpkObject SpkBoolean;
#endif

int SpkBoolean_Boot(void);

extern struct SpkBehavior *Spk_ClassBoolean, *Spk_ClassFalse, *Spk_ClassTrue;
#if 0
extern struct SpkClassTmpl Spk_ClassBooleanTmpl, Spk_ClassFalseTmpl, Spk_ClassTrueTmpl;
#endif

#endif /* !MALTIPY */


extern SpkUnknown *Spk_false, *Spk_true;


#endif /* __spk_bool_h__ */
