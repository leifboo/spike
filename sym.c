
#include "sym.h"

#include "class.h"
#include "native.h"
#include <stdlib.h>
#include <string.h>


SpkBehavior *Spk_ClassSymbol;

static struct {
    size_t size, mask, tally;
    SpkSymbol **array;
} hashTable;


/*------------------------------------------------------------------------*/
/* class template */

static SpkMethodTmpl SymbolMethods[] = {
    { 0, 0, 0}
};

SpkClassTmpl Spk_ClassSymbolTmpl = {
    "Symbol", {
        /*accessors*/ 0,
        SymbolMethods,
        /*lvalueMethods*/ 0,
        /*offsetof(SpkSymbolSubclass, variables)*/ 0,
        sizeof(char)
    }, /*meta*/ {
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
    while (c = *p++)
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

static SpkSymbol *newSymbol(const char *str, size_t len, size_t hash) {
    SpkSymbol *sym;
    size_t start, i, o;
    
    if (!hashTable.array) {
        hashTable.size = 2; /* must be a power of 2 */
        hashTable.mask = hashTable.size - 1;
        hashTable.array = (SpkSymbol **)calloc(hashTable.size, sizeof(SpkSymbol *));
    }
    
    start = hash & hashTable.mask;
    
    for (i = start; i < hashTable.size; ++i) {
        sym = hashTable.array[i];
        if (!sym) {
            goto insert;
        } else if (strcmp(sym->str, str) == 0) {
            Spk_INCREF(sym);
            return sym;
        }
    }
    
    for (i = 0; i < start; ++i) {
        sym = hashTable.array[i];
        if (!sym) {
            goto insert;
        } else if (strcmp(sym->str, str) == 0) {
            Spk_INCREF(sym);
            return sym;
        }
    }
    
    Spk_Halt(Spk_HALT_ASSERTION_ERROR, "not reached");
    return 0;
    
 insert:
    sym = (SpkSymbol *)malloc(sizeof(SpkSymbol) + len);
    sym->base.klass = Spk_ClassSymbol;
    sym->hash = hash;
    memcpy(sym->str, str, len + 1);
    
    hashTable.array[i] = sym;
    ++hashTable.tally;
    
    if (hashTable.size - hashTable.tally < hashTable.size / 4 + 1) {
        /* grow */
        size_t oldSize;
        SpkSymbol **oldArray;
        SpkSymbol *oldSym;
        
        oldSize = hashTable.size;
        oldArray = hashTable.array;
        
        hashTable.size <<= 1;
        hashTable.mask = hashTable.size - 1;
        hashTable.array = (SpkSymbol **)calloc(hashTable.size, sizeof(SpkSymbol *));
        
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
                Spk_Halt(Spk_HALT_ASSERTION_ERROR, "not reached");
                return 0;
 reinsert:
                hashTable.array[i] = oldSym;
            }
        }
        
        free(oldArray);
    }
    
    Spk_INCREF(sym);
    return sym;
}

SpkSymbol *SpkSymbol_get(const char *str) {
    size_t hash, len;
    
    hash = getHashAndLength((unsigned char *)str, &len);
    return newSymbol(str, len, hash);
}

SpkSymbol *SpkSymbol_fromCStringAndLength(const char *str, size_t len) {
    size_t hash = getHash((unsigned char *)str, len);
    return newSymbol(str, len, hash);
}
