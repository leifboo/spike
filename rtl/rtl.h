
#ifndef __spike_rtl_h__
#define __spike_rtl_h__


#include <stddef.h>


struct Object {
    struct Behavior *klass;
};


struct Behavior {
    struct Object base;
    struct Behavior *superclass;
    struct Array *methodTable[2];
    size_t instVarCount;
};


struct Symbol {
    struct Object base;
    size_t hash;
    char str[1];
};


struct Method {
    struct Object base;
    size_t minArgumentCount;
    size_t maxArgumentCount;
    size_t localCount;
    char code[1];
};


struct Context {
    struct Object base;
    struct Context *homeContext;            /* %ebp */
    union {
        struct /* MethodContext */ {
            struct Behavior *methodClass;   /* %ebx */
            struct Object *receiver;        /* %esi */
            struct Object **instVarPointer; /* %edi */
            void *stackp;                   /* %esp */
        } m;
        struct /* BlockContext */ {
            size_t nargs;
            void *pc;
        } b;
    } u;
    struct Object *var[1];
};


struct Message {
    struct Object base;
    unsigned int ns; /* boxed */
    struct Symbol *selector;
    struct Array *arguments;
};


struct Array {
    struct Object base;
    size_t size;
    struct Object *item[1];
};


extern struct Method *SpikeLookupMethod(struct Behavior *behavior, unsigned int ns, struct Symbol *selector);


#endif /* __spike_rtl_h__ */
