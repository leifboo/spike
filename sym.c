
#include "sym.h"

#include "class.h"
#include "heart.h"
#include "native.h"
#include "str.h"

#include <stdlib.h>
#include <string.h>


struct Symbol {
    Object base;
    size_t hash;
    char str[1];
};


static struct {
    size_t size, mask, tally;
    Symbol **array;
} hashTable;


/*------------------------------------------------------------------------*/
/* methods */

static Unknown *Symbol_printString(Unknown *_self, Unknown *arg0, Unknown *arg1) {
    Symbol *self;
    String *result;
    size_t len;
    char *str;
    
    self = (Symbol *)_self;
    len = strlen(self->str);
    result = String_FromCStringAndLength(0, len + 1);
    if (!result)
        return 0;
    str = String_AsCString(result);
    str[0] = '$';
    memcpy(&str[1], self->str, len);
    return (Unknown *)result;
}


/*------------------------------------------------------------------------*/
/* class tmpl */

static MethodTmpl methods[] = {
    { "printString", NativeCode_ARGS_0, &Symbol_printString },
    { 0 }
};

ClassTmpl ClassSymbolTmpl = {
    HEART_CLASS_TMPL(Symbol, Object), {
        /*accessors*/ 0,
        methods,
        /*lvalueMethods*/ 0,
        /*offsetof(SymbolSubclass, variables)*/ 0,
        sizeof(char)
    }, /*meta*/ {
        0
    }
};


/*------------------------------------------------------------------------*/
/* C API */

static size_t getHashAndLength(unsigned char *str, size_t *len) {
    /* djb2 from http://www.cse.yorku.ca/~oz/hash.html */
    unsigned char *p;
    size_t hash = 5381;
    int c;
    
    p = str;
    while ((c = *p++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    *len = p - str - 1;
    return hash;
}

static size_t getHash(unsigned char *str, size_t len) {
    /* djb2 from http://www.cse.yorku.ca/~oz/hash.html */
    unsigned char *p;
    size_t hash = 5381;
    int c;
    
    p = str;
    while (len--) {
        c = *p++;
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return hash;
}

static Symbol *newSymbol(const char *str, size_t len, size_t hash) {
    Symbol *sym;
    size_t start, i, o;
    
    if (!hashTable.array) {
        hashTable.size = 2; /* must be a power of 2 */
        hashTable.mask = hashTable.size - 1;
        hashTable.array = (Symbol **)calloc(hashTable.size, sizeof(Symbol *));
    }
    
    start = hash & hashTable.mask;
    
    for (i = start; i < hashTable.size; ++i) {
        sym = hashTable.array[i];
        if (!sym) {
            goto insert;
        } else if (strcmp(sym->str, str) == 0) {
            INCREF(sym);
            return sym;
        }
    }
    
    for (i = 0; i < start; ++i) {
        sym = hashTable.array[i];
        if (!sym) {
            goto insert;
        } else if (strcmp(sym->str, str) == 0) {
            INCREF(sym);
            return sym;
        }
    }
    
    Halt(HALT_ASSERTION_ERROR, "not reached");
    return 0;
    
 insert:
    sym = (Symbol *)malloc(sizeof(Symbol) + len);
    sym->base.base.refCount = 1;
    sym->base.klass = CLASS(Symbol);
    sym->hash = hash;
    memcpy(sym->str, str, len);
    sym->str[len] = '\0';
    
    hashTable.array[i] = sym;
    ++hashTable.tally;
    
    if (hashTable.size - hashTable.tally < hashTable.size / 4 + 1) {
        /* grow */
        size_t oldSize;
        Symbol **oldArray;
        Symbol *oldSym;
        
        oldSize = hashTable.size;
        oldArray = hashTable.array;
        
        hashTable.size <<= 1;
        hashTable.mask = hashTable.size - 1;
        hashTable.array = (Symbol **)calloc(hashTable.size, sizeof(Symbol *));
        
        for (o = 0; o < oldSize; ++o) {
            oldSym = oldArray[o];
            if (oldSym) {
                start = oldSym->hash & hashTable.mask;
                for (i = start; i < hashTable.size; ++i) {
                    if (!hashTable.array[i]) {
                        goto reinsert;
                    }
                }
                for (i = 0; i < start; ++i) {
                    if (!hashTable.array[i]) {
                        goto reinsert;
                    }
                }
                Halt(HALT_ASSERTION_ERROR, "not reached");
                return 0;
 reinsert:
                hashTable.array[i] = oldSym;
            }
        }
        
        free(oldArray);
    }
    
    INCREF(sym);
    return sym;
}

Symbol *Symbol_FromCString(const char *str) {
    size_t hash, len;
    
    hash = getHashAndLength((unsigned char *)str, &len);
    return newSymbol(str, len, hash);
}

Symbol *Symbol_FromCStringAndLength(const char *str, size_t len) {
    size_t hash = getHash((unsigned char *)str, len);
    return newSymbol(str, len, hash);
}

const char *Symbol_AsCString(Symbol *self) {
    return self->str;
}

size_t Symbol_Hash(Symbol *self) {
    return self->hash;
}
