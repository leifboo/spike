
#ifndef __str_h__
#define __str_h__


#include "obj.h"
#include <stdio.h>


typedef struct String String;


extern struct ClassTmpl ClassStringTmpl;


int IsString(Unknown *);
String *String_FromCString(const char *);
String *String_FromCStringAndLength(const char *, size_t);
String *String_FromCStream(FILE *, size_t);
String *String_Concat(String **, String *);
char *String_AsCString(String *);
size_t String_Size(String *);
int String_IsEqual(String *, String *);


#endif /* __str_h__ */
