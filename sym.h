
#ifndef __spk_sym_h__
#define __spk_sym_h__


#include "obj.h"


typedef struct SpkSymbol {
    SpkObject base;
    size_t hash;
    char str[1];
} SpkSymbol;


extern struct SpkBehavior *Spk_ClassSymbol;
extern struct SpkClassTmpl Spk_ClassSymbolTmpl;


SpkSymbol *SpkSymbol_get(const char *str);
SpkSymbol *SpkSymbol_fromCStringAndLength(const char *str, size_t len);


#endif /* __spk_sym_h__ */
