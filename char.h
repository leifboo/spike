
#ifndef __char_h__
#define __char_h__


#include "obj.h"
#include <stddef.h>


typedef struct Char {
    Object base;
    char value;
} Char;

typedef struct CharSubclass {
    Char base;
    Object *variables[1]; /* stretchy */
} CharSubclass;


extern struct Behavior *ClassChar;


void SpkClassChar_init(void);
Char *SpkChar_fromLiteral(char *, size_t);


#endif /* __char_h__ */
