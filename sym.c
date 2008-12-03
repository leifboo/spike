
#include "sym.h"

#include "behavior.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


Behavior *ClassSymbol;


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
    { "print", SpkNativeCode_ARGS_0 | SpkNativeCode_CALLABLE, &Symbol_print },
    { 0, 0, 0}
};

SpkClassTmpl ClassSymbolTmpl = {
    "Symbol",
    0,
    sizeof(Symbol),
    sizeof(char),
    0,
    SymbolMethods
};


/*------------------------------------------------------------------------*/
/* C API */

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
