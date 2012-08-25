
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


struct Context {  /* <- %ebp */
    struct Object base;
    struct Context *caller;                 /*  4 saved %ebp */
    struct Context *homeContext;            /*  8 %ebx */
    size_t argumentCount;                   /* 12 */
    void *pc;                               /* 16 %eip (return address) */
    void *sp;                               /* 20 %esp */
    void *regSaveArea[4];                   /* 24 %ebx, %esi, %edi, (reserved) */
    /* the following are not present for BlockContext */
    struct Method *method;                  /* 40 */
    struct Behavior *methodClass;           /* 44 */
    struct Object *receiver;                /* 48 %esi */
    struct Object **instVarPointer;         /* 52 %edi */
    void *stackBase;                        /* 56 base/bottom %esp */
    size_t size;                            /* 60 size of 'var' array */
    struct Object *var[1];                  /* 64 */
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
extern struct Symbol *SpikeFindSelectorOfMethod(struct Behavior *behavior, struct Method *method);
extern struct Object *SpikeCast(struct Behavior *target, struct Object *object);


extern struct Behavior
    __spk_x_Array,
    __spk_x_BlockContext,
    __spk_x_Char,
    __spk_x_Class,
    __spk_x_Closure,
    __spk_x_Float,
    __spk_x_Message,
    __spk_x_Metaclass,
    __spk_x_String;

extern struct Object __spk_sym_rangeError, __spk_sym_typeError;

extern struct Object __spk_x_false, __spk_x_true;


#define CAST(c, o) ((struct c *)SpikeCast(&(__spk_x_ ## c), (struct Object *)(o)))


#endif /* __spike_rtl_h__ */
