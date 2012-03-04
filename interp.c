
#include "interp.h"

#include "behavior.h"
#include "bool.h"
#include "class.h"
#include "heart.h"
#include "host.h"
#include "module.h"
#include "native.h"
#include "rodata.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*------------------------------------------------------------------------*/
/* class struct definitions */

struct Context {
    VariableObject base;
    Context *caller; /* a.k.a. "sender" */
    Opcode *pc;
    Unknown **stackp;
    Context *homeContext;
    union {
        struct /* MethodContext */ {
            Method *method;
            struct Behavior *methodClass;
            Unknown *receiver;
            Unknown **framep;
        } m;
        struct /* BlockContext */ {
            size_t index;
            size_t nargs;
            Opcode *startpc;
        } b;
    } u;
    int *mark;
};


struct Semaphore {
    Object base;
    Fiber *firstLink;
    Fiber *lastLink;
    int excessSignals;
};


struct Fiber {
    Object base;
    Fiber *nextLink;
    Context *suspendedContext;
    Context *leafContext;
    int priority;
    Semaphore *myList;
};


struct ProcessorScheduler {
    Object base;
    Semaphore **quiescentFiberLists;
    Fiber *activeFiber;
};


struct Interpreter {
    Object base;

    /* fibers */
    ProcessorScheduler *scheduler;
    Semaphore *theInterruptSemaphore;
    int interruptPending;

    /* contexts */
    Context *activeContext;
    Context *newContext;

    /* error handling */
    int printingStack;

};


/*------------------------------------------------------------------------*/

/*** TODO ***/
#define XXX 0
#define HIGHEST_PRIORITY XXX


/* Context pseudo-fields */
#define sender caller


/*------------------------------------------------------------------------*/
/* class templates */

typedef struct MessageSubclass {
    Message base;
    Unknown *variables[1]; /* stretchy */
} MessageSubclass;

static AccessorTmpl MessageAccessors[] = {
    { "selector",  T_OBJECT, offsetof(Message, selector),  Accessor_READ },
    { "arguments", T_OBJECT, offsetof(Message, arguments), Accessor_READ },
    { 0 }
};

static MethodTmpl MessageMethods[] = {
    { 0 }
};

ClassTmpl ClassMessageTmpl = {
    HEART_CLASS_TMPL(Message, Object), {
        MessageAccessors,
        MessageMethods,
        /*lvalueMethods*/ 0,
        offsetof(MessageSubclass, variables)
    }, /*meta*/ {
        0
    }
};


static void Method_zero(Object *_self) {
    Method *self = (Method *)_self;
    (*CLASS(Method)->superclass->zero)(_self);
    self->debug.source = 0;
    self->debug.lineCodeTally = 0;
    self->debug.lineCodes = 0;
    self->nativeCode = 0;
    return;
}

static void Method_dealloc(Object *_self, Unknown **l) {
    Method *self = (Method *)_self;
    XLDECREF(self->debug.source, l);
    free(self->debug.lineCodes);
    (*CLASS(Method)->superclass->dealloc)(_self, l);
}

typedef struct MethodSubclass {
    Method base;
    Unknown *variables[1]; /* stretchy */
} MethodSubclass;

static MethodTmpl MethodMethods[] = {
    { 0 }
};

ClassTmpl ClassMethodTmpl = {
    HEART_CLASS_TMPL(Method, VariableObject), {
        /*accessors*/ 0,
        MethodMethods,
        /*lvalueMethods*/ 0,
        offsetof(MethodSubclass, variables),
        sizeof(Opcode),
        &Method_zero,
        &Method_dealloc
    }, /*meta*/ {
        0
    }
};


static void Context_zero(Object *);
static void Context_dealloc(Object *, Unknown **);
static Unknown *Context_blockCopy(Unknown *, Unknown *, Unknown *);
static Unknown *Context_compoundExpression(Unknown *, Unknown *, Unknown *);

typedef struct ContextSubclass {
    Context base;
    Unknown *variables[1]; /* stretchy */
} ContextSubclass;

static MethodTmpl ContextMethods[] = {
    { "blockCopy", NativeCode_ARGS_2, &Context_blockCopy },
    { "compoundExpression", NativeCode_ARGS_ARRAY, &Context_compoundExpression },
    { 0 }
};

ClassTmpl ClassContextTmpl = {
    HEART_CLASS_TMPL(Context, VariableObject), {
        /*accessors*/ 0,
        ContextMethods,
        /*lvalueMethods*/ 0,
        offsetof(ContextSubclass, variables),
        sizeof(Unknown *),
        &Context_zero,
        &Context_dealloc
    }, /*meta*/ {
        0
    }
};


static MethodTmpl MethodContextMethods[] = {
    { 0 }
};

ClassTmpl ClassMethodContextTmpl = {
    HEART_CLASS_TMPL(MethodContext, Context), {
        /*accessors*/ 0,
        MethodContextMethods,
        /*lvalueMethods*/ 0,
        offsetof(ContextSubclass, variables),
        sizeof(Unknown *)
    }, /*meta*/ {
        0
    }
};


static AccessorTmpl BlockContextAccessors[] = {
    { "numArgs", T_SIZE, offsetof(Context, u.b.nargs), Accessor_READ },
    { 0 }
};

static MethodTmpl BlockContextMethods[] = {
    { 0 }
};

ClassTmpl ClassBlockContextTmpl = {
    HEART_CLASS_TMPL(BlockContext, Context), {
        BlockContextAccessors,
        BlockContextMethods,
        /*lvalueMethods*/ 0,
        offsetof(ContextSubclass, variables),
        sizeof(Unknown *)
    }, /*meta*/ {
        0
    }
};


static MethodTmpl NullMethods[] = {
    { 0 }
};

ClassTmpl ClassNullTmpl = {
    HEART_CLASS_TMPL(Null, Object), {
        /*accessors*/ 0,
        NullMethods,
        /*lvalueMethods*/ 0
    }, /*meta*/ {
        0
    }
};


static MethodTmpl UninitMethods[] = {
    { 0 }
};

ClassTmpl ClassUninitTmpl = {
    HEART_CLASS_TMPL(Uninit, Object), {
        /*accessors*/ 0,
        UninitMethods,
        /*lvalueMethods*/ 0
    }, /*meta*/ {
        0
    }
};


static MethodTmpl VoidMethods[] = {
    { 0 }
};

ClassTmpl ClassVoidTmpl = {
    HEART_CLASS_TMPL(Void, Object), {
        /*accessors*/ 0,
        VoidMethods,
        /*lvalueMethods*/ 0
    }, /*meta*/ {
        0
    }
};


static void Interpreter_zero(Object *_self) {
    Interpreter *self = (Interpreter *)_self;
    (*CLASS(Interpreter)->superclass->zero)(_self);
    self->scheduler = 0;
    self->theInterruptSemaphore = 0;
    self->interruptPending = 0;
    self->activeContext = 0;
    self->newContext = 0;
    self->printingStack = 0;
    return;
}

static void Interpreter_dealloc(Object *_self, Unknown **l) {
    Interpreter *self = (Interpreter *)_self;
    LDECREF(self->scheduler, l);
    XLDECREF(self->theInterruptSemaphore, l); /* XXX: no semaphore yet */
    XLDECREF(self->activeContext, l);
    XLDECREF(self->newContext, l);
    (*CLASS(Interpreter)->superclass->dealloc)(_self, l);
}

typedef struct InterpreterSubclass {
    Interpreter base;
    Unknown *variables[1]; /* stretchy */
} InterpreterSubclass;

static MethodTmpl InterpreterMethods[] = {
    { 0 }
};

ClassTmpl ClassInterpreterTmpl = {
    HEART_CLASS_TMPL(Interpreter, Object), {
        /*accessors*/ 0,
        InterpreterMethods,
        /*lvalueMethods*/ 0,
        offsetof(InterpreterSubclass, variables),
        /*itemSize*/ 0,
        &Interpreter_zero,
        &Interpreter_dealloc
    }, /*meta*/ {
        0
    }
};


static void ProcessorScheduler_zero(Object *_self) {
    ProcessorScheduler *self = (ProcessorScheduler *)_self;
    (*CLASS(ProcessorScheduler)->superclass->zero)(_self);
    self->quiescentFiberLists = 0;
    self->activeFiber = 0;
    return;
}

static void ProcessorScheduler_dealloc(Object *_self, Unknown **l) {
    ProcessorScheduler *self = (ProcessorScheduler *)_self;
    LDECREF(self->activeFiber, l);
    (*CLASS(ProcessorScheduler)->superclass->dealloc)(_self, l);
}

typedef struct ProcessorSchedulerSubclass {
    ProcessorScheduler base;
    Unknown *variables[1]; /* stretchy */
} ProcessorSchedulerSubclass;

static MethodTmpl ProcessorSchedulerMethods[] = {
    { 0 }
};

ClassTmpl ClassProcessorSchedulerTmpl = {
    HEART_CLASS_TMPL(ProcessorScheduler, Object), {
        /*accessors*/ 0,
        ProcessorSchedulerMethods,
        /*lvalueMethods*/ 0,
        offsetof(ProcessorSchedulerSubclass, variables),
        /*itemSize*/ 0,
        &ProcessorScheduler_zero,
        &ProcessorScheduler_dealloc
    }, /*meta*/ {
        0
    }
};


static void Fiber_zero(Object *_self) {
    Fiber *self = (Fiber *)_self;
    (*CLASS(Fiber)->superclass->zero)(_self);
    self->nextLink = 0;
    self->suspendedContext = 0;
    self->leafContext = 0;
    self->priority = 0;
    self->myList = 0;
    return;
}

static void Fiber_dealloc(Object *_self, Unknown **l) {
    Fiber *self = (Fiber *)_self;
    XLDECREF(self->nextLink, l);
    XLDECREF(self->suspendedContext, l);
    LDECREF(self->leafContext, l);
    XLDECREF(self->myList, l);
    (*CLASS(Fiber)->superclass->dealloc)(_self, l);
}

typedef struct FiberSubclass {
    Fiber base;
    Unknown *variables[1]; /* stretchy */
} FiberSubclass;

static MethodTmpl FiberMethods[] = {
    { 0 }
};

ClassTmpl ClassFiberTmpl = {
    HEART_CLASS_TMPL(Fiber, Object), {
        /*accessors*/ 0,
        FiberMethods,
        /*lvalueMethods*/ 0,
        offsetof(FiberSubclass, variables),
        /*itemSize*/ 0,
        &Fiber_zero,
        &Fiber_dealloc
    }, /*meta*/ {
        0
    }
};


/*------------------------------------------------------------------------*/
/* initialization */

void Interpreter_Boot(void) {
    Method *callBlock;
    
    callBlock = Method_New(1);
    Method_OPCODES(callBlock)[0] = OPCODE_CALL_BLOCK;
    Behavior_InsertMethod(CLASS(BlockContext), METHOD_NAMESPACE_RVALUE, __apply__, callBlock);
    DECREF(callBlock);
}

Interpreter *Interpreter_New(void) {
    size_t contextSize;
    Context *leafContext;
    Fiber *fiber;
    ProcessorScheduler *scheduler;
    Interpreter *newInterpreter;
    
    contextSize = LEAF_MAX_STACK_SIZE +
                  LEAF_MAX_ARGUMENT_COUNT;
    ++contextSize; /* XXX: doesNotUnderstand overhead */
    leafContext = Context_New(contextSize);
    leafContext->pc = 0;
    leafContext->stackp = 0;
    leafContext->homeContext = 0;
    leafContext->u.m.method = 0;
    leafContext->u.m.methodClass = 0;
    leafContext->u.m.receiver = 0;
    leafContext->u.m.framep = 0;
    
    fiber =(Fiber *)Object_New(CLASS(Fiber));
    fiber->nextLink = 0;
    fiber->suspendedContext = 0;
    fiber->leafContext = leafContext; /* steal reference */
    fiber->priority = 0;
    fiber->myList = 0;
    
    scheduler =(ProcessorScheduler *)Object_New(CLASS(ProcessorScheduler));
    scheduler->quiescentFiberLists = 0;
    scheduler->activeFiber = fiber; /* steal reference */
    
    newInterpreter = (Interpreter *)Object_New(CLASS(Interpreter));
    Interpreter_Init(newInterpreter, scheduler);
    DECREF(scheduler);
    
    return newInterpreter;
}

void Interpreter_Init(Interpreter *self, ProcessorScheduler *aScheduler) {
    /* fibers */
    self->scheduler = aScheduler;  INCREF(aScheduler);
    self->theInterruptSemaphore = Semaphore_New();
    self->interruptPending = 0;

    /* contexts */
    self->activeContext = self->scheduler->activeFiber->suspendedContext;
    XINCREF(self->activeContext);
    self->newContext = 0;

    /* error handling */
    self->printingStack = 0;
}

Message *Message_New() {
    return (Message *)Object_New(CLASS(Message));
}

Method *Method_New(size_t size) {
    return (Method *)Object_NewVar(CLASS(Method), size);
}


/*------------------------------------------------------------------------*/
/* contexts */

Context *Context_New(size_t size) {
    return (Context *)Object_NewVar(CLASS(MethodContext), size);
}

static void Context_zero(Object *_self) {
    Context *self = (Context *)_self;
    Unknown **p;
    size_t count;
    
    (*CLASS(Context)->superclass->zero)(_self);
    
    self->caller = 0;
    self->pc = 0;
    self->stackp = 0;
    self->homeContext = 0;
    
    memset(&self->u, 0, sizeof(self->u));
    
    for (count = 0, p = Context_VARIABLES(self); count < self->base.size; ++count, ++p) {
        INCREF(GLOBAL(uninit));
        *p = GLOBAL(uninit);
    }
    
    self->mark = 0;
    
    return;
}

static void Context_dealloc(Object *_self, Unknown **l) {
    Context *self;
    Unknown **p;
    size_t count;
    
    self = (Context *)_self;
    
    for (count = 0, p = Context_VARIABLES(self);
         count < self->base.size;
         ++count, ++p)
    {
        LDECREF(*p, l);
    }
    
    XLDECREF(self->caller, l);
    if (self->homeContext) { /* XXX: shady */
        /* block context */
        LDECREF(self->homeContext, l);
    } else {
        /* method context */
        /* These might be null in a leaf context. */
        XLDECREF(self->u.m.method, l);
        XLDECREF(self->u.m.methodClass, l);
        XLDECREF(self->u.m.receiver, l);
    }
    
    (*CLASS(Context)->superclass->dealloc)(_self, l);
}

static Unknown *Context_blockCopy(Unknown *_self, Unknown *arg0, Unknown *arg1) {
    Context *self;
    size_t index, numArgs;
    size_t size;
    Context *newContext;
    
    self = (Context *)_self;
    if (!Host_IsInteger(arg0) || !Host_IsInteger(arg1)) {
        Halt(HALT_TYPE_ERROR, "an integer object is required");
        return 0;
    }
    index = (size_t)Host_IntegerAsCLong(arg0);
    numArgs = (size_t)Host_IntegerAsCLong(arg1);
    
    /* The compiler guarantees that the home context is at least as
       big as the biggest child block context needs to be. */
    size = self->homeContext->base.size;
    
    newContext = Context_New(size);
    INCREF(CLASS(BlockContext));
    DECREF(newContext->base.base.klass);
    newContext->base.base.klass = CLASS(BlockContext);
    newContext->caller = 0;
    newContext->stackp = &Context_VARIABLES(newContext)[newContext->base.size];
    newContext->homeContext = self->homeContext;  INCREF(self->homeContext);
    newContext->u.b.index = index;
    newContext->u.b.nargs = numArgs;
    newContext->u.b.startpc = newContext->pc = self->pc + 3; /* skip branch */
    
    return (Unknown *)newContext;
}

static Unknown *Context_compoundExpression(Unknown *_self, Unknown *arg0, Unknown *arg1) {
    Context *self = (Context *)_self;
    return SendWithArguments(GLOBAL(theInterpreter),
                                 self->homeContext->u.m.receiver,
                                 compoundExpression,
                                 arg0);
}


/*------------------------------------------------------------------------*/
/* interpreter loop */

/* stackPointer */
static Unknown *popObject(Unknown **stackPointer) {
    Unknown *tmp = stackPointer[-1];
    INCREF(GLOBAL(uninit));
    stackPointer[-1] = GLOBAL(uninit);
    return tmp;
}

#define POP_OBJECT() (popObject(++stackPointer))

#define POP(nItems) \
do { Unknown **end = stackPointer + (nItems); \
     while (stackPointer < end) { \
         INCREF(GLOBAL(uninit)); \
         *stackPointer++ = GLOBAL(uninit); } } while (0)

#define CLEAN(sp, nItems) \
do { Unknown **end = sp + (nItems); \
     while (sp < end) { \
         Unknown *op = *sp; \
         INCREF(GLOBAL(uninit)); \
         *sp++ = GLOBAL(uninit); \
         DECREF(op); } } while (0)

#define PUSH(object) \
do { --stackPointer; \
     DECREF(stackPointer[0]); \
     stackPointer[0] = (Unknown *)(object); } while (0)

#define STACK_TOP() (*stackPointer)

#define INSTANCE_VARS(op, mc) ((Unknown **)((char *)(op) + GET_CLASS(op)->instVarOffset) + (mc)->instVarBaseIndex)


#define DECODE_UINT(result) do { \
    Opcode byte; \
    unsigned int shift = 0; \
    (result) = 0; \
    while (1) { \
        byte = *instructionPointer++; \
        (result) |= ((byte & 0x7F) << shift); \
        if ((byte & 0x80) == 0) { \
            break; \
        } \
        shift += 7; \
    } \
} while (0)

#define DECODE_SINT(result) do { \
    Opcode byte; \
    unsigned int shift = 0; \
    (result) = 0; \
    do { \
        byte = *instructionPointer++; \
        (result) |= ((byte & 0x7F) << shift); \
        shift += 7; \
    } while (byte & 0x80); \
    if (shift < 8*sizeof((result)) && (byte & 0x40)) { \
        /* sign extend */ \
        (result) |= - (1 << shift); \
    } \
} while (0)

/* XXX: What about leaf methods? */
#define TRAP(selector, argument) \
    self->activeContext->pc = instructionPointer; \
    self->activeContext->stackp = stackPointer; \
    trap(self, selector, argument); \
    return 0


#define GET_CLASS(op) (((Object *)(op))->klass)


static Fiber *trap(Interpreter *, Unknown *, Unknown *);
static void trapUnknownOpcode(Interpreter *);
static void halt(Interpreter *, Unknown *, Unknown *);


Unknown *Interpreter_Interpret(Interpreter *self) {

    /* context registers */
    Context *homeContext, *leafContext;
    Unknown *receiver;
    Method *method;
    Behavior *methodClass, *mc;
    register Opcode *instructionPointer;
    register Unknown **stackPointer;
    register Unknown **framePointer;
    register Unknown **instVarPointer;
    register Unknown **globalPointer;
    register Unknown **literalPointer;

    size_t index;
    Unknown *tmp;

    /* message sending */
    MethodNamespace ns;
    Unknown *selector = 0; /* ref counted */
    size_t argumentCount = 0, varArg = 0, variadic = 0, fixedArgumentCount = 0;
    unsigned int oper;
    Opcode *oldIP;
    
    /* unwinding */
    int mark = 666;
    
    self->activeContext->mark = &mark;
    assert(!self->activeContext->sender || self->activeContext->sender->mark); /* XXX */
    
 fetchContextRegisters:
    homeContext = self->activeContext->homeContext;
    receiver = homeContext->u.m.receiver;
    method = homeContext->u.m.method;
    methodClass = homeContext->u.m.methodClass;
    instructionPointer = self->activeContext->pc;
    stackPointer = self->activeContext->stackp;
    framePointer = homeContext->u.m.framep;
    instVarPointer = INSTANCE_VARS(receiver, methodClass);
    if (methodClass->module) {
        globalPointer = Module_VARIABLES(methodClass->module);
        literalPointer = Module_LITERALS(methodClass->module);
    } else {
        globalPointer = 0;
        literalPointer = 0;
    }
    assert(self->newContext == 0);
    
    leafContext = self->scheduler->activeFiber->leafContext;
    
 checkForInterrupts:
    if (self->interruptPending) {
        self->interruptPending = 0;
        Interpreter_SynchronousSignal(self, self->theInterruptSemaphore);
        if (self->newContext) {
            DECREF(self->activeContext);
            self->activeContext = self->newContext;
            self->newContext = 0;
            goto fetchContextRegisters;
        }
    }

 loop:
    while (1) {
        
        switch (*instructionPointer++) {
            
        default:
            --instructionPointer;
            trapUnknownOpcode(self);
            break;
            
        case OPCODE_NOP:
            break;

            /*** push opcodes ***/
        case OPCODE_PUSH_LOCAL:
            DECODE_UINT(index);
            tmp = framePointer[index];
            INCREF(tmp);
            PUSH(tmp);
            break;
        case OPCODE_PUSH_INST_VAR:
            DECODE_UINT(index);
            tmp = instVarPointer[index];
            INCREF(tmp);
            PUSH(tmp);
            break;
        case OPCODE_PUSH_GLOBAL:
            DECODE_UINT(index);
            tmp = globalPointer[index];
            INCREF(tmp);
            PUSH(tmp);
            break;
        case OPCODE_PUSH_LITERAL:
            DECODE_UINT(index);
            tmp = literalPointer[index];
            INCREF(tmp);
            PUSH(tmp);
            break;
        case OPCODE_PUSH_SUPER:
            /* OPCODE_PUSH_SUPER is a pseudo-op equivalent to
             * OPCODE_PUSH_SELF.  The receiver is always pushed onto
             * the stack so that the stack clean-up machinery doesn't
             * have to distinguish between sends and super-sends.
             */
        case OPCODE_PUSH_SELF:     PUSH(receiver);             INCREF(receiver);             break;
        case OPCODE_PUSH_FALSE:    PUSH(GLOBAL(xfalse));   INCREF(GLOBAL(xfalse));   break;
        case OPCODE_PUSH_TRUE:     PUSH(GLOBAL(xtrue));    INCREF(GLOBAL(xtrue));    break;
        case OPCODE_PUSH_NULL:     PUSH(GLOBAL(null));     INCREF(GLOBAL(null));     break;
        case OPCODE_PUSH_VOID:     PUSH(GLOBAL(xvoid));    INCREF(GLOBAL(xvoid));    break;
        case OPCODE_PUSH_CONTEXT:  PUSH(self->activeContext);  INCREF(self->activeContext);  break;
                
            /*** store opcodes ***/
        case OPCODE_STORE_LOCAL:
            DECODE_UINT(index);
            tmp = STACK_TOP();
            INCREF(tmp);
            DECREF(framePointer[index]);
            framePointer[index] = tmp;
            break;
        case OPCODE_STORE_INST_VAR:
            DECODE_UINT(index);
            tmp = STACK_TOP();
            INCREF(tmp);
            DECREF(instVarPointer[index]);
            instVarPointer[index] = tmp;
            break;
        case OPCODE_STORE_GLOBAL:
            DECODE_UINT(index);
            tmp = STACK_TOP();
            INCREF(tmp);
            DECREF(globalPointer[index]);
            globalPointer[index] = tmp;
            break;
            
            /*** additional stack opcodes ***/
        case OPCODE_DUP:
            tmp = STACK_TOP();
            INCREF(tmp);
            PUSH(tmp);
            break;
        case OPCODE_DUP_N: {
            Unknown **s;
            size_t n;
            DECODE_UINT(n);
            s = stackPointer + n;
            while (n--) {
                tmp = *--s;
                INCREF(tmp);
                *--stackPointer = tmp;
            }
            break; }
        case OPCODE_POP:
            DECREF(STACK_TOP());
            POP(1);
            break;
        case OPCODE_ROT: {
            Unknown **s, **end;
            size_t n;
            DECODE_UINT(n);
            if (n > 1) {
                tmp = STACK_TOP();
                s = stackPointer;
                end = stackPointer + n - 1;
                while (s < end) {
                    s[0] = s[1];
                    ++s;
                }
                *s = tmp;
            }
            break; }

            /*** branch opcodes ***/
        case OPCODE_BRANCH_IF_FALSE: {
            ptrdiff_t displacement;
            Unknown *x, *o, *boolean;
            x = GLOBAL(xfalse);
            o = GLOBAL(xtrue);
            goto branch;
        case OPCODE_BRANCH_IF_TRUE:
            x = GLOBAL(xtrue);
            o = GLOBAL(xfalse);
 branch:
            boolean = POP_OBJECT();
            if (boolean == x) {
                Opcode *base;
                DECREF(boolean);
            case OPCODE_BRANCH_ALWAYS:
                base = instructionPointer - 1;
                DECODE_SINT(displacement);
                instructionPointer = base + displacement;
                if (displacement < 0) {
                    goto checkForInterrupts;
                }
            } else if (boolean != o) {
                --instructionPointer;
                TRAP(mustBeBoolean, 0);
            } else {
                DECREF(boolean);
                DECODE_SINT(displacement);
            }
            break; }
            
            /* identity comparison opcode */
        case OPCODE_ID: {
            Unknown *x, *y, *boolean;
            x = POP_OBJECT();
            y = POP_OBJECT();
            boolean = x == y ? GLOBAL(xtrue) : GLOBAL(xfalse);
            PUSH(boolean);
            INCREF(boolean);
            DECREF(x);
            DECREF(y);
            break; }

            /*** send opcodes -- operators ***/
        case OPCODE_OPER:
            oper = (unsigned int)(*instructionPointer++);
            argumentCount = operSelectors[oper].argumentCount;
            varArg = 0;
            receiver = stackPointer[argumentCount];
            methodClass = GET_CLASS(receiver);
            goto oper;
        case OPCODE_OPER_SUPER:
            oper = (unsigned int)(*instructionPointer++);
            argumentCount = operSelectors[oper].argumentCount;
            varArg = 0;
            methodClass = methodClass->superclass;
 oper:
            for (mc = methodClass; mc; mc = mc->superclass) {
                method = mc->operTable[oper];
                if (method) {
                    methodClass = mc;
                    goto callNewMethod;
                }
            }
            oldIP = instructionPointer - 2;
            ns = METHOD_NAMESPACE_RVALUE;
            selector = *operSelectors[oper].selector;
            INCREF(selector);
            goto createActualMessage;
        case OPCODE_CALL:
            oldIP = instructionPointer - 1;
            oper = (unsigned int)(*instructionPointer++);
            DECODE_UINT(argumentCount);
            varArg = 0;
            receiver = stackPointer[argumentCount];
            methodClass = GET_CLASS(receiver);
            goto call;
        case OPCODE_CALL_VA:
            oldIP = instructionPointer - 1;
            oper = (unsigned int)(*instructionPointer++);
            DECODE_UINT(argumentCount);
            varArg = 1;
            receiver = stackPointer[argumentCount + 1];
            methodClass = GET_CLASS(receiver);
            goto call;
        case OPCODE_CALL_SUPER:
            oldIP = instructionPointer - 1;
            oper = (unsigned int)(*instructionPointer++);
            DECODE_UINT(argumentCount);
            varArg = 0;
            methodClass = methodClass->superclass;
            goto call;
        case OPCODE_CALL_SUPER_VA:
            oldIP = instructionPointer - 1;
            oper = (unsigned int)(*instructionPointer++);
            DECODE_UINT(argumentCount);
            varArg = 1;
            methodClass = methodClass->superclass;
 call:
            for (mc = methodClass; mc; mc = mc->superclass) {
                method = methodClass->operCallTable[oper];
                if (method) {
                    methodClass = mc;
                    goto callNewMethod;
                }
            }
            ns = METHOD_NAMESPACE_RVALUE;
            selector = *operCallSelectors[oper].selector;
            INCREF(selector);
            goto createActualMessage;
            
            /*** send opcodes -- "*p = v" ***/
        case OPCODE_SET_IND:
            receiver = stackPointer[1];
            methodClass = GET_CLASS(receiver);
            goto ind;
        case OPCODE_SET_IND_SUPER:
            methodClass = methodClass->superclass;
 ind:
            argumentCount = 1;
            varArg = 0;
            for (mc = methodClass; mc; mc = mc->superclass) {
                method = mc->assignInd;
                if (method) {
                    methodClass = mc;
                    goto callNewMethod;
                }
            }
            oldIP = instructionPointer - 1;
            ns = METHOD_NAMESPACE_LVALUE;
            selector = *operSelectors[OPER_IND].selector;
            INCREF(selector);
            goto createActualMessage;
            
            /*** send opcodes -- "a[i] = v" ***/
        case OPCODE_SET_INDEX:
            oldIP = instructionPointer - 1;
            DECODE_UINT(argumentCount);
            varArg = 0;
            receiver = stackPointer[argumentCount];
            methodClass = GET_CLASS(receiver);
            goto index;
        case OPCODE_SET_INDEX_VA:
            oldIP = instructionPointer - 1;
            DECODE_UINT(argumentCount);
            varArg = 1;
            receiver = stackPointer[argumentCount + 1];
            methodClass = GET_CLASS(receiver);
            goto index;
        case OPCODE_SET_INDEX_SUPER:
            oldIP = instructionPointer - 1;
            DECODE_UINT(argumentCount);
            varArg = 0;
            methodClass = methodClass->superclass;
            goto index;
        case OPCODE_SET_INDEX_SUPER_VA:
            oldIP = instructionPointer - 1;
            DECODE_UINT(argumentCount);
            varArg = 1;
            methodClass = methodClass->superclass;
 index:
            for (mc = methodClass; mc; mc = mc->superclass) {
                method = methodClass->assignIndex;
                if (method) {
                    methodClass = mc;
                    goto callNewMethod;
                }
            }
            ns = METHOD_NAMESPACE_LVALUE;
            selector = *operCallSelectors[OPER_INDEX].selector;
            INCREF(selector);
            goto createActualMessage;

            /*** send opcodes -- "obj.attr" ***/
        case OPCODE_SET_ATTR:
            ns = METHOD_NAMESPACE_LVALUE;
            argumentCount = 1;
            varArg = 0;
            goto attr;
        case OPCODE_GET_ATTR:
            ns = METHOD_NAMESPACE_RVALUE;
            argumentCount = varArg = 0;
 attr:
            oldIP = instructionPointer - 1;
            receiver = stackPointer[argumentCount];
            methodClass = GET_CLASS(receiver);
            DECODE_UINT(index);
            selector = literalPointer[index];
            INCREF(selector);
            goto lookupMethodInClass;
        case OPCODE_SET_ATTR_SUPER:
            ns = METHOD_NAMESPACE_LVALUE;
            argumentCount = 1;
            varArg = 0;
            goto superAttr;
        case OPCODE_GET_ATTR_SUPER:
            ns = METHOD_NAMESPACE_RVALUE;
            argumentCount = varArg = 0;
 superAttr:
            oldIP = instructionPointer - 1;
            methodClass = methodClass->superclass;
            DECODE_UINT(index);
            selector = literalPointer[index];
            INCREF(selector);
            goto lookupMethodInClass;
            
            /*** send opcodes -- "obj.*attr" ***/
        case OPCODE_SET_ATTR_VAR:
            ns = METHOD_NAMESPACE_LVALUE;
            argumentCount = 1;
            varArg = 0;
            goto attrVar;
        case OPCODE_GET_ATTR_VAR:
            ns = METHOD_NAMESPACE_RVALUE;
            argumentCount = varArg = 0;
 attrVar:
            receiver = stackPointer[argumentCount + 1];
            methodClass = GET_CLASS(receiver);
 perform:
            selector = stackPointer[argumentCount]; /* steal reference */
            if (0 /*!Host_IsSelector(selector)*/ ) {
                --instructionPointer;
                TRAP(mustBeSymbol, 0);
            }
            if (argumentCount == 1) {
                stackPointer[1] = stackPointer[0];
            }
            POP(1);
            oldIP = instructionPointer - 1;
            goto lookupMethodInClass;
        case OPCODE_SET_ATTR_VAR_SUPER:
            ns = METHOD_NAMESPACE_LVALUE;
            argumentCount = 1;
            varArg = 0;
            goto superSetAttr;
        case OPCODE_GET_ATTR_VAR_SUPER:
            ns = METHOD_NAMESPACE_RVALUE;
            argumentCount = varArg = 0;
 superSetAttr:
            methodClass = methodClass->superclass;
            goto perform;
            
            /*** send opcodes -- "obj.attr(a1, a2, ...)" &  keyword expressions ***/
        case OPCODE_SEND_MESSAGE:
            oldIP = instructionPointer - 1;
            DECODE_UINT(index);
            DECODE_UINT(argumentCount);
            varArg = 0;
            receiver = stackPointer[argumentCount];
            methodClass = GET_CLASS(receiver);
            goto send;
        case OPCODE_SEND_MESSAGE_VA:
            oldIP = instructionPointer - 1;
            DECODE_UINT(index);
            DECODE_UINT(argumentCount);
            varArg = 1;
            receiver = stackPointer[argumentCount + 1];
            methodClass = GET_CLASS(receiver);
            goto send;
        case OPCODE_SEND_MESSAGE_SUPER:
            oldIP = instructionPointer - 1;
            DECODE_UINT(index);
            DECODE_UINT(argumentCount);
            varArg = 0;
            methodClass = methodClass->superclass;
            goto send;
        case OPCODE_SEND_MESSAGE_SUPER_VA:
            oldIP = instructionPointer - 1;
            DECODE_UINT(index);
            DECODE_UINT(argumentCount);
            varArg = 1;
            methodClass = methodClass->superclass;
 send:
            ns = METHOD_NAMESPACE_RVALUE;
            selector = literalPointer[index];
            INCREF(selector);
            goto lookupMethodInClass;

            /*** send opcodes -- "(obj.*attr)(a1, a2, ...)" ***/
        case OPCODE_SEND_MESSAGE_VAR:
            oldIP = instructionPointer - 1;
            DECODE_UINT(argumentCount);
            varArg = 0;
            receiver = stackPointer[argumentCount + 1];
            methodClass = GET_CLASS(receiver);
            goto sendVar;
        case OPCODE_SEND_MESSAGE_VAR_VA:
            oldIP = instructionPointer - 1;
            DECODE_UINT(argumentCount);
            varArg = 1;
            receiver = stackPointer[argumentCount + 2];
            methodClass = GET_CLASS(receiver);
            goto sendVar;
        case OPCODE_SEND_MESSAGE_SUPER_VAR:
            oldIP = instructionPointer - 1;
            DECODE_UINT(argumentCount);
            varArg = 0;
            methodClass = methodClass->superclass;
            goto sendVar;
        case OPCODE_SEND_MESSAGE_SUPER_VAR_VA:
            oldIP = instructionPointer - 1;
            DECODE_UINT(argumentCount);
            varArg = 1;
            methodClass = methodClass->superclass;
 sendVar:
            ns = METHOD_NAMESPACE_RVALUE;
            selector = POP_OBJECT(); /* steal reference */
            goto lookupMethodInClass;
            
            /*** send opcodes -- generic ***/
        case OPCODE_SEND_MESSAGE_NS_VAR_VA: {
            Unknown *namespaceObj;
            receiver = stackPointer[3];
            methodClass = GET_CLASS(receiver);
 sendNSVar:
            namespaceObj = stackPointer[2];
            if (!Host_IsInteger(namespaceObj)) {
                assert(XXX); /* trap */
            }
            ns = (MethodNamespace)Host_IntegerAsCLong(namespaceObj);
            assert(0 <= ns && ns < NUM_METHOD_NAMESPACES);
            selector = stackPointer[1]; /* steal reference */
            if (0 /*!Host_IsSelector(selector)*/ ) {
                --instructionPointer;
                TRAP(mustBeSymbol, 0);
            }
            stackPointer[2] = stackPointer[0];
            POP(2);
            DECREF(namespaceObj);
            oldIP = instructionPointer - 1;
            argumentCount = 0;
            varArg = 1;
            goto lookupMethodInClass; }
        case OPCODE_SEND_MESSAGE_SUPER_NS_VAR_VA:
            methodClass = methodClass->superclass;
            goto sendNSVar;

 lookupMethodInClass:
            for (mc = methodClass; mc; mc = mc->superclass) {
                method = Behavior_LookupMethod(mc, ns, selector);
                if (method) {
                    DECREF(method);
                    methodClass = mc;
                    DECREF(selector);
                    selector = 0;
 callNewMethod:
                    /* call */
                    self->activeContext->pc = instructionPointer;
                    instructionPointer = Method_OPCODES(method);
                    instVarPointer = INSTANCE_VARS(receiver, methodClass);
                    if (methodClass->module) {
                        globalPointer = Module_VARIABLES(methodClass->module);
                        literalPointer = Module_LITERALS(methodClass->module);
                    } else {
                        globalPointer = 0;
                        literalPointer = 0;
                    }
                    goto loop;
                }
            }
            
            if (selector == doesNotUnderstand) {
                /* recursive doesNotUnderstand: */
                instructionPointer = oldIP;
                TRAP(recursiveDoesNotUnderstand, STACK_TOP());
            }
            
 createActualMessage:
            do {
                Message *message;
                Unknown *varArgTuple;
                
                if (varArg) {
                    varArgTuple = stackPointer[0];
                    if (!IsArgs(varArgTuple)) {
                        TRAP(mustBeTuple, 0);
                    }
                } else {
                    varArgTuple = 0;
                }
                message = Message_New();
                message->ns = ns;
                message->selector = selector; /* steal reference */
                message->arguments
                    = Host_GetArgs(
                        stackPointer + varArg, argumentCount,
                        varArgTuple, 0
                        );
                
                CLEAN(stackPointer, varArg + argumentCount);
                PUSH(message);  /* XXX: doesNotUnderstand overhead  -- see Context_new() */
                argumentCount = 1;
                varArg = 0;
                ns = METHOD_NAMESPACE_RVALUE;
            } while (0);
            
            selector = doesNotUnderstand;
            INCREF(selector);
            goto lookupMethodInClass;
            
            /*** save/restore/return opcodes ***/
        case OPCODE_RET:
            instructionPointer = self->activeContext->pc;
            receiver = homeContext->u.m.receiver;
            method = homeContext->u.m.method;
            methodClass = homeContext->u.m.methodClass;
            instVarPointer = INSTANCE_VARS(receiver, methodClass);
            if (methodClass->module) {
                globalPointer = Module_VARIABLES(methodClass->module);
                literalPointer = Module_LITERALS(methodClass->module);
            } else {
                globalPointer = 0;
                literalPointer = 0;
            }
            if (self->activeContext->mark != &mark) {
                /* suspend */
                self->activeContext->pc = instructionPointer;
                self->activeContext->stackp = stackPointer;
                return 0; /* unwind */
            }
            break;
        case OPCODE_RET_TRAMP: {
            /* return from trampoline */
            Unknown *result = POP_OBJECT();
            self->activeContext->pc = instructionPointer;
            self->activeContext->stackp = stackPointer;
            return result; }
            
        case OPCODE_LEAF:
            leafContext->sender = self->activeContext;  INCREF(self->activeContext);
            leafContext->homeContext = leafContext;     INCREF(leafContext); /* note cycle */
            
            XDECREF(leafContext->u.m.method);
            INCREF(method);
            leafContext->u.m.method = method;
            
            XDECREF(leafContext->u.m.methodClass);
            INCREF(methodClass);
            leafContext->u.m.methodClass = methodClass;
            
            /* Sometimes this is the only reference to the receiver. */
            XDECREF(leafContext->u.m.receiver);
            INCREF(receiver);
            leafContext->u.m.receiver = receiver;
            
            leafContext->mark = &mark;
            
            self->activeContext->stackp = stackPointer;
            INCREF(leafContext);
            DECREF(self->activeContext);
            self->activeContext = homeContext = leafContext;
            
            stackPointer = framePointer = Context_VARIABLES(leafContext) + LEAF_MAX_STACK_SIZE;
            break;
            
        case OPCODE_SAVE: {
            size_t contextSize, stackSize;
            Context *newContext;
            
            /* Create a new context for the currently
             * executing method (cf. activateNewMethod).
             */
            DECODE_UINT(contextSize);
            DECODE_UINT(stackSize);
            
            /* XXX: doesNotUnderstand overhead */
            ++contextSize;
            ++stackSize;
            
            newContext = Context_New(contextSize);
            
            newContext->sender = self->activeContext;   INCREF(self->activeContext);
            newContext->pc = instructionPointer;
            newContext->homeContext = newContext;       INCREF(newContext); /* note cycle */
            newContext->u.m.method = method;            INCREF(method);
            newContext->u.m.methodClass = methodClass;  INCREF(methodClass);
            newContext->u.m.receiver = receiver;        INCREF(receiver);
            newContext->mark = &mark;
            
            newContext->stackp = newContext->u.m.framep = Context_VARIABLES(newContext) + stackSize;
            
            self->activeContext->stackp = stackPointer;
            DECREF(self->activeContext);
            self->activeContext = homeContext = newContext;
            
            stackPointer = newContext->stackp;
            framePointer = newContext->u.m.framep;
            break; }
            
        case OPCODE_ARG_VA:
            variadic = 1;
            goto args;
        case OPCODE_ARG: {
            size_t i, varArgTupleSize;
            size_t argc, minFixedArgumentCount, maxFixedArgumentCount;
            size_t excessStackArgCount, consumedArrayArgCount;
            Context *newContext;
            Unknown *varArgTuple;
            Unknown **p, **csp;
            Opcode *base;
            
            variadic = 0;
 args:
            base = instructionPointer - 1;
            DECODE_UINT(minFixedArgumentCount);
            DECODE_UINT(maxFixedArgumentCount);
            /* XXX: this is a rogue register; there may be others... */
            fixedArgumentCount = maxFixedArgumentCount;
            
            newContext = self->activeContext;
            
            p = framePointer;
            csp = newContext->caller->stackp; /* caller's stack pointer */
            
            /* process arguments */
            if (varArg) {
                varArgTuple = csp[0];
                if (!IsArgs(varArgTuple)) {
                    TRAP(mustBeTuple, 0);
                }
                varArgTupleSize = ArgsSize(varArgTuple);
            } else {
                varArgTuple = 0;
                varArgTupleSize = 0;
            }
            
            argc = argumentCount + varArgTupleSize;
            if (variadic
                ? argc < minFixedArgumentCount
                : argc < minFixedArgumentCount || maxFixedArgumentCount < argc) {
                TRAP(wrongNumberOfArguments, 0);
            }
            if (argumentCount < maxFixedArgumentCount) {
                /* copy & reverse fixed arguments from stack */
                for (i = 0; i < argumentCount; ++p, ++i) {
                    tmp = *p;
                    *p = csp[varArg + argumentCount - i - 1];
                    INCREF(*p);
                    DECREF(tmp);
                }
                excessStackArgCount = 0;
                /* copy fixed arguments from array */
                consumedArrayArgCount = maxFixedArgumentCount - argumentCount;
                if (varArgTupleSize < consumedArrayArgCount)
                    consumedArrayArgCount = varArgTupleSize; /* the rest default */
                for (i = 0; i < consumedArrayArgCount; ++p, ++i) {
                    tmp = *p;
                    *p = GetArg(varArgTuple, i);
                    DECREF(tmp);
                }
            } else {
                /* copy & reverse fixed arguments from stack */
                for (i = 0; i < maxFixedArgumentCount; ++p, ++i) {
                    tmp = *p;
                    *p = csp[varArg + argumentCount - i - 1];
                    INCREF(*p);
                    DECREF(tmp);
                }
                excessStackArgCount = argumentCount - maxFixedArgumentCount;
                consumedArrayArgCount = 0;
            }
            if (variadic) {
                /* initialize the argument array variable */
                p = framePointer + maxFixedArgumentCount;
                tmp = *p;
                *p = Host_GetArgs(
                    csp + varArg, excessStackArgCount,
                    varArgTuple, consumedArrayArgCount
                    );
                DECREF(tmp);
            }
            
            /* clean up the caller's stack */
            CLEAN(csp, varArg + argumentCount + 1);
            newContext->caller->stackp = csp;
            
            if (minFixedArgumentCount < maxFixedArgumentCount) {
                /* default arguments */
                ptrdiff_t displacement;
                
                for (i = minFixedArgumentCount; i < maxFixedArgumentCount; ++i) {
                    DECODE_SINT(displacement);
                    if (i == argc) {
                        instructionPointer = base + displacement;
                        goto loop;
                    }
                }
                /* skip default arg code */
                DECODE_SINT(displacement);
                instructionPointer = base + displacement;
            }
            
            break; }
            
        case OPCODE_NATIVE: {
            Unknown *result, *arg1 = 0, *arg2 = 0;
            if (variadic) {
                arg1 = framePointer[0];
            } else switch (fixedArgumentCount) {
            case 2: arg2 = framePointer[1];
            case 1: arg1 = framePointer[0];
            case 0: break;
            default: assert(XXX);
            }
            self->activeContext->pc = instructionPointer;
            result = (*method->nativeCode)(receiver, arg1, arg2);
            if (result) {
                PUSH(result);
            } else if (self->activeContext->mark != &mark) {
                return 0; /* unwind */
            } else { /* unwinding is done, and the result is already
                        on the stack */
                goto fetchContextRegisters;
            }
            break; }
            
        case OPCODE_NATIVE_PUSH_INST_VAR: {
            unsigned int type;
            size_t offset;
            char *addr;
            
            DECODE_UINT(type);
            DECODE_UINT(offset);
            addr = (char *)receiver + offset;
            switch (type) {
            case T_OBJECT:
                tmp = *(Unknown **)addr;
                if (!tmp)
                    tmp = GLOBAL(null);
                INCREF(tmp);
                break;
            case T_SIZE:
                tmp = Host_IntegerFromCLong(*(size_t *)addr);
                break;
            default:
                trapUnknownOpcode(self);
                tmp = GLOBAL(uninit);
                INCREF(tmp);
            }
            PUSH(tmp);
            break; }
            
        case OPCODE_NATIVE_STORE_INST_VAR: {
            unsigned int type;
            size_t offset;
            char *addr;
            
            DECODE_UINT(type);
            DECODE_UINT(offset);
            addr = (char *)receiver + offset;
            tmp = STACK_TOP();
            switch (type) {
            case T_OBJECT:
                INCREF(tmp);
                XDECREF(*(Unknown **)addr);
                *(Unknown **)addr = tmp;
                break;
            default:
                trapUnknownOpcode(self);
            }
            break; }
            
        case OPCODE_RESTORE_SENDER: {
            Context *thisCntx, *caller;
            /* restore sender */
            assert(!self->newContext);
            self->newContext = homeContext->sender;
            if (!self->newContext) {
                --instructionPointer;
                TRAP(cannotReturn, 0);
                break;
            }
            INCREF(self->newContext);
            if (!self->newContext->pc) {
                --instructionPointer;
                TRAP(cannotReturn, 0);
                break;
            }
            thisCntx = self->activeContext;
            caller = 0;
            while (thisCntx != self->newContext) {
                XDECREF(caller);
                caller = thisCntx->caller;
                thisCntx->caller = 0;
                thisCntx->pc = 0;
                if (thisCntx->homeContext == thisCntx) {
                    /* break cycles */
                    tmp = (Unknown *)thisCntx->homeContext;
                    thisCntx->homeContext = 0;
                    DECREF(tmp);
                }
                thisCntx = caller;
            }
            XDECREF(caller);
            goto restore; }
        case OPCODE_RESTORE_CALLER: {
            Unknown *result;
            /* restore caller */
            assert(!self->newContext);
            self->newContext = self->activeContext->caller; /* steal reference */
            self->activeContext->caller = 0;
            /* suspend */
            self->activeContext->pc = instructionPointer + 1; /* skip 'ret' */
            self->activeContext->stackp = stackPointer + 1; /* skip result */
            if (self->activeContext->homeContext == self->activeContext) {
                /* break cycles */
                tmp = (Unknown *)self->activeContext->homeContext;
                self->activeContext->homeContext = 0;
                DECREF(tmp);
            }
 restore:
            result = POP_OBJECT();
            
            DECREF(self->activeContext);
            self->activeContext = self->newContext; self->newContext = 0;
            homeContext = self->activeContext->homeContext;
            
            stackPointer = self->activeContext->stackp;
            framePointer = homeContext->u.m.framep;
            
            PUSH(result);
            break; }
            
            /*** block context opcodes ***/
        case OPCODE_CALL_BLOCK: {
            Context *blockContext;
            Unknown **p;
            size_t i;
            
            blockContext = (Context *)(receiver);
            if (blockContext->caller || !blockContext->pc) {
                TRAP(cannotReenterBlock, 0);
            }
            INCREF(blockContext);
            
            /* process arguments */
            if (varArg) {
                Unknown *varArgTuple;
                
                varArgTuple = stackPointer[0];
                if (!IsArgs(varArgTuple)) {
                    TRAP(mustBeTuple, 0);
                }
                if (argumentCount + ArgsSize(varArgTuple) != blockContext->u.b.nargs) {
                    TRAP(wrongNumberOfArguments, 0);
                }
                
                /* copy & reverse arguments from stack */
                p = &blockContext->homeContext->u.m.framep[blockContext->u.b.index];
                for (i = 0; i < argumentCount; ++p, ++i) {
                    tmp = *p;
                    *p = stackPointer[1 + argumentCount - i - 1];
                    INCREF(*p);
                    DECREF(tmp);
                }
                
                /* copy arguments from array */
                for (i = 0; i < ArgsSize(varArgTuple); ++p, ++i) {
                    tmp = *p;
                    *p = GetArg(varArgTuple, i);
                    DECREF(tmp);
                }
                
            } else {
                if (argumentCount != blockContext->u.b.nargs) {
                    TRAP(wrongNumberOfArguments, 0);
                }
                
                /* copy & reverse arguments from stack */
                p = &blockContext->homeContext->u.m.framep[blockContext->u.b.index];
                for (i = 0; i < argumentCount; ++p, ++i) {
                    tmp = *p;
                    *p = stackPointer[argumentCount - i - 1];
                    INCREF(*p);
                    DECREF(tmp);
                }
                
            }
            
            /* clean up the caller's stack */
            CLEAN(stackPointer, varArg + argumentCount + 1);
            self->activeContext->stackp = stackPointer;
            
            blockContext->caller = self->activeContext;  INCREF(self->activeContext);
            blockContext->mark = &mark;
            DECREF(self->activeContext);
            self->activeContext = blockContext;
            goto fetchContextRegisters; }
            
            /*** debugging ***/
        case OPCODE_CHECK_STACKP: {
            size_t offset;
            DECODE_UINT(offset);
            assert(method != self->activeContext->u.m.method || /* in leaf routine */
                   /* XXX: What about BlockContexts? */
                   stackPointer == homeContext->u.m.framep - offset);
            break; }
        }
    }

}


/*------------------------------------------------------------------------*/
/* native code support */

Unknown *Interpreter_SendMessage(
    Interpreter *interpreter,
    Unknown *obj,
    unsigned int ns,
    Unknown *selector,
    Unknown *argumentArray
    )
{
    static Method *start;
    Context *thisContext;
    Method *method;
    Opcode *oldPC; Unknown **oldSP;
    Unknown *result;
    
    thisContext = interpreter->activeContext;
    if (!thisContext) {
        /* call from Python or other foreign code */
        Object *receiver;
        
        receiver = CAST(Object, obj);
        assert(receiver);
        
        if (!start) {
            start = NewNativeMethod(NativeCode_ARGS_ARRAY, 0);
        }
        
        thisContext = Context_New(10); /* XXX */
        thisContext->caller = 0;
        thisContext->pc = Method_OPCODES(start) + 7;
        thisContext->stackp = &Context_VARIABLES(thisContext)[10]; /* XXX */
        thisContext->homeContext = thisContext;            INCREF(thisContext);
        thisContext->u.m.method = start;                   INCREF(start);
        thisContext->u.m.methodClass = receiver->klass;    INCREF(receiver->klass);
        thisContext->u.m.receiver = (Unknown *)obj;     INCREF(obj);
        thisContext->u.m.framep = thisContext->stackp;
        
        interpreter->activeContext = thisContext; /* steal reference */
    }
    
    assert(thisContext == thisContext->homeContext);
    method = thisContext->u.m.method;
    
    oldPC = thisContext->pc;
    oldSP = thisContext->stackp;
    
    assert(*thisContext->pc == OPCODE_BRANCH_ALWAYS && "call from non-leaf native method");
    
    /* move the program counter to the trampoline code */
    if (obj) {
        thisContext->pc += 2;
    } else {
        thisContext->pc += 4;
        obj = thisContext->u.m.receiver;
    }
    
    /* push arguments on the stack */
    INCREF(obj);
    INCREF(selector);
    INCREF(argumentArray);
    DECREF(thisContext->stackp[-1]);
    DECREF(thisContext->stackp[-2]);
    DECREF(thisContext->stackp[-3]);
    DECREF(thisContext->stackp[-4]);
    *--thisContext->stackp = obj;
    *--thisContext->stackp = Host_IntegerFromCLong(ns);
    *--thisContext->stackp = selector;
    *--thisContext->stackp = argumentArray;
    assert(thisContext->stackp >= &Context_VARIABLES(thisContext)[0]);
    
    /* interpret */
    result = Interpreter_Interpret(interpreter);
    if (!result)
        return 0; /* unwind */
    
    thisContext->pc = oldPC;
    assert(thisContext->stackp == oldSP);
    
    if (thisContext->u.m.method == start) {
        assert(interpreter->activeContext == thisContext);
        interpreter->activeContext = 0;
        DECREF(thisContext);
    }
    
    return result;
}


/*------------------------------------------------------------------------*/
/* fibers */

Semaphore *Semaphore_New() {
    return XXX /*(Semaphore *)Object_New(CLASS(Semaphore)) */ ;
}

int Semaphore_IsEmpty(Semaphore *self) {
    return !self->firstLink;
}

void Semaphore_AddLast(Semaphore *self, Fiber *link) {
    INCREF(link);
    INCREF(link);
    XDECREF(self->lastLink);
    if (Semaphore_IsEmpty(self)) {
        self->firstLink = link;
    } else {
        XDECREF(self->lastLink->nextLink);
        self->lastLink->nextLink = link;
    }
    self->lastLink = link;
}

Fiber *Semaphore_RemoveFirst(Semaphore *self) {
    Fiber *first = self->firstLink;
    if (!first) {
        /* XXX: exception */
        return 0;
    }
    if (first == self->lastLink) {
        DECREF(first);
        self->firstLink = self->lastLink = 0;
    } else {
        XINCREF(first->nextLink);
        self->firstLink = first->nextLink;
    }
    XDECREF(first->nextLink);
    first->nextLink = 0;
    return first;
}

void Interpreter_SynchronousSignal(Interpreter *self, Semaphore *aSemaphore) {
    if (Semaphore_IsEmpty(aSemaphore)) {
        ++aSemaphore->excessSignals;
    } else {
        Interpreter_Resume(self, Semaphore_RemoveFirst(aSemaphore));
    }
}

void Interpreter_TransferTo(Interpreter *self, Fiber *newFiber) {
    Fiber *oldFiber = self->scheduler->activeFiber;
    oldFiber->suspendedContext = self->activeContext;
    self->scheduler->activeFiber = newFiber;
    assert(!self->newContext);
    self->newContext = newFiber->suspendedContext;
    INCREF(self->newContext);
}

Fiber *Interpreter_WakeHighestPriority(Interpreter *self) {
    int p = HIGHEST_PRIORITY - 1;
    Semaphore **fiberLists = self->scheduler->quiescentFiberLists;
    Semaphore *fiberList = fiberLists[p];
    while (Semaphore_IsEmpty(fiberList)) {
        --p;
        if (p < 0) {
            return trap(self, noRunnableFiber, 0);
        }
        fiberList = fiberLists[p];
    }
    return Semaphore_RemoveFirst(fiberList);
}

/* ----- private ----- */

void Interpreter_PutToSleep(Interpreter *self, Fiber *aFiber) {
    Semaphore **fiberLists = self->scheduler->quiescentFiberLists;
    Semaphore *fiberList = fiberLists[aFiber->priority - 1];
    Semaphore_AddLast(fiberList, aFiber);
    aFiber->myList = fiberList;
}

void Interpreter_Resume(Interpreter *self, Fiber *aFiber) {
    Fiber *activeFiber = self->scheduler->activeFiber;
    if (aFiber->priority > activeFiber->priority) {
        Interpreter_PutToSleep(self, activeFiber);
        Interpreter_TransferTo(self, aFiber);
    } else {
        Interpreter_PutToSleep(self, aFiber);
    }
}


/*------------------------------------------------------------------------*/
/* debug support */

static unsigned int lineNumber(size_t offset, Method *method) {
    Opcode *instructionPointer, *end;
    size_t codeDelta; long lineDelta;
    size_t label;
    unsigned int lineNo;
    
    instructionPointer = method->debug.lineCodes;
    end = instructionPointer + method->debug.lineCodeTally;
    
    label = 0;
    lineNo = 0;
    while (instructionPointer < end) {
        DECODE_UINT(codeDelta);
        DECODE_SINT(lineDelta);
        label += codeDelta;
        if (offset < label)
            break;
        lineNo += lineDelta;
    }
    return lineNo;
}

void Interpreter_PrintCallStack(Interpreter *self) {
    Context *ctxt, *home;
    Method *method;
    Behavior *methodClass;
    Unknown *methodSel, *source;
    size_t offset;
    unsigned int lineNo;
    
    for (ctxt = self->activeContext; ctxt; ctxt = ctxt->sender) {
        home = ctxt->homeContext;
        method = home->u.m.method;
        methodClass = home->u.m.methodClass;
        methodSel = Behavior_FindSelectorOfMethod(methodClass, method);
        source = method->debug.source;
        offset = ctxt->pc - Method_OPCODES(method);
        lineNo = lineNumber(offset, method);
        printf("%04lx %p%s%s::%s at %s:%u\n",
               (unsigned long)offset,
               ctxt,
               (ctxt == home ? " " : " [] in "),
               Behavior_NameAsCString(methodClass),
               methodSel ? Host_SelectorAsCString(methodSel) : "<unknown>",
               source ? Host_StringAsCString(source) : "<unknown>",
               lineNo);
        XDECREF(methodSel);
    }
}


/*------------------------------------------------------------------------*/
/* error handling */

static Fiber *trap(Interpreter *self, Unknown *selector, Unknown *argument) {
    /* XXX */
    halt(self, selector, argument);
    return 0;
}

static void trapUnknownOpcode(Interpreter *self) {
    trap(self, unknownOpcode, 0);
}

void halt(Interpreter *self, Unknown *selector, Unknown *argument) {
    if (argument) {
        Unknown *symbol = 0; Message *message;
        if (Host_IsSymbol(argument)) {
            symbol = argument;
        }
        message = CAST(Message, argument);
        if (symbol) {
            printf("\n%s '%s'", Host_SymbolAsCString(selector), Host_SymbolAsCString(symbol));
        } else if (message) {
            printf("\n%s '%s'", Host_SymbolAsCString(selector), Host_SymbolAsCString(message->selector));
        } else {
            printf("\n%s", Host_SymbolAsCString(selector));
        }
    } else {
        printf("\n%s", Host_SymbolAsCString(selector));
    }
    printf("\n\n");
    if (!self->printingStack) {
        self->printingStack = 1;
        Interpreter_PrintCallStack(self);
    }
    abort();
}
