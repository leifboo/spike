
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


Behavior *ClassString;


/*------------------------------------------------------------------------*/
/* method helpers */

static Object *String_binaryLogicalOper(String *self, Object *arg0, Oper oper) {
    String *arg;
    Boolean *result;
    
    assert(arg0->klass == ClassString); /* XXX */
    arg = (String *)arg0;
    switch (oper) {
    case OPER_LT: result = BOOL(strcmp(self->str, arg->str) < 0);  break;
    case OPER_GT: result = BOOL(strcmp(self->str, arg->str) > 0);  break;
    case OPER_LE: result = BOOL(strcmp(self->str, arg->str) <= 0); break;
    case OPER_GE: result = BOOL(strcmp(self->str, arg->str) >= 0); break;
    case OPER_EQ: result = BOOL(strcmp(self->str, arg->str) == 0); break;
    case OPER_NE: result = BOOL(strcmp(self->str, arg->str) != 0); break;
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
    
    assert(arg0->klass == ClassString); /* XXX */
    self = (String *)_self;
    arg = (String *)arg0;
    resultLen = self->len + arg->len;
    result = (String *)malloc(sizeof(String) + resultLen);
    result->base.klass = ClassString;
    result->len = resultLen;
    memcpy(result->str, self->str, self->len);
    memcpy(result->str + self->len, arg->str, arg->len);
    result->str[resultLen] = '\0';
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
    String *self;
    long index;
    
    self = (String *)_self;
    assert(arg0->klass == ClassInteger); /* XXX */
    index = SpkInteger_asLong((Integer *)arg0);
    assert(0 <= index && index < self->len); /* XXX */
    return (Object *)SpkChar_fromChar(self->str[index]);
}

/* XXX: temporary */
static Object *String_setItem(Object *_self, Object *arg0, Object *arg1) {
    String *self;
    long index;
    char value;
    
    self = (String *)_self;
    assert(arg0->klass == ClassInteger); /* XXX */
    index = SpkInteger_asLong((Integer *)arg0);
    assert(0 <= index && index < self->len); /* XXX */
    assert(arg1->klass == ClassChar); /* XXX */
    value = SpkChar_asChar((Char *)arg1);
    self->str[index] = value;
    if (0) {
        /* XXX */
        return Spk_void;
    }
    return arg1;
}


/*------------------------------------------------------------------------*/
/* methods -- other */

static Object *String_print(Object *_self, Object *arg0, Object *arg1) {
    String *self;
    
    self = (String *)_self;
    fwrite(self->str, 1, self->len, stdout);
    return Spk_void;
}


/*------------------------------------------------------------------------*/
/* class template */

static SpkMethodTmpl methods[] = {
    /* operators */
#if 0 /*XXX*/
    { "__mul__",    SpkNativeCode_ARGS_1 | SpkNativeCode_LEAF, &String_mul    },
    { "__mod__",    SpkNativeCode_ARGS_1 | SpkNativeCode_LEAF, &String_mod    },
#endif
    { "__add__",    SpkNativeCode_ARGS_1 | SpkNativeCode_LEAF, &String_add    },
    { "__lt__",     SpkNativeCode_ARGS_1 | SpkNativeCode_LEAF, &String_lt     },
    { "__gt__",     SpkNativeCode_ARGS_1 | SpkNativeCode_LEAF, &String_gt     },
    { "__le__",     SpkNativeCode_ARGS_1 | SpkNativeCode_LEAF, &String_le     },
    { "__ge__",     SpkNativeCode_ARGS_1 | SpkNativeCode_LEAF, &String_ge     },
    { "__eq__",     SpkNativeCode_ARGS_1 | SpkNativeCode_LEAF, &String_eq     },
    { "__ne__",     SpkNativeCode_ARGS_1 | SpkNativeCode_LEAF, &String_ne     },
    /* call operators */
    { "__item__",    SpkNativeCode_ARGS_1 | SpkNativeCode_LEAF, &String_item    },
    { "__setItem__", SpkNativeCode_ARGS_2 | SpkNativeCode_LEAF, &String_setItem },
    /* other */
    { "print", SpkNativeCode_ARGS_0 | SpkNativeCode_CALLABLE, &String_print },
    { 0, 0, 0}
};

SpkClassTmpl ClassStringTmpl = {
    "String",
    0,
    sizeof(String),
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
    result = (String *)malloc(sizeof(String) + len*sizeof(char));
    result->base.klass = ClassString;
    d = result->str;
    
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
    
    result->len = d - result->str;
    *d++ = '\0';
    return result;
}
