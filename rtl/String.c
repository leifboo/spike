
#include "rtl.h"

#include "oper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct Object Object;


struct String {
    Object base;
    size_t size;
    char str[1];
};


extern void SpikeError(Object *);


extern struct Behavior String;
extern Object __sym_typeError, __sym_rangeError;
extern Object false, true;


#define BOOL(cond) ((cond) ? &true : &false)
#define STR(op) ((char *)(op)->str)
#define LEN(op) ((op)->size - 1)

#define CAST(c, op) ((op)->klass == &c ? (struct c *)(op) : 0)


/*------------------------------------------------------------------------*/
/* method helpers */

static Object *String_binaryLogicalOper(struct String *self, Object *arg0, Oper oper) {
    struct String *arg;
    Object *result;
    
    arg = CAST(String, arg0);
    if (!arg) {
        switch (oper) {
        case OPER_EQ:
            /* XXX: 0 == 0.0 */
            return &false;
        case OPER_NE:
            return &true;
        default:
            SpikeError(&__sym_typeError);
            return 0;
        }
    }
    
    switch (oper) {
    case OPER_LT: result = BOOL(strcmp(STR(self), STR(arg)) < 0);  break;
    case OPER_GT: result = BOOL(strcmp(STR(self), STR(arg)) > 0);  break;
    case OPER_LE: result = BOOL(strcmp(STR(self), STR(arg)) <= 0); break;
    case OPER_GE: result = BOOL(strcmp(STR(self), STR(arg)) >= 0); break;
    case OPER_EQ: result = BOOL(strcmp(STR(self), STR(arg)) == 0); break;
    case OPER_NE: result = BOOL(strcmp(STR(self), STR(arg)) != 0); break;
    default: result = 0; /* not reached */
    }
    return result;
}


static struct String *String_fromCStringAndLength(const char *str, size_t len) {
    struct String *result;
    char *buffer;
    
    result = (struct String *)malloc(sizeof(struct String) + len);
    if (!result)
        return 0;
    result->base.klass = &String;
    result->size = len + 1;
    buffer = STR(result);
    if (str)
        memcpy(buffer, str, len);
    buffer[len] = '\0';
    return result;
}


/*------------------------------------------------------------------------*/
/* methods -- operators */

/* OPER_ADD */
Object *String_add(struct String *self, Object *arg0) {
    struct String *arg, *result;
    size_t resultLen;
    
    arg = CAST(String, arg0);
    if (!arg) {
        SpikeError(&__sym_typeError);
        return 0;
    }
    resultLen = LEN(self) + LEN(arg);
    result = (struct String *)malloc(sizeof(struct String) + resultLen);
    if (!result)
        return 0;
    result->base.klass = &String;
    result->size = resultLen + 1;
    memcpy(STR(result), STR(self), LEN(self));
    memcpy(STR(result) + LEN(self), STR(arg), arg->size);
    return (Object *)result;
}

/* OPER_LT */
Object *String_lt(struct String *self, Object *arg0) {
    return String_binaryLogicalOper(self, arg0, OPER_LT);
}

/* OPER_GT */
Object *String_gt(struct String *self, Object *arg0) {
    return String_binaryLogicalOper(self, arg0, OPER_GT);
}

/* OPER_LE */
Object *String_le(struct String *self, Object *arg0) {
    return String_binaryLogicalOper(self, arg0, OPER_LE);
}

/* OPER_GE */
Object *String_ge(struct String *self, Object *arg0) {
    return String_binaryLogicalOper(self, arg0, OPER_GE);
}

/* OPER_EQ */
Object *String_eq(struct String *self, Object *arg0) {
    return String_binaryLogicalOper(self, arg0, OPER_EQ);
}

/* OPER_NE */
Object *String_ne(struct String *self, Object *arg0) {
    return String_binaryLogicalOper(self, arg0, OPER_NE);
}

/* OPER_GET_ITEM */
Object *String_item(struct String *self, int index) {
#if 0 /* XXX: already unboxed -- no type check */
    if ((index & 3) != 2) {
        SpikeError(&__sym_typeError);
        return 0;
    }
    index >>= 2;
#endif
    if (index < 0 || LEN(self) <= (size_t)index) {
        SpikeError(&__sym_rangeError);
        return 0;
    }
    // XXX
    //return (Object *)Char_FromCChar(STR(self)[index]);
    return (Object *)((STR(self)[index] << 2) | 2);
}


/*------------------------------------------------------------------------*/
/* methods -- attributes */

int String_size(struct String *self) {
    return LEN(self);
}


/*------------------------------------------------------------------------*/
/* methods -- printing */

Object *String_printString(struct String *self) {
    struct String *result;
    char *d, *s; const char *subst;
    char c;
    
    result = String_fromCStringAndLength(0, 2*LEN(self) + 2);
    d = STR(result);
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
    return (Object *)result;
}


Object *String_fromInteger(int anInteger) {
    struct String *result;
    char *str;
    
#if 0 /* XXX: already unboxed -- no type check */
    if ((anInteger & 3) != 2) {
        SpikeError(&__sym_typeError);
        return 0;
    }
    anInteger >>= 2;
#endif
    result = String_fromCStringAndLength(0, 4*sizeof(anInteger));
    if (!result)
        return 0;
    str = STR(result);
    sprintf(str, "%d", anInteger);
    result->size = strlen(str) + 1;
    return (Object *)result;
}
