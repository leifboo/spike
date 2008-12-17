
#ifndef __bool_h__
#define __bool_h__


#include "obj.h"


typedef Object Boolean;


extern struct Behavior *Spk_ClassBoolean, *Spk_ClassFalse, *Spk_ClassTrue;
extern struct SpkClassTmpl Spk_ClassBooleanTmpl, Spk_ClassFalseTmpl, Spk_ClassTrueTmpl;
extern Boolean *Spk_false, *Spk_true;


#endif /* __bool_h__ */
