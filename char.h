
#ifndef __spk_char_h__
#define __spk_char_h__


#include "obj.h"
#include <stddef.h>


typedef struct SpkChar {
    SpkObject base;
    char value;
} SpkChar;

typedef struct SpkCharSubclass {
    SpkChar base;
    SpkUnknown *variables[1]; /* stretchy */
} SpkCharSubclass;


extern struct SpkBehavior *Spk_ClassChar;
extern struct SpkClassTmpl Spk_ClassCharTmpl;


SpkChar *SpkChar_FromChar(char);
char SpkChar_AsChar(SpkChar *);


#endif /* __spk_char_h__ */
