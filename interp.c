
#include "interp.h"

#include "behavior.h"
#include "bool.h"
#include "class.h"
#include "dict.h"
#include "int.h"
#include "module.h"
#include "sym.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*** TODO ***/
#define XXX 0
#define sender caller
#define HIGHEST_PRIORITY XXX


Null *Spk_null;
Uninit *Spk_uninit;
Void *Spk_void;

Behavior *ClassMessage, *ClassThunk, *ClassNull, *ClassUninit, *ClassVoid;


Object *SpkObject_new(size_t size) {
    return (Object *)malloc(sizeof(Object) + (size - 1) * sizeof(Object *));
}


static void oopcpy(Object **dest, Object **src, size_t count) {
    for ( ; count > 0; --count)
        *dest++ = *src++;
}



/*------------------------------------------------------------------------*/
/* class templates */

static SpkMethodTmpl MessageMethods[] = {
    { 0, 0, 0}
};

SpkClassTmpl ClassMessageTmpl = {
    "Message",
    offsetof(MessageSubclass, variables),
    sizeof(Message),
    0,
    MessageMethods
};


static SpkMethodTmpl ThunkMethods[] = {
    { 0, 0, 0}
};

SpkClassTmpl ClassThunkTmpl = {
    "Thunk",
    offsetof(ThunkSubclass, variables),
    sizeof(Thunk),
    0,
    ThunkMethods
};


static SpkMethodTmpl NullMethods[] = {
    { 0, 0, 0}
};

SpkClassTmpl ClassNullTmpl = {
    "Null",
    offsetof(ObjectSubclass, variables),
    sizeof(Null),
    0,
    NullMethods
};


static SpkMethodTmpl UninitMethods[] = {
    { 0, 0, 0}
};

SpkClassTmpl ClassUninitTmpl = {
    "Uninit",
    offsetof(ObjectSubclass, variables),
    sizeof(Uninit),
    0,
    UninitMethods
};


static SpkMethodTmpl VoidMethods[] = {
    { 0, 0, 0}
};

SpkClassTmpl ClassVoidTmpl = {
    "Void",
    offsetof(ObjectSubclass, variables),
    sizeof(Void),
    0,
    VoidMethods
};


/*------------------------------------------------------------------------*/
/* initialization */

Object *SpkInterpreter_start(Object *entry) {
    Method *callThunk, *trampoline;
    Context *context;
    Fiber fiber;
    ProcessorScheduler scheduler;
    Interpreter interpreter;
    opcode_t *ip; size_t trampolineSize;
    
    callThunk = (Method *)malloc(sizeof(Method));
    callThunk->nativeCode = 0;
    callThunk->argumentCount = 0;
    callThunk->localCount = 0;
    callThunk->stackSize = 0;
    callThunk->size = 1;
    callThunk->opcodes[0] = OPCODE_CALL_THUNK;
    ClassThunk->operCallTable[OPER_APPLY].method = callThunk;
    ClassThunk->operCallTable[OPER_APPLY].methodClass = ClassThunk;
    
    trampolineSize = 5;
    trampoline = (Method *)malloc(sizeof(Method) + trampolineSize*sizeof(opcode_t));
    trampoline->nativeCode = 0;
    trampoline->argumentCount = 1;
    trampoline->localCount = 0;
    trampoline->stackSize = 1;
    trampoline->size = trampolineSize;
    ip = &trampoline->opcodes[0];
    *ip++ = OPCODE_PUSH_SELF;
    *ip++ = OPCODE_CALL;
    *ip++ = (opcode_t)OPER_APPLY;
    *ip++ = (opcode_t)0;
    *ip++ = OPCODE_RETT;
    
    context = SpkContext_new(contextSizeForMethod(trampoline));

    context->caller = 0;
    context->pc = &trampoline->opcodes[0];
    context->stackp = &context->variables[trampoline->stackSize];
    context->homeContext = context;
    context->u.m.method = trampoline;
    context->u.m.methodClass = entry->klass;
    context->u.m.receiver = entry;
    
    fiber.nextLink = 0;
    fiber.suspendedContext = context;
    fiber.priority = 0;
    fiber.myList = 0;
    
    scheduler.quiescentFiberLists = 0;
    scheduler.activeFiber = &fiber;
    
    SpkInterpreter_init(&interpreter, &scheduler);
    return SpkInterpreter_interpret(&interpreter);
}

void SpkInterpreter_init(Interpreter *self, ProcessorScheduler *aScheduler) {
    /* fibers */
    self->scheduler = aScheduler;
    self->theInterruptSemaphore = SpkSemaphore_new();
    self->interruptPending = 0;

    /* contexts */
    self->activeContext = self->scheduler->activeFiber->suspendedContext;
    self->newContext = 0;

    /* special objects */
    self->selectorCannotReturn           = SpkSymbol_get("cannotReturn");
    self->selectorDoesNotUnderstand      = SpkSymbol_get("doesNotUnderstand");
    self->selectorMustBeBoolean          = SpkSymbol_get("mustBeBoolean");
    self->selectorNoRunnableFiber        = SpkSymbol_get("noRunnableFiber");
    self->selectorUnknownOpcode          = SpkSymbol_get("unknownOpcode");
    self->selectorWrongNumberOfArguments = SpkSymbol_get("wrongNumberOfArguments");

    /* error handling */
    self->printingStack = 0;
}

Message *SpkMessage_new() {
    Message *newMessage;
    
    newMessage = (Message *)malloc(sizeof(Message));
    newMessage->base.klass = ClassMessage;
    return newMessage;
}

Method *SpkMethod_new(size_t argumentCount,
                      size_t localCount,
                      size_t stackSize,
                      size_t size) {
    Method *newMethod;
    
    newMethod = (Method *)malloc(sizeof(Method) + size*sizeof(opcode_t));
    newMethod->nativeCode = 0;
    newMethod->argumentCount = argumentCount;
    newMethod->localCount = localCount;
    newMethod->stackSize = stackSize;
    newMethod->size = size;
    return newMethod;
}

Method *SpkMethod_newNative(SpkNativeCodeFlags flags, SpkNativeCode nativeCode) {
    Method *newMethod;
    size_t size, argumentCount;
    opcode_t *ip;
    
    size = 0;
    if (flags & SpkNativeCode_CALLABLE) {
        size += 2; /* thunk, ret */
    }
    if (flags & SpkNativeCode_LEAF) {
        size += 1; /* trap */
    } else {
        size += 3; /* save, trap, restore */
    }
    size += 1; /* ret */
    
    switch (flags & SpkNativeCode_SIGNATURE_MASK) {
    case SpkNativeCode_ARGS_0: argumentCount = 0; break;
    case SpkNativeCode_ARGS_1: argumentCount = 1; break;
    case SpkNativeCode_ARGS_2: argumentCount = 2; break;
    default: assert(XXX);
    }
    
    newMethod = (Method *)malloc(sizeof(Method) + size*sizeof(opcode_t));
    newMethod->nativeCode = nativeCode;
    newMethod->argumentCount = argumentCount;
    newMethod->localCount = 0;
    newMethod->stackSize = argumentCount + 1;
    newMethod->size = size;
    
    ip = &newMethod->opcodes[0];
    if (flags & SpkNativeCode_CALLABLE) {
        *ip++ = OPCODE_THUNK;
    }
    if (flags & SpkNativeCode_LEAF) {
        *ip++ = OPCODE_TRAP_NATIVE;
    } else {
        *ip++ = OPCODE_SAVE;
        *ip++ = OPCODE_TRAP_NATIVE;
        *ip++ = OPCODE_RESTORE_SENDER;
    }
    *ip++ = OPCODE_RET;
    
    return newMethod;
}


/*------------------------------------------------------------------------*/
/* contexts */

Context *SpkContext_new(size_t size) {
    return (Context *)malloc(sizeof(Context) + (size - 1) * sizeof(Object *));
}

Context *SpkContext_blockCopy(Context *self, size_t numArgs, opcode_t *instructionPointer) {
    Context *home = self->homeContext;
    size_t contextSize = contextSizeForMethod(home->u.m.method);
    Context *newContext = SpkContext_new(contextSize);
    newContext->sender = 0;
    newContext->pc = instructionPointer;
    newContext->stackp = 0;
    newContext->homeContext = home;
    newContext->u.b.nargs = numArgs;
    newContext->u.b.startpc = instructionPointer;
    return newContext;
}

void SpkContext_init(Context *self, Context *activeContext) {
    size_t contextSize;
    
    if (self->u.b.nargs != 0) {
        assert(XXX);
    }
    contextSize = contextSizeForMethod(self->homeContext->u.m.method);
    self->pc = self->u.b.startpc;
    self->stackp = &self->variables[contextSize];
    self->sender = activeContext;
}

void SpkContext_initWithArg(Context *self, Object *argument, Context *activeContext) {
    size_t contextSize;
    
    if (self->u.b.nargs != 1) {
        assert(XXX);
    }
    contextSize = contextSizeForMethod(self->homeContext->u.m.method);
    self->variables[0] = argument;
    self->pc = self->u.b.startpc;
    self->stackp = &self->variables[contextSize - 1];
    self->sender = activeContext;
}


/*------------------------------------------------------------------------*/
/* interpreter loop */

/* stackPointer */
#define POP_OBJECT() (*stackPointer++)
#define POP(nItems) (stackPointer += nItems)
#define UNPOP(nItems) (stackPointer -= nItems)
#define PUSH(object) (*--stackPointer = (Object *)(object))
#define STACK_TOP() (*stackPointer)
#define STACK_VALUE(offset) (stackPointer[offset])

#define INSTANCE_VARS(op) ((Object **)(((char *)op) + (op)->klass->instVarOffset))


#define DECODE_UINT(result) do { \
    opcode_t byte; \
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
    opcode_t byte; \
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
    SpkInterpreter_trap(self, selector, argument)


Object *SpkInterpreter_interpret(Interpreter *self) {

    /* context registers */
    Context *homeContext;
    Object *receiver;
    Method *method;
    Behavior *methodClass;
    register opcode_t *instructionPointer;
    opcode_t *linkRegister;
    register Object **stackPointer;
    register Object **framePointer;
    register Object **instVarPointer;
    register Object **globalPointer;

    size_t index;

    /* message sending */
    Symbol *messageSelector = 0;
    size_t argumentCount = 0;
    unsigned int operator;
    opcode_t *oldIP;
    
 fetchContextRegisters:
    homeContext = self->activeContext->homeContext;
    receiver = homeContext->u.m.receiver;
    method = homeContext->u.m.method;
    methodClass = homeContext->u.m.methodClass;
    instructionPointer = self->activeContext->pc;
    linkRegister = 0;
    stackPointer = self->activeContext->stackp;
    framePointer = &homeContext->variables[method->stackSize]; /* XXX: inconsistent */
    instVarPointer = INSTANCE_VARS(receiver);
    globalPointer = INSTANCE_VARS((Object *)methodClass->module);
    self->newContext = 0;

 checkForInterrupts:
    if (self->interruptPending) {
        self->interruptPending = 0;
        SpkInterpreter_synchronousSignal(self, self->theInterruptSemaphore);
        if (self->newContext) {
            self->activeContext = self->newContext;
            goto fetchContextRegisters;
        }
    }

 loop:
    while (1) {
        
        switch (*instructionPointer++) {
            
        default:
            --instructionPointer;
            SpkInterpreter_unknownOpcode(self);
            break;
            
        case OPCODE_NOP:
            break;

/*** push opcodes ***/
        case OPCODE_PUSH_LOCAL:
            DECODE_UINT(index);
            PUSH(framePointer[index]);
            break;
        case OPCODE_PUSH_INST_VAR:
            DECODE_UINT(index);
            PUSH(instVarPointer[index]);
            break;
        case OPCODE_PUSH_GLOBAL:
            DECODE_UINT(index);
            PUSH(globalPointer[index]);
            break;
        case OPCODE_PUSH_SELF:     PUSH(receiver);             break;
        case OPCODE_PUSH_FALSE:    PUSH(Spk_false);            break;
        case OPCODE_PUSH_TRUE:     PUSH(Spk_true);             break;
        case OPCODE_PUSH_NULL:     PUSH(Spk_null);             break;
        case OPCODE_PUSH_VOID:     PUSH(Spk_void);             break;
        case OPCODE_PUSH_CONTEXT:  PUSH(self->activeContext);  break;
        case OPCODE_DUP: {
            Object *temp;
            temp = STACK_TOP();
            PUSH(temp);
            break; }
        case OPCODE_DUP_N: {
            Object **s;
            size_t n;
            DECODE_UINT(n);
            s = stackPointer + n;
            while (n--)
                *--stackPointer = *--s;
            break; }
        case OPCODE_PUSH_INT: {
            long value;
            DECODE_SINT(value);
            PUSH(SpkInteger_fromLong(value));
            break; }
                
/*** store opcodes ***/
        case OPCODE_STORE_LOCAL:
            DECODE_UINT(index);
            framePointer[index] = STACK_TOP();
            break;
        case OPCODE_STORE_INST_VAR:
            DECODE_UINT(index);
            instVarPointer[index] = STACK_TOP();
            break;
        case OPCODE_STORE_GLOBAL:
            DECODE_UINT(index);
            globalPointer[index] = STACK_TOP();
            break;
            
/*** additional stack opcodes ***/
        case OPCODE_POP:
            POP(1);
            break;
        case OPCODE_ROT: {
            Object **s, **end, *temp;
            size_t n;
            DECODE_UINT(n);
            if (n > 1) {
                temp = STACK_TOP();
                s = stackPointer;
                end = stackPointer + n - 1;
                while (s < end) {
                    s[0] = s[1];
                    ++s;
                }
                *s = temp;
            }
            break; }

/*** branch opcodes ***/
        case OPCODE_BRANCH_IF_FALSE: {
            ptrdiff_t displacement;
            Object *x, *o, *boolean;
            x = (Object *)Spk_false;
            o = (Object *)Spk_true;
            goto branch;
        case OPCODE_BRANCH_IF_TRUE:
            x = (Object *)Spk_true;
            o = (Object *)Spk_false;
 branch:
            boolean = POP_OBJECT();
            if (boolean == x) {
                opcode_t *base;
            case OPCODE_BRANCH_ALWAYS:
                base = instructionPointer - 1;
                DECODE_SINT(displacement);
                instructionPointer = base + displacement;
                if (displacement < 0) {
                    goto checkForInterrupts;
                }
            } else if (boolean != o) {
                --instructionPointer;
                UNPOP(1);
                TRAP(self->selectorMustBeBoolean, 0);
            } else {
                DECODE_SINT(displacement);
            }
            break; }
            
/* identity comparison opcode */
        case OPCODE_ID: {
            Object *x, *y;
            x = POP_OBJECT();
            y = POP_OBJECT();
            PUSH(x == y ? Spk_true : Spk_false);
            break; }

/*** send opcodes -- operators ***/
        case OPCODE_OPER:
            operator = (unsigned int)(*instructionPointer++);
            argumentCount = operSelectors[operator].argumentCount;
            receiver = stackPointer[argumentCount];
            methodClass = receiver->klass;
            goto oper;
        case OPCODE_OPER_SUPER:
            operator = (unsigned int)(*instructionPointer++);
            argumentCount = operSelectors[operator].argumentCount;
            methodClass = methodClass->superclass;
 oper:
            method = methodClass->operTable[operator].method;
            if (method) {
                methodClass = methodClass->operTable[operator].methodClass;
                goto callNewMethod;
            }
            instructionPointer -= 2;
            TRAP(self->selectorDoesNotUnderstand, (Object *)operSelectors[operator].messageSelector);
            break;
        case OPCODE_CALL:
            oldIP = instructionPointer - 1;
            operator = (unsigned int)(*instructionPointer++);
            DECODE_UINT(argumentCount);
            receiver = stackPointer[argumentCount];
            methodClass = receiver->klass;
            goto call;
        case OPCODE_CALL_SUPER:
            oldIP = instructionPointer - 1;
            operator = (unsigned int)(*instructionPointer++);
            DECODE_UINT(argumentCount);
            methodClass = methodClass->superclass;
 call:
            method = methodClass->operCallTable[operator].method;
            if (method) {
                methodClass = methodClass->operCallTable[operator].methodClass;
                goto callNewMethod;
            }
            instructionPointer = oldIP;
            TRAP(self->selectorDoesNotUnderstand, (Object *)operCallSelectors[operator].messageSelector);
            break;

/*** send opcodes -- "obj.attr" ***/
        case OPCODE_ATTR:
            oldIP = instructionPointer - 1;
            receiver = STACK_TOP();
            methodClass = receiver->klass;
            DECODE_UINT(index);
            messageSelector = (Symbol *)(globalPointer[index]);
            goto lookupMethodInClass;
        case OPCODE_ATTR_SUPER:
            oldIP = instructionPointer - 1;
            methodClass = methodClass->superclass;
            DECODE_UINT(index);
            messageSelector = (Symbol *)(globalPointer[index]);
 lookupMethodInClass:
            argumentCount = 0;
            for ( ; methodClass; methodClass = methodClass->superclass) {
                method = SpkBehavior_lookupMethod(methodClass, messageSelector);
                if (method) {
 callNewMethod:
                    /* call (jmpl -- jump and link) */
                    linkRegister = instructionPointer;
                    instructionPointer = &method->opcodes[0];
 jump:
                    framePointer = stackPointer;
                    instVarPointer = INSTANCE_VARS(receiver);
                    globalPointer = INSTANCE_VARS((Object *)methodClass->module);
                    goto loop;
                }
            }
            do { /* createActualMessage */
                Array *argumentArray = (Array *)SpkObject_new(argumentCount);
                Message *message = SpkMessage_new();
                message->messageSelector = messageSelector;
                message->argumentArray = argumentArray;
                oopcpy(&argumentArray->variables[0], stackPointer, argumentCount);
                instructionPointer = oldIP;
                TRAP(self->selectorDoesNotUnderstand, (Object *)message);
            } while (0);
            break;

/*** save/restore/return opcodes ***/
        case OPCODE_RET: {
            /* ret/retl (blr) */
            Object *ret = POP_OBJECT();
            POP(method->argumentCount + 1);
            PUSH(ret);
 ret:
            instructionPointer = linkRegister;
            receiver = homeContext->u.m.receiver;
            method = homeContext->u.m.method;
            methodClass = homeContext->u.m.methodClass;
            framePointer = &homeContext->variables[method->stackSize];
            instVarPointer = INSTANCE_VARS(receiver);
            globalPointer = INSTANCE_VARS((Object *)methodClass->module);
            break; }
        case OPCODE_RETT:
            /* return from trampoline */
            return POP_OBJECT();
            
        case OPCODE_SAVE: {
            /* save */
            size_t contextSize, count;
            Context *newContext;
            Object **p;

            /* Create a new context for the currently
             * executing method (cf. activateNewMethod).
             */
            contextSize = contextSizeForMethod(method);
            newContext = SpkContext_new(contextSize);

            newContext->sender = self->activeContext;
            newContext->pc = instructionPointer;
            newContext->homeContext = newContext;
            newContext->u.m.method = method;
            newContext->u.m.methodClass = methodClass;
            newContext->u.m.receiver = receiver;
            
            /* initialize the stack */
            count = method->stackSize;
            for (p = &newContext->variables[0]; count > 0; ++p, --count) {
                *p = Spk_uninit;
            }
            newContext->stackp = p;
            count = method->argumentCount;
            /* XXX: What about leaf methods? */
            if (count != argumentCount) {
                TRAP(self->selectorWrongNumberOfArguments, 0);
            }
            /* copy & reverse arguments */
            for ( ; count > 0; ++p, --count) {
                *p = stackPointer[count - 1];
            }
            /* initialize locals */
            count = method->localCount;
            for ( ; count > 0; ++p, --count) {
                *p = Spk_uninit;
            }
            
            self->activeContext->pc = linkRegister;
            self->activeContext->stackp = stackPointer;
            
            framePointer = stackPointer = newContext->stackp;
            self->activeContext = homeContext = newContext;
            break; }
            
        case OPCODE_RESTORE_SENDER: {
            Context *thisCntx, *caller;
            /* restore sender */
            self->newContext = homeContext->sender;
            if (!self->newContext || !self->newContext->pc) {
                --instructionPointer;
                TRAP(self->selectorCannotReturn, 0);
                break;
            }
            thisCntx = self->activeContext;
            for ( ; thisCntx != self->newContext; thisCntx = caller) {
                caller = thisCntx->caller;
                thisCntx->caller = 0;
                thisCntx->pc = 0;
            }
            goto restore; }
        case OPCODE_RESTORE_CALLER: {
            Object *ret;
            /* restore caller */
            self->activeContext->sender = 0;
            self->activeContext->pc = 0;
            self->newContext = self->activeContext->caller;
 restore:
            ret = POP_OBJECT();

            self->activeContext = self->newContext; self->newContext = 0;
            homeContext = self->activeContext->homeContext;

            linkRegister = self->activeContext->pc;
            stackPointer = self->activeContext->stackp;
            framePointer = 0;
            
            PUSH(ret);
            break; }
            
/*** thunk opcodes ***/
        case OPCODE_THUNK: {
            /* new thunk */
            Thunk *thunk = (Thunk *)malloc(sizeof(Thunk));
            thunk->base.klass = ClassThunk;
            thunk->receiver = receiver;
            thunk->method = method;
            thunk->methodClass = methodClass;
            thunk->pc = instructionPointer;
            POP(1); /* receiver */
            PUSH(thunk);
            goto ret; }
        case OPCODE_CALL_THUNK: {
            Thunk *thunk = (Thunk *)(receiver);
            receiver = thunk->receiver;
            method = thunk->method;
            methodClass = thunk->methodClass;
            instructionPointer = thunk->pc;
            goto jump; }
            
/*** traps ***/
        case OPCODE_TRAP_NATIVE: {
            Object *result, *arg1 = 0, *arg2 = 0;
            switch (method->argumentCount) {
            case 2:
                arg1 = STACK_VALUE(1);
                arg2 = STACK_VALUE(0);
                break;
            case 1:
                arg1 = STACK_VALUE(0);
                break;
            case 0:
                break;
            default:
                assert(XXX);
            }
            result = (*method->nativeCode)(receiver, arg1, arg2);
            PUSH(result);
            break; }
            
/*** debugging ***/
        case OPCODE_CHECK_STACKP: {
            size_t offset;
            DECODE_UINT(offset);
            assert(method != self->activeContext->u.m.method /* in leaf routine */ ||
                   stackPointer == &self->activeContext->variables[method->stackSize] - offset);
            break; }
        }
    }

}

Object **SpkInterpreter_instanceVars(Object *object) {
    return INSTANCE_VARS(object);
}


/*------------------------------------------------------------------------*/
/* fibers */

Semaphore *SpkSemaphore_new() {
    return (Semaphore *)malloc(sizeof(Semaphore));
}

int SpkSemaphore_isEmpty(Semaphore *self) {
    return !self->firstLink;
}

void SpkSemaphore_addLast(Semaphore *self, Fiber *link) {
    if (SpkSemaphore_isEmpty(self)) {
        self->firstLink = link;
    } else {
        self->lastLink->nextLink = link;
    }
    self->lastLink = link;
}

Fiber *SpkSemaphore_removeFirst(Semaphore *self) {
    Fiber *first = self->firstLink;
    if (first == self->lastLink) {
        self->firstLink = self->lastLink = 0;
    } else {
        self->firstLink = first->nextLink;
    }
    first->nextLink = 0;
    return first;
}

void SpkInterpreter_synchronousSignal(Interpreter *self, Semaphore *aSemaphore) {
    if (SpkSemaphore_isEmpty(aSemaphore)) {
        ++aSemaphore->excessSignals;
    } else {
        SpkInterpreter_resume(self, SpkSemaphore_removeFirst(aSemaphore));
    }
}

void SpkInterpreter_transferTo(Interpreter *self, Fiber *newFiber) {
    Fiber *oldFiber = self->scheduler->activeFiber;
    oldFiber->suspendedContext = self->activeContext;
    self->scheduler->activeFiber = newFiber;
    self->newContext = newFiber->suspendedContext;
}

Fiber *SpkInterpreter_wakeHighestPriority(Interpreter *self) {
    int p = HIGHEST_PRIORITY - 1;
    Semaphore **fiberLists = self->scheduler->quiescentFiberLists;
    Semaphore *fiberList = fiberLists[p];
    while (SpkSemaphore_isEmpty(fiberList)) {
        --p;
        if (p < 0) {
            return SpkInterpreter_trap(self, self->selectorNoRunnableFiber, 0);
        }
        fiberList = fiberLists[p];
    }
    return SpkSemaphore_removeFirst(fiberList);
}

/* ----- private ----- */

void SpkInterpreter_putToSleep(Interpreter *self, Fiber *aFiber) {
    Semaphore **fiberLists = self->scheduler->quiescentFiberLists;
    Semaphore *fiberList = fiberLists[aFiber->priority - 1];
    SpkSemaphore_addLast(fiberList, aFiber);
    aFiber->myList = fiberList;
}

void SpkInterpreter_resume(Interpreter *self, Fiber *aFiber) {
    Fiber *activeFiber = self->scheduler->activeFiber;
    if (aFiber->priority > activeFiber->priority) {
        SpkInterpreter_putToSleep(self, activeFiber);
        SpkInterpreter_transferTo(self, aFiber);
    } else {
        SpkInterpreter_putToSleep(self, aFiber);
    }
}


/*------------------------------------------------------------------------*/
/* debug support */

void SpkInterpreter_printCallStack(Interpreter *self) {
    Context *ctxt, *home;
    Method *method;
    Behavior *methodClass;
    Symbol *methodSel, *call;
    
    call = SpkSymbol_get("__apply__");
    for (ctxt = self->activeContext; ctxt; ctxt = ctxt->sender) {
        home = ctxt->homeContext;
        method = home->u.m.method;
        methodClass = home->u.m.methodClass;
        methodSel = SpkBehavior_findSelectorOfMethod(methodClass, method);
        printf("%04x %p%s",
               ctxt->pc - &method->opcodes[0], ctxt, (ctxt == home ? " " : " {} in "));
        if (methodSel == call) {
            printf("%s\n", SpkBehavior_name(methodClass));
        } else {
            printf("%s::%s\n",
                   SpkBehavior_name(methodClass),
                   methodSel ? methodSel->str : "<unknown>");
        }
    }
}


/*------------------------------------------------------------------------*/
/* error handling */

Fiber *SpkInterpreter_trap(Interpreter *self, Symbol *selector, Object *argument) {
    /* XXX */
    SpkInterpreter_halt(self, selector, argument);
    return 0;
}

void SpkInterpreter_unknownOpcode(Interpreter *self) {
    SpkInterpreter_trap(self, self->selectorUnknownOpcode, 0);
}

void SpkInterpreter_halt(Interpreter *self, Symbol *selector, Object *argument) {
    if (argument) {
        if (argument->klass == ClassSymbol) {
            printf("\n%s '%s'", selector->str, ((Symbol *)argument)->str);
        } else if (argument->klass == ClassMessage) {
            printf("\n%s '%s'", selector->str, ((Message *)argument)->messageSelector->str);
        } else {
            printf("\n%s", selector->str);
        }
    } else {
        printf("\n%s", selector->str);
    }
    printf("\n\n");
    if (!self->printingStack) {
        self->printingStack = 1;
        SpkInterpreter_printCallStack(self);
    }
    exit(1);
}
