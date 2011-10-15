
#ifndef __spike_rtl_h__
#define __spike_rtl_h__


#include <stddef.h>


typedef struct Array    Array;
typedef struct Behavior Behavior;
typedef struct Context  Context;
typedef struct Message  Message;
typedef struct Method   Method;
typedef struct Object   Object;
typedef struct Symbol   Symbol;


struct Object {
    Behavior *klass;
};


struct Behavior {
    Object base;
    Behavior *superclass;
    Array *methodTable[2];
    size_t instVarCount;
};


struct Symbol {
    Object base;
    size_t hash;
    char str[1];
};


struct Method {
    Object base;
    size_t minArgumentCount;
    size_t maxArgumentCount;
    size_t localCount;
    char code[1];
};


struct Context {
    Object base;
    Context *homeContext;            /* %ebp */
    union {
        struct /* MethodContext */ {
            Behavior *methodClass;   /* %ebx */
            Object *receiver;        /* %esi */
            Object **instVarPointer; /* %edi */
            void *stackp;            /* %esp */
        } m;
        struct /* BlockContext */ {
            size_t nargs;
            void *pc;
        } b;
    } u;
    Object *var[1];
};


struct Message {
    Object base;
    unsigned int ns;
    Symbol *selector;
    Array *arguments;
};


struct Array {
    Object base;
    size_t size;
    Object *item[1];
};


extern Method *SpikeLookupMethod(Behavior *behavior, unsigned int ns, Symbol *selector);


#endif /* __spike_rtl_h__ */
