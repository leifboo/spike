
#include "sym.h"

#include "behavior.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


Behavior *Spk_ClassSymbol;

static struct {
    size_t size, mask, tally;
    Symbol **array;
} hashTable;


/*------------------------------------------------------------------------*/
/* methods */

static Object *Symbol_print(Object *_self, Object *arg0, Object *arg1) {
    Symbol *self;
    
    self = (Symbol *)_self;
    fprintf(stdout, "$%s", self->str);
    return Spk_void;
}


/*------------------------------------------------------------------------*/
/* class template */

static SpkMethodTmpl SymbolMethods[] = {
    { "print", SpkNativeCode_METH_ATTR | SpkNativeCode_ARGS_0, &Symbol_print },
    { 0, 0, 0}
};

SpkClassTmpl Spk_ClassSymbolTmpl = {
    "Symbol",
    0,
    sizeof(Symbol),
    sizeof(char),
    0,
    SymbolMethods
};


/*------------------------------------------------------------------------*/
/* C API */

static size_t getHash(unsigned char *str, size_t *len) {
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

Symbol *SpkSymbol_get(const char *str) {
    Symbol *sym;
    size_t hash, start, len, i, o;
    
    if (!hashTable.array) {
        hashTable.size = 2; /* must be a power of 2 */
        hashTable.mask = hashTable.size - 1;
        hashTable.array = (Symbol **)calloc(hashTable.size, sizeof(Symbol *));
    }
    
    hash = getHash((unsigned char *)str, &len);
    start = hash & hashTable.mask;
    
    for (i = start; i < hashTable.size; ++i) {
        sym = hashTable.array[i];
        if (!sym) {
            goto insert;
        } else if (strcmp(sym->str, str) == 0) {
            return sym;
        }
    }
    
    for (i = 0; i < start; ++i) {
        sym = hashTable.array[i];
        if (!sym) {
            goto insert;
        } else if (strcmp(sym->str, str) == 0) {
            return sym;
        }
    }
    
    assert(0 && "not reached");
    
 insert:
    sym = (Symbol *)malloc(sizeof(Symbol) + len);
    sym->base.klass = Spk_ClassSymbol;
    sym->hash = hash;
    memcpy(sym->str, str, len + 1);
    
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
                assert(0 && "not reached");
 reinsert:
                hashTable.array[i] = oldSym;
            }
        }
        
        free(oldArray);
    }
    
    return sym;
}
