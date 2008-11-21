
#include "sym.h"

#include "behavior.h"
#include <stdlib.h>
#include <string.h>


Behavior *ClassSymbol;


static SpkMethodTmpl SymbolMethods[] = {
    { 0, 0, 0}
};

SpkClassTmpl ClassSymbolTmpl = {
    "Symbol",
    0,
    sizeof(Symbol),
    0,
    SymbolMethods
};


Symbol *SpkSymbol_get(const char *str) {
    /* Just a simple linked list for now! */
    static Symbol *list = 0;
    Symbol *s;
    
    for (s = list; s; s = s->next) {
        if (strcmp(s->str, str) == 0) {
            return s;
        }
    }
    
    s = (Symbol *)malloc(sizeof(Symbol) + strlen(str));
    s->base.klass = ClassSymbol;
    s->next = list;
    strcpy(s->str, str);
    list = s;
    
    return s;
}
