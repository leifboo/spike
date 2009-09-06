
#ifndef __spk_sym_h__
#define __spk_sym_h__


#include "obj.h"


typedef struct SpkSymbol SpkSymbol;


extern struct SpkBehavior *Spk_ClassSymbol;
extern struct SpkClassTmpl Spk_ClassSymbolTmpl;


SpkSymbol *SpkSymbol_FromCString(const char *str);
SpkSymbol *SpkSymbol_FromCStringAndLength(const char *str, size_t len);
const char *SpkSymbol_AsCString(SpkSymbol *);


#endif /* __spk_sym_h__ */
