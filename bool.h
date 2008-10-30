
#ifndef __bool_h__
#define __bool_h__


#include "obj.h"


typedef Object Boolean;


extern struct Behavior *ClassBoolean, *ClassFalse, *ClassTrue;
extern struct SpkClassTmpl ClassBooleanTmpl, ClassFalseTmpl, ClassTrueTmpl;
extern Boolean *Spk_false, *Spk_true;


#endif /* __bool_h__ */
