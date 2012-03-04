
#ifndef __char_h__
#define __char_h__


#include "obj.h"


typedef struct Char Char;


extern struct ClassTmpl ClassCharTmpl;


Char *Char_FromCChar(char);
char Char_AsCChar(Char *);


#endif /* __char_h__ */
