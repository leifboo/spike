
#ifndef __spk_char_h__
#define __spk_char_h__


#include "obj.h"


typedef struct SpkChar SpkChar;


extern struct SpkClassTmpl Spk_ClassCharTmpl;


SpkChar *SpkChar_FromCChar(char);
char SpkChar_AsCChar(SpkChar *);


#endif /* __spk_char_h__ */
