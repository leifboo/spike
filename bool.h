
#ifndef __spk_bool_h__
#define __spk_bool_h__


#include "obj.h"


#ifndef MALTIPY

typedef SpkObject SpkBoolean;


extern struct SpkBehavior *Spk_ClassBoolean, *Spk_ClassFalse, *Spk_ClassTrue;
extern struct SpkClassTmpl Spk_ClassBooleanTmpl, Spk_ClassFalseTmpl, Spk_ClassTrueTmpl;

#endif /* !MALTIPY */


extern SpkUnknown *Spk_false, *Spk_true;


#endif /* __spk_bool_h__ */
