
#ifndef __str_h__
#define __str_h__


#include "obj.h"
#include <stdio.h>


typedef VariableObject String;
typedef VariableObjectSubclass StringSubclass;


extern struct Behavior *Spk_ClassString;
extern struct SpkClassTmpl Spk_ClassStringTmpl;


String *SpkString_fromLiteral(char *, size_t);
String *SpkString_fromStream(FILE *, size_t);
char *SpkString_asString(String *);
size_t SpkString_size(String *);


#endif /* __str_h__ */
