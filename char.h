
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


extern struct Behavior *Spk_ClassChar;
extern struct SpkClassTmpl Spk_ClassCharTmpl;


Char *SpkChar_fromChar(char);
Char *SpkChar_fromLiteral(char *, size_t);
char SpkChar_asChar(Char *);


#endif /* __char_h__ */
