
#include "rtl.h"

#include <stdlib.h>
#include <string.h>


struct Context *SpikeCreateBlockContext(
    void *const pc,
    size_t argumentCount,
    struct Context *homeContext
    )
{
    struct Context *newContext;
    
    newContext = (struct Context *)calloc(1, offsetof(struct Context, method));
    
    newContext->base.klass = &__spk_x_BlockContext;
    newContext->caller = 0;
    newContext->homeContext = homeContext;
    newContext->argumentCount = argumentCount;
    newContext->pc = pc;
    newContext->sp = 0;
    newContext->regSaveArea[0] = homeContext; /* %rbx */
    newContext->regSaveArea[1] = homeContext->receiver; /* %rsi */
    newContext->regSaveArea[2] = homeContext->instVarPointer; /* %rdi */
    newContext->regSaveArea[3] = 0; /* reserved/unused */
    
    return newContext;
}


struct Context *SpikeCreateClosure(struct Context *blockContext) {
    struct Context *newContext, *homeContext;
    size_t size;
    int i;
    void *reg;
    
    /* copy the home context */
    homeContext = blockContext->homeContext;
    size = offsetof(struct Context, var[homeContext->size]);
    newContext = (struct Context *)malloc(size);
    memcpy(newContext, homeContext, size);
    
    /* reset certain fields */
    newContext->base.klass = &__spk_x_Closure;
    newContext->caller = 0;
    newContext->homeContext = newContext;
    newContext->stackBase = 0;
    
    /* copy certain fields from the block context */
    newContext->argumentCount = blockContext->argumentCount;
    newContext->pc = blockContext->pc;
    newContext->sp = blockContext->sp;
    for (i = 0; i < 4; ++i) {
        reg = blockContext->regSaveArea[i];
        if (reg == blockContext || reg == homeContext)
            reg = newContext;
        newContext->regSaveArea[i] = reg;
    }
    
    return newContext;
}


void SpikeMoveVarArgs(struct Context *context) {
    /*
     * Create a new variable argument array.  Move any excess
     * arguments into the new array.  ("Excess" arguments are those
     * beyond the last fixed argument.)  Slide any fixed arguments to
     * fill-in the resulting gap, so that they reside at known offsets
     * from the Context pointer.
     *
     *
     * On entry:
     *
     *     +------------+ <- context
     *     |            |
     *     | header     |
     *     | (struct    |
     *     |  Context)  |
     *     |            |
     *     |------------|
     *     | var M      |
     *     | ...        | local vars
     *     | var 3      |
     *     | var 2      |
     *     | var 1      |
     *     |------------|
     *     | arg N      |
     *     | arg N-1    |
     *     | ...        |
     *     | arg F+1    |
     *     | arg F      | reversed arguments pushed by caller
     *     | arg F-1    |
     *     | ...        |
     *     | arg 2      |
     *     | arg 1      |
     *     +------------+
     *     | receiver   | receiver / space for result
     *     +------------+
     *
     *
     * On exit:
     *
     *     +------------+ <- context
     *     |            |
     *     | header     |
     *     | (struct    |
     *     |  Context)  |
     *     |            |
     *     |------------|
     *     | var M      |
     *     | ...        | other local vars
     *     | var 3      |
     *     | var 2      |
     *     |------------|
     *     | var 1      | -> argument array with arg F+1 ... arg N
     *     |------------|
     *     | arg F      |
     *     | arg F-1    |
     *     | ...        | fixed arguments
     *     | arg 2      |
     *     | arg 1      |
     *     |------------|
     *     |            |
     *     | ...        | vacated and cleared space (N - F)
     *     |            |
     *     +------------+
     *     | receiver   | receiver / space for result
     *     +------------+
     *
     */
    
    struct Object **arg;
    struct Method *method;
    struct Array *varArgArray;
    size_t fixedArgCount, varArgCount, i;
    
    method = context->method;
    
    fixedArgCount = method->maxArgumentCount & 0x7FFFFFFF;
    if (context->argumentCount > fixedArgCount)
        varArgCount = context->argumentCount - fixedArgCount;
    else
        varArgCount = 0;
    
    /* create the variable argument array */
    varArgArray = (struct Array *)calloc(1, offsetof(struct Array, item[varArgCount]));
    varArgArray->base.klass = &__spk_x_Array;
    varArgArray->size = varArgCount;
    
    /* copy & reverse arguments from stack */
    arg = &context->var[method->localCount];
    for (i = 0; i < varArgCount; ++i)
        varArgArray->item[i] = arg[varArgCount - i - 1];
    
    /* initialize the "...args" local variable */
    context->var[method->localCount - 1] = (struct Object *)varArgArray;
    
    /* slide the fixed arguments */
    for (i = 0; i < fixedArgCount; ++i)
        arg[i] = arg[i + varArgCount];
    
    /* clear the vacated space */
    arg += fixedArgCount;
    for (i = 0; i < varArgCount; ++i)
        arg[i] = 0;
}
