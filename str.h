
#ifndef __str_h__
#define __str_h__


#include "obj.h"
#include <stddef.h>


typedef struct String {
    Object base;
    size_t len;
    char str[1];
} String;


extern struct Behavior *ClassString;


void SpkClassString_init(void);
String *SpkString_fromLiteral(char *, size_t);


#endif /* __str_h__ */
