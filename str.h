
#ifndef __spk_str_h__
#define __spk_str_h__


#include "obj.h"
#include <stdio.h>


typedef SpkVariableObject SpkString;
typedef SpkVariableObjectSubclass SpkStringSubclass;


extern struct SpkBehavior *Spk_ClassString;
extern struct SpkClassTmpl Spk_ClassStringTmpl;


SpkString *SpkString_fromCString(const char *);
SpkString *SpkString_fromCStringAndLength(const char *, size_t);
SpkString *SpkString_fromStream(FILE *, size_t);
SpkString *SpkString_concat(SpkString **, SpkString *);
char *SpkString_asString(SpkString *);
size_t SpkString_size(SpkString *);


#endif /* __spk_str_h__ */
