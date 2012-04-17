
#include "kwsel.h"

#include "array.h"
#include "heart.h"
#include "obj.h"
#include "sym.h"

#include <stdlib.h>
#include <string.h>


char *kwSep[2] = { " ", ":" };


Unknown *NewKeywordSelectorBuilder(void) {
    return (Unknown *)Array_New(2);
}


void AppendKeyword(Unknown **builder, Symbol *kw) {
    Array *kwArray;
    size_t size, i;
    
    kwArray = CAST(Array, *builder);
    size = Array_Size(kwArray);
    for (i = 0; i < size; ++i) {
        Unknown *item = Array_GetItem(kwArray, i);
        if (item == GLOBAL(uninit)) {
            break;
        }
    }
    if (i == size) {
        Array *newArray;
        size_t newSize;
        
        newSize = size * 2;
        newArray = Array_New(newSize);
        for (i = 0; i < size; ++i) {
            Unknown *item = Array_GetItem(kwArray, i);
            Array_SetItem(newArray, i, item);
        }
        kwArray = newArray;
        *builder = (Unknown *)kwArray;
    }
    Array_SetItem(kwArray, i, (Unknown *)kw);
}


static int compareSymbols(Unknown *const *p1, Unknown *const *p2) {
    Symbol *sym1, *sym2;
    const char *s1, *s2;
    
    sym1 = CAST(Symbol, *p1);
    sym2 = CAST(Symbol, *p2);
    if (!sym1 && !sym2)
        return 0;
    if (!sym1)
        return 1;
    if (!sym2)
        return -1;
    s1 = Symbol_AsCString(sym1);
    s2 = Symbol_AsCString(sym2);
    return strcmp(s1, s2);
}


Symbol *GetKeywordSelector(Unknown *builder, Symbol *kw) {
    Array *kwArray;
    Symbol *sym, *selector;
    size_t size, len, i, tally;
    char *str;
    
    kwArray = CAST(Array, builder);
    size = Array_Size(kwArray);
    len = 0;
    
    if (kw) {
        sym = kw;
        len += strlen(Symbol_AsCString(sym));
        if (0) {
            /* XXX: The formal arguments must be permuted in the same
               way. */
            Array_Sort(kwArray, &compareSymbols);
        }
    }
    for (i = 0; i < size; ++i) {
        Unknown *item = Array_GetItem(kwArray, i);
        if (item == GLOBAL(uninit)) {
            break;
        }
        sym = CAST(Symbol, item);
        len += strlen(Symbol_AsCString(sym)) + strlen(kwSep[1]);
    }
    
    tally = i;
    if (tally > 0)
        len += strlen(kwSep[0]);
    
    str = (char *)malloc(len + 1);
    str[0] = '\0';
    
    if (kw) {
        sym = kw;
        strcat(str, Symbol_AsCString(sym));
        if (tally > 0)
            strcat(str, kwSep[0]);
    }
    for (i = 0; i < size; ++i) {
        Unknown *item = Array_GetItem(kwArray, i);
        if (item == GLOBAL(uninit)) {
            break;
        }
        sym = CAST(Symbol, item);
        strcat(str, Symbol_AsCString(sym));
        strcat(str, kwSep[1]);
    }
    str[len] = '\0';
    
    selector = Symbol_FromCString(str);
    free(str);
    return selector;
}

