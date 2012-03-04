
#include "str.h"

#include "bool.h"
#include "char.h"
#include "class.h"
#include "heart.h"
#include "int.h"
#include "native.h"
#include <string.h>


#define BOOL(cond) ((cond) ? GLOBAL(xtrue) : GLOBAL(xfalse))
#define STR(op) ((char *)VariableObject_ITEM_BASE(op))
#define LEN(op) ((op)->size - 1)


/*------------------------------------------------------------------------*/
/* method helpers */

static Unknown *String_binaryLogicalOper(String *self, Unknown *arg0, Oper oper) {
    String *arg;
    Unknown *result;
    
    arg = CAST(String, arg0);
    if (!arg) switch (oper) {
    case OPER_EQ:
        /* XXX: 0 == 0.0 */
        return GLOBAL(xfalse);
    case OPER_NE:
        return GLOBAL(xtrue);
    default:
        Halt(HALT_ASSERTION_ERROR, "XXX");
        return 0;
    }
    
    switch (oper) {
    case OPER_LT: result = BOOL(strcmp(STR(self), STR(arg)) < 0);  break;
    case OPER_GT: result = BOOL(strcmp(STR(self), STR(arg)) > 0);  break;
    case OPER_LE: result = BOOL(strcmp(STR(self), STR(arg)) <= 0); break;
    case OPER_GE: result = BOOL(strcmp(STR(self), STR(arg)) >= 0); break;
    case OPER_EQ: result = BOOL(strcmp(STR(self), STR(arg)) == 0); break;
    case OPER_NE: result = BOOL(strcmp(STR(self), STR(arg)) != 0); break;
    default:
        Halt(HALT_ASSERTION_ERROR, "not reached");
        return 0;
    }
    return result;
}


/*------------------------------------------------------------------------*/
/* methods -- operators */

/* OPER_ADD */
static Unknown *String_add(Unknown *_self, Unknown *arg0, Unknown *arg1) {
    String *self, *arg, *result;
    size_t resultLen;
    
    self = (String *)_self;
    arg = CAST(String, arg0);
    if (!arg) {
        Halt(HALT_TYPE_ERROR, "a string is required");
        return 0;
    }
    resultLen = LEN(self) + LEN(arg);
    result = (String *)Object_NewVar(CLASS(String), resultLen + 1);
    if (!result)
        return 0;
    memcpy(STR(result), STR(self), LEN(self));
    memcpy(STR(result) + LEN(self), STR(arg), arg->size);
    return (Unknown *)result;
}

/* OPER_LT */
static Unknown *String_lt(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return String_binaryLogicalOper((String *)self, arg0, OPER_LT);
}

/* OPER_GT */
static Unknown *String_gt(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return String_binaryLogicalOper((String *)self, arg0, OPER_GT);
}

/* OPER_LE */
static Unknown *String_le(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return String_binaryLogicalOper((String *)self, arg0, OPER_LE);
}

/* OPER_GE */
static Unknown *String_ge(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return String_binaryLogicalOper((String *)self, arg0, OPER_GE);
}

/* OPER_EQ */
static Unknown *String_eq(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return String_binaryLogicalOper((String *)self, arg0, OPER_EQ);
}

/* OPER_NE */
static Unknown *String_ne(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return String_binaryLogicalOper((String *)self, arg0, OPER_NE);
}

/* OPER_GET_ITEM */
static Unknown *String_item(Unknown *_self, Unknown *arg0, Unknown *arg1) {
    String *self; Integer *arg;
    ptrdiff_t index;
    
    self = (String *)_self;
    arg = CAST(Integer, arg0);
    if (!arg) {
        Halt(HALT_TYPE_ERROR, "an integer is required");
        return 0;
    }
    index = Integer_AsCPtrdiff(arg);
    if (index < 0 || LEN(self) <= (size_t)index) {
        Halt(HALT_INDEX_ERROR, "index out of range");
        return 0;
    }
    return (Unknown *)Char_FromCChar(STR(self)[index]);
}


/*------------------------------------------------------------------------*/
/* methods -- attributes */

static Unknown *String_size(Unknown *self, Unknown *arg0, Unknown *arg1) {
    return (Unknown *)Integer_FromCPtrdiff(LEN((String *)self));
}


/*------------------------------------------------------------------------*/
/* methods -- enumerating */

static Unknown *String_do(Unknown *_self, Unknown *arg0, Unknown *arg1) {
    String *self;
    size_t i;
    Char *c;
    Unknown *result;
    
    self = (String *)_self;
    for (i = 0; i < LEN(self); ++i) {
        c = Char_FromCChar(STR(self)[i]);
        result = Call(GLOBAL(theInterpreter), arg0, OPER_APPLY, c, 0);
        if (!result)
            return 0; /* unwind */
    }
    return GLOBAL(xvoid);
}


/*------------------------------------------------------------------------*/
/* methods -- printing */

static Unknown *String_printString(Unknown *_self, Unknown *arg0, Unknown *arg1) {
    String *self;
    String *result;
    char *d, *s; const char *subst;
    char c;
    
    self = (String *)_self;
    result = String_FromCStringAndLength(0, 2*LEN(self) + 2);
    d = String_AsCString(result);
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
    result->size = d - STR(result) + 1;
    return (Unknown *)result;
}


/*------------------------------------------------------------------------*/
/* class tmpl */

typedef VariableObjectSubclass StringSubclass;

static MethodTmpl methods[] = {
    /* operators */
#if 0 /*XXX*/
    { "__mul__",    NativeCode_BINARY_OPER | NativeCode_LEAF, &String_mul    },
    { "__mod__",    NativeCode_BINARY_OPER | NativeCode_LEAF, &String_mod    },
#endif
    { "__add__",    NativeCode_BINARY_OPER | NativeCode_LEAF, &String_add    },
    { "__lt__",     NativeCode_BINARY_OPER | NativeCode_LEAF, &String_lt     },
    { "__gt__",     NativeCode_BINARY_OPER | NativeCode_LEAF, &String_gt     },
    { "__le__",     NativeCode_BINARY_OPER | NativeCode_LEAF, &String_le     },
    { "__ge__",     NativeCode_BINARY_OPER | NativeCode_LEAF, &String_ge     },
    { "__eq__",     NativeCode_BINARY_OPER | NativeCode_LEAF, &String_eq     },
    { "__ne__",     NativeCode_BINARY_OPER | NativeCode_LEAF, &String_ne     },
    /* call operators */
    { "__index__", NativeCode_ARGS_1 | NativeCode_LEAF, &String_item },
    /* attributes */
    { "len", NativeCode_ARGS_0, &String_size },
    { "size", NativeCode_ARGS_0, &String_size },
    /* enumerating */
    { "do:", NativeCode_ARGS_1, &String_do },
    /* printing */
    { "printString", NativeCode_ARGS_0, &String_printString },
    { 0 }
};

ClassTmpl ClassStringTmpl = {
    HEART_CLASS_TMPL(String, Object), {
        /*accessors*/ 0,
        methods,
        /*lvalueMethods*/ 0,
        offsetof(StringSubclass, variables),
        sizeof(char)
    }, /*meta*/ {
        0
    }
};


/*------------------------------------------------------------------------*/
/* C API */

String *String_FromCString(const char *str) {
    return String_FromCStringAndLength(str, strlen(str));
}

String *String_FromCStringAndLength(const char *str, size_t len) {
    String *result;
    char *buffer;
    
    result = (String *)Object_NewVar(CLASS(String), len + 1);
    if (!result)
        return 0;
    buffer = STR(result);
    if (str)
        memcpy(buffer, str, len);
    buffer[len] = '\0';
    return result;
}

String *String_FromCStream(FILE *stream, size_t size) {
    String *result;
    char *buffer;
    
    result = (String *)Object_NewVar(CLASS(String), size);
    if (!result)
        return 0;
    buffer = STR(result);
    if (!fgets(buffer, size, stream)) {
        return 0;
    }
    result->size = strlen(buffer) + 1;
    return result;
}

String *String_Concat(String **var, String *newPart) {
    String *self, *result;
    size_t resultLen;
    
    self = *var;
    resultLen = LEN(self) + LEN(newPart);
    result = (String *)Object_NewVar(CLASS(String), resultLen + 1);
    if (!result) {
        *var = 0;
        return 0;
    }
    memcpy(STR(result), STR(self), LEN(self));
    memcpy(STR(result) + LEN(self), STR(newPart), newPart->size);
    *var = result;
    return result;
}

char *String_AsCString(String *self) {
    return STR(self);
}

size_t String_Size(String *self) {
    return self->size;
}

int String_IsEqual(String *a, String *b) {
    return strcmp(STR(a), STR(b)) == 0;
}
