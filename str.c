
#include "str.h"

#include "char.h"
#include "behavior.h"
#include "bool.h"
#include "int.h"
#include "interp.h"
#include "module.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define BOOL(cond) ((cond) ? Spk_true : Spk_false)
#define STR(op) ((char *)SpkVariableObject_ITEM_BASE(op))
#define LEN(op) ((op)->size - 1)


Behavior *Spk_ClassString;


/*------------------------------------------------------------------------*/
/* method helpers */

static Object *String_binaryLogicalOper(String *self, Object *arg0, Oper oper) {
    String *arg;
    Boolean *result;
    
    assert(arg = Spk_CAST(String, arg0)); /* XXX */
    switch (oper) {
    case OPER_LT: result = BOOL(strcmp(STR(self), STR(arg)) < 0);  break;
    case OPER_GT: result = BOOL(strcmp(STR(self), STR(arg)) > 0);  break;
    case OPER_LE: result = BOOL(strcmp(STR(self), STR(arg)) <= 0); break;
    case OPER_GE: result = BOOL(strcmp(STR(self), STR(arg)) >= 0); break;
    case OPER_EQ: result = BOOL(strcmp(STR(self), STR(arg)) == 0); break;
    case OPER_NE: result = BOOL(strcmp(STR(self), STR(arg)) != 0); break;
    default: assert(0);
    }
    return (Object *)result;
}


/*------------------------------------------------------------------------*/
/* methods -- operators */

/* OPER_ADD */
static Object *String_add(Object *_self, Object *arg0, Object *arg1) {
    String *self, *arg, *result;
    size_t resultLen;
    
    self = (String *)_self;
    assert(arg = Spk_CAST(String, arg0)); /* XXX */
    resultLen = LEN(self) + LEN(arg);
    result = (String *)malloc(sizeof(String) + resultLen + 1);
    result->base.klass = Spk_ClassString;
    result->size = resultLen + 1;
    memcpy(STR(result), STR(self), LEN(self));
    memcpy(STR(result) + LEN(self), STR(arg), arg->size);
    return (Object *)result;
}

/* OPER_LT */
static Object *String_lt(Object *self, Object *arg0, Object *arg1) {
    return String_binaryLogicalOper((String *)self, arg0, OPER_LT);
}

/* OPER_GT */
static Object *String_gt(Object *self, Object *arg0, Object *arg1) {
    return String_binaryLogicalOper((String *)self, arg0, OPER_GT);
}

/* OPER_LE */
static Object *String_le(Object *self, Object *arg0, Object *arg1) {
    return String_binaryLogicalOper((String *)self, arg0, OPER_LE);
}

/* OPER_GE */
static Object *String_ge(Object *self, Object *arg0, Object *arg1) {
    return String_binaryLogicalOper((String *)self, arg0, OPER_GE);
}

/* OPER_EQ */
static Object *String_eq(Object *self, Object *arg0, Object *arg1) {
    return String_binaryLogicalOper((String *)self, arg0, OPER_EQ);
}

/* OPER_NE */
static Object *String_ne(Object *self, Object *arg0, Object *arg1) {
    return String_binaryLogicalOper((String *)self, arg0, OPER_NE);
}

/* OPER_GET_ITEM */
static Object *String_item(Object *_self, Object *arg0, Object *arg1) {
    String *self; Integer *arg;
    long index;
    
    self = (String *)_self;
    assert(arg = Spk_CAST(Integer, arg0)); /* XXX */
    index = SpkInteger_asLong(arg);
    assert(0 <= index && index < LEN(self)); /* XXX */
    return (Object *)SpkChar_fromChar(STR(self)[index]);
}


/*------------------------------------------------------------------------*/
/* methods -- other */

static Object *String_print(Object *_self, Object *arg0, Object *arg1) {
    String *self;
    
    self = (String *)_self;
    fwrite(STR(self), 1, LEN(self), stdout);
    return Spk_void;
}


/*------------------------------------------------------------------------*/
/* class template */

static SpkMethodTmpl methods[] = {
    /* operators */
#if 0 /*XXX*/
    { "__mul__",    SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &String_mul    },
    { "__mod__",    SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &String_mod    },
#endif
    { "__add__",    SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &String_add    },
    { "__lt__",     SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &String_lt     },
    { "__gt__",     SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &String_gt     },
    { "__le__",     SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &String_le     },
    { "__ge__",     SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &String_ge     },
    { "__eq__",     SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &String_eq     },
    { "__ne__",     SpkNativeCode_BINARY_OPER | SpkNativeCode_LEAF, &String_ne     },
    /* call operators */
    { "__item__", SpkNativeCode_ARGS_1 | SpkNativeCode_LEAF, &String_item },
    /* other */
    { "print", SpkNativeCode_METH_ATTR | SpkNativeCode_ARGS_0, &String_print },
    { 0, 0, 0}
};

SpkClassTmpl Spk_ClassStringTmpl = {
    "String",
    offsetof(StringSubclass, variables),
    sizeof(String),
    sizeof(char),
    0,
    methods
};


/*------------------------------------------------------------------------*/
/* C API */

String *SpkString_fromLiteral(char *str, size_t len) {
    String *result;
    char *d, *s;
    char c;
    
    /* strip quotes */
    s = str + 1; len -= 2;
    
    /* XXX: This allocates too much. */
    result = (String *)malloc(sizeof(String) + len*sizeof(char) + 1);
    result->base.klass = Spk_ClassString;
    d = STR(result);
    
    while (len--) {
        c = *s++;
        /* Note: a trailing '\' is syntactically impossible. */
        if (len && c == '\\') {
            c = *s++;
            --len;
            
            /* XXX: numeric escape codes */
            switch (c) {
            case 'a': c = '\a'; break;
            case 'b': c = '\b'; break;
            case 'f': c = '\f'; break;
            case 'n': c = '\n'; break;
            case 'r': c = '\r'; break;
            case 't': c = '\t'; break;
            case 'v': c = '\v'; break;
                
            case '\\':
            case '\'':
            case '"':
                break;
                
            default:
                /* XXX: error */
                break;
            }
            *d++ = c;
        } else {
            *d++ = c;
        }
    }
    
    *d++ = '\0';
    result->size = d - STR(result);
    return result;
}

char *SpkString_asString(String *self) {
    return STR(self);
}
