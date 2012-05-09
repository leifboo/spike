
#include "rtl.h"

#include <stdlib.h>


extern struct Behavior MethodContext, BlockContext;


struct Context *SpikeCreateMethodContext(
    size_t minArgumentCount,
    size_t maxArgumentCount,
    int varArgList,
    size_t localCount,
    size_t argumentCount,
    struct Object **arg,
    struct Behavior *methodClass,
    struct Object *receiver,
    struct Object **instVarPointer,
    void *stackp
    )
{
    struct Context *newContext;
    size_t size, n, i;
    
    size = maxArgumentCount + localCount;
    
    newContext = (struct Context *)calloc(1, offsetof(struct Context, var[size]));
    
    newContext->base.klass = &MethodContext;
    newContext->homeContext = newContext;
    newContext->u.m.methodClass     = methodClass;
    newContext->u.m.receiver        = receiver;
    newContext->u.m.instVarPointer  = instVarPointer;
    newContext->u.m.stackp          = stackp;
    
    /* copy & reverse fixed arguments from stack */
    n = argumentCount < maxArgumentCount ? argumentCount : maxArgumentCount;
    for (i = 0; i < n; ++i)
        newContext->var[i] = arg[-(i + 1)];
    
    return newContext;
}


struct Context *SpikeCreateBlockContext(
    void *const startpc,
    size_t nargs,
    struct Context *homeContext
    )
{
    struct Context *newContext;
    
    newContext = (struct Context *)calloc(1, sizeof(struct Context));
    
    newContext->base.klass = &BlockContext;
    newContext->homeContext = homeContext;
    newContext->u.b.nargs = nargs;
    newContext->u.b.pc = startpc;
    
    return newContext;
}
