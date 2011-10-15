
#include "rtl.h"

#include <stdlib.h>


extern Behavior MethodContext, BlockContext;


Context *SpikeCreateMethodContext(
    size_t minArgumentCount,
    size_t maxArgumentCount,
    int varArgList,
    size_t localCount,
    size_t argumentCount,
    Object **arg,
    Behavior *methodClass,
    Object *receiver,
    Object **instVarPointer,
    void *stackp
    )
{
    Context *newContext;
    size_t size, n, i;
    
    size = maxArgumentCount + localCount;
    
    newContext = (Context *)calloc(1, sizeof(Context) + size*sizeof(Object *));
    
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


Context *SpikeCreateBlockContext(void *const startpc, size_t nargs, Context *homeContext) {
    Context *newContext;
    
    newContext = (Context *)calloc(1, sizeof(Context));
    
    newContext->base.klass = &BlockContext;
    newContext->homeContext = homeContext;
    newContext->u.b.nargs = nargs;
    newContext->u.b.pc = startpc;
    
    return newContext;
}
