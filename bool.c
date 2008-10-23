
#include "bool.h"

#include "behavior.h"
#include "module.h"
#include <stdio.h>
#include <stdlib.h>


Boolean *Spk_false, *Spk_true;
static Behavior *ClassBoolean, *ClassFalse, *ClassTrue;


/*------------------------------------------------------------------------*/
/* methods */

static Object *False_print(Object *self) {
    printf("false\n");
}

static Object *True_print(Object *self) {
    printf("true\n");
}


/*------------------------------------------------------------------------*/
/* C API */

void SpkClassBoolean_init(void) {
    ClassBoolean = SpkBehavior_new(0 /*ClassObject*/ , builtInModule);
    ClassFalse = SpkBehavior_new(ClassBoolean, builtInModule);
    ClassTrue = SpkBehavior_new(ClassBoolean, builtInModule);
    
    ClassFalse->print = &False_print;
    ClassTrue->print = &True_print;
    
    Spk_false = (Boolean *)malloc(sizeof(Boolean));
    Spk_false->klass = ClassFalse;
    Spk_true = (Boolean *)malloc(sizeof(Boolean));
    Spk_true->klass = ClassTrue;
}
