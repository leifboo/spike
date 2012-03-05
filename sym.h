
#ifndef __sym_h__
#define __sym_h__


#include "obj.h"


typedef struct Symbol Symbol;


extern struct ClassTmpl ClassSymbolTmpl;


int IsSymbol(Unknown *);
Symbol *Symbol_FromCString(const char *str);
Symbol *Symbol_FromCStringAndLength(const char *str, size_t len);
const char *Symbol_AsCString(Symbol *);
size_t Symbol_Hash(Symbol *);


#endif /* __sym_h__ */
