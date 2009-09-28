
#include "str.h"

#include "bool.h"
#include "char.h"
#include "class.h"
#include "heart.h"
#include "int.h"
#include "native.h"
#include <string.h>


#define BOOL(cond) ((cond) ? Spk_GLOBAL(xtrue) : Spk_GLOBAL(xfalse))
#define STR(op) ((char *)SpkVariableObject_ITEM_BASE(op))
#define LEN(op) ((op)->size - 1)


/*------------------------------------------------------------------------*/
/* method helpers */

static SpkUnknown *String_binaryLogicalOper(SpkString *self, SpkUnknown *arg0, SpkOper oper) {
    SpkString *arg;
    SpkUnknown *result;
    
    arg = Spk_CAST(String, arg0);
    if (!arg) switch (oper) {
    case Spk_OPER_EQ:
        /* XXX: 0 == 0.0 */
        Spk_INCREF(Spk_GLOBAL(xfalse));
        return Spk_GLOBAL(xfalse);
    case Spk_OPER_NE:
        Spk_INCREF(Spk_GLOBAL(xtrue));
        return Spk_GLOBAL(xtrue);
    default:
        Spk_Halt(Spk_HALT_ASSERTION_ERROR, "XXX");
        return 0;
    }
    
    switch (oper) {
    case Spk_OPER_LT: result = BOOL(strcmp(STR(self), STR(arg)) < 0);  break;
    case Spk_OPER_GT: result = BOOL(strcmp(STR(self), STR(arg)) > 0);  break;
    case Spk_OPER_LE: result = BOOL(strcmp(STR(self), STR(arg)) <= 0); break;
    case Spk_OPER_GE: result = BOOL(strcmp(STR(self), STR(arg)) >= 0); break;
    case Spk_OPER_EQ: result = BOOL(strcmp(STR(self), STR(arg)) == 0); break;
    case Spk_OPER_NE: result = BOOL(strcmp(STR(self), STR(arg)) != 0); break;
    default:
        Spk_Halt(Spk_HALT_ASSERTION_ERROR, "not reached");
        return 0;
    }
    Spk_INCREF(result);
    return result;
}


/*------------------------------------------------------------------------*/
/* methods -- operators */

/* Spk_OPER_ADD */
static SpkUnknown *String_add(SpkUnknown *_self, SpkUnknown *arg0, SpkUnknown *arg1) {
    SpkString *self, *arg, *result;
    size_t resultLen;
    
    self = (SpkString *)_self;
    arg = Spk_CAST(String, arg0);
    if (!arg) {
        Spk_Halt(Spk_HALT_TYPE_ERROR, "a string is required");
        return 0;
    }
    resultLen = LEN(self) + LEN(arg);
    result = (SpkString *)SpkObject_NewVar(Spk_CLASS(String), resultLen + 1);
    if (!result)
        return 0;
    memcpy(STR(result), STR(self), LEN(self));
    memcpy(STR(result) + LEN(self), STR(arg), arg->size);
    return (SpkUnknown *)result;
}

/* Spk_OPER_LT */
static SpkUnknown *String_lt(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return String_binaryLogicalOper((SpkString *)self, arg0, Spk_OPER_LT);
}

/* Spk_OPER_GT */
static SpkUnknown *String_gt(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return String_binaryLogicalOper((SpkString *)self, arg0, Spk_OPER_GT);
}

/* Spk_OPER_LE */
static SpkUnknown *String_le(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return String_binaryLogicalOper((SpkString *)self, arg0, Spk_OPER_LE);
}

/* Spk_OPER_GE */
static SpkUnknown *String_ge(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return String_binaryLogicalOper((SpkString *)self, arg0, Spk_OPER_GE);
}

/* Spk_OPER_EQ */
static SpkUnknown *String_eq(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return String_binaryLogicalOper((SpkString *)self, arg0, Spk_OPER_EQ);
}

/* Spk_OPER_NE */
static SpkUnknown *String_ne(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return String_binaryLogicalOper((SpkString *)self, arg0, Spk_OPER_NE);
}

/* Spk_OPER_GET_ITEM */
static SpkUnknown *String_item(SpkUnknown *_self, SpkUnknown *arg0, SpkUnknown *arg1) {
    SpkString *self; SpkInteger *arg;
    ptrdiff_t index;
    
    self = (SpkString *)_self;
    arg = Spk_CAST(Integer, arg0);
    if (!arg) {
        Spk_Halt(Spk_HALT_TYPE_ERROR, "an integer is required");
        return 0;
    }
    index = SpkInteger_AsCPtrdiff(arg);
    if (index < 0 || LEN(self) <= (size_t)index) {
        Spk_Halt(Spk_HALT_INDEX_ERROR, "index out of range");
        return 0;
    }
    return (SpkUnknown *)SpkChar_FromCChar(STR(self)[index]);
}


/*------------------------------------------------------------------------*/
/* methods -- attributes */

static SpkUnknown *String_size(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    return (SpkUnknown *)SpkInteger_FromCPtrdiff(LEN((SpkString *)self));
}


/*------------------------------------------------------------------------*/
/* methods -- enumerating */

static SpkUnknown *String_do(SpkUnknown *_self, SpkUnknown *arg0, SpkUnknown *arg1) {
    SpkString *self;
    size_t i;
    SpkChar *c;
    SpkUnknown *result;
    
    self = (SpkString *)_self;
    for (i = 0; i < LEN(self); ++i) {
        c = SpkChar_FromCChar(STR(self)[i]);
        result = Spk_Call(Spk_GLOBAL(theInterpreter), arg0, Spk_OPER_APPLY, c, 0);
        Spk_DECREF(c);
        if (!result)
            return 0; /* unwind */
        Spk_DECREF(result);
    }
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
}


/*------------------------------------------------------------------------*/
/* methods -- printing */

static SpkUnknown *String_printString(SpkUnknown *_self, SpkUnknown *arg0, SpkUnknown *arg1) {
    SpkString *self;
    SpkString *result;
    char *d, *s, *subst;
    char c;
    
    self = (SpkString *)_self;
    result = SpkString_FromCStringAndLength(0, 2*LEN(self) + 2);
    d = SpkString_AsCString(result);
    s = STR(self);
    *d++ = '"';
    while ((c = *s++)) {
        /* XXX: numeric escape codes */
        subst = 0;
        switch (c) {
        case '\a': subst = "\\a"; break;
        case '\b': subst = "\\b"; break;
        case '\f': subst = "\\f"; break;
        case '\n': subst = "\\n"; break;
        case '\r': subst = "\\r"; break;
        case '\t': subst = "\\t"; break;
        case '\v': subst = "\\v"; break;
        case '\\': subst = "\\\\"; break;
        case '"':  subst = "\\\""; break;
        default:
            break;
        }
        if (subst) {
            *d++ = *subst++;
            *d++ = *subst++;
        } else {
            *d++ = c;
        }
    }
    *d++ = '"';
    *d = '\0';
    return (SpkUnknown *)result;
}


/*------------------------------------------------------------------------*/
/* class tmpl */

typedef SpkVariableObjectSubclass SpkStringSubclass;

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
    { "__index__", SpkNativeCode_ARGS_1 | SpkNativeCode_LEAF, &String_item },
    /* attributes */
    { "len", SpkNativeCode_ARGS_0, &String_size },
    { "size", SpkNativeCode_ARGS_0, &String_size },
    /* enumerating */
    { "do:", SpkNativeCode_ARGS_1, &String_do },
    /* printing */
    { "printString", SpkNativeCode_ARGS_0, &String_printString },
    { 0 }
};

SpkClassTmpl Spk_ClassStringTmpl = {
    Spk_HEART_CLASS_TMPL(String, Object), {
        /*accessors*/ 0,
        methods,
        /*lvalueMethods*/ 0,
        offsetof(SpkStringSubclass, variables),
        sizeof(char)
    }, /*meta*/ {
        0
    }
};


/*------------------------------------------------------------------------*/
/* C API */

SpkString *SpkString_FromCString(const char *str) {
    return SpkString_FromCStringAndLength(str, strlen(str));
}

SpkString *SpkString_FromCStringAndLength(const char *str, size_t len) {
    SpkString *result;
    char *buffer;
    
    result = (SpkString *)SpkObject_NewVar(Spk_CLASS(String), len + 1);
    if (!result)
        return 0;
    buffer = STR(result);
    if (str)
        memcpy(buffer, str, len);
    buffer[len] = '\0';
    return result;
}

SpkString *SpkString_FromCStream(FILE *stream, size_t size) {
    SpkString *result;
    char *buffer;
    
    result = (SpkString *)SpkObject_NewVar(Spk_CLASS(String), size);
    if (!result)
        return 0;
    buffer = STR(result);
    if (!fgets(buffer, size, stream)) {
        Spk_DECREF(result);
        return 0;
    }
    result->size = strlen(buffer) + 1;
    return result;
}

SpkString *SpkString_Concat(SpkString **var, SpkString *newPart) {
    SpkString *self, *result;
    size_t resultLen;
    
    self = *var;
    resultLen = LEN(self) + LEN(newPart);
    result = (SpkString *)SpkObject_NewVar(Spk_CLASS(String), resultLen + 1);
    if (!result) {
        *var = 0;
        Spk_DECREF(self);
        return 0;
    }
    memcpy(STR(result), STR(self), LEN(self));
    memcpy(STR(result) + LEN(self), STR(newPart), newPart->size);
    Spk_DECREF(self);
    *var = result;
    return result;
}

char *SpkString_AsCString(SpkString *self) {
    return STR(self);
}

size_t SpkString_Size(SpkString *self) {
    return self->size;
}
