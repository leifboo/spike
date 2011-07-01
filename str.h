
#ifndef __spk_str_h__
#define __spk_str_h__


#include "obj.h"
#include <stdio.h>


typedef SpkVariableObject SpkString;


extern struct SpkClassTmpl Spk_ClassStringTmpl;


SpkString *SpkString_FromCString(const char *);
SpkString *SpkString_FromCStringAndLength(const char *, size_t);
SpkString *SpkString_FromCStream(FILE *, size_t);
SpkString *SpkString_Concat(SpkString **, SpkString *);
char *SpkString_AsCString(SpkString *);
size_t SpkString_Size(SpkString *);
int SpkString_IsEqual(SpkString *, SpkString *);


#endif /* __spk_str_h__ */
