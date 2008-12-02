
#ifndef __interp_h__
#define __interp_h__


#include "obj.h"
#include "oper.h"

#include <stddef.h>


typedef struct Method Method;
typedef struct Context Context;
typedef struct Fiber Fiber;


typedef unsigned char opcode_t;

enum Opcode {
    OPCODE_NOP,
    OPCODE_PUSH_LOCAL,
    OPCODE_PUSH_INST_VAR,
    OPCODE_PUSH_GLOBAL,
    OPCODE_PUSH_SELF,
    OPCODE_PUSH_FALSE,
    OPCODE_PUSH_TRUE,
    OPCODE_PUSH_NULL,
    OPCODE_PUSH_VOID,
    OPCODE_PUSH_CONTEXT,
    OPCODE_DUP,
    OPCODE_DUP_N,
    OPCODE_PUSH_INT,
    OPCODE_STORE_LOCAL,
    OPCODE_STORE_INST_VAR,
    OPCODE_STORE_GLOBAL,
    OPCODE_POP,
    OPCODE_ROT,
    OPCODE_BRANCH_IF_FALSE,
    OPCODE_BRANCH_IF_TRUE,
    OPCODE_BRANCH_ALWAYS,
    OPCODE_ID,
    OPCODE_OPER,
    OPCODE_OPER_SUPER,
    OPCODE_CALL,
    OPCODE_CALL_SUPER,
    OPCODE_ATTR,
    OPCODE_ATTR_SUPER,
    OPCODE_RET,
    OPCODE_RETT,
    OPCODE_SAVE,
    OPCODE_RESTORE_SENDER,
    OPCODE_RESTORE_CALLER,
    OPCODE_THUNK,
    OPCODE_CALL_THUNK,
    OPCODE_TRAP_NATIVE,
    OPCODE_CHECK_STACKP,
    
    NUM_OPCODES
};


typedef ObjectSubclass Array;
typedef Object Null, Uninit, Void;


struct Context {
    Object base;
    Context *caller; /* a.k.a. "sender" */
    opcode_t *pc;
    Object **stackp;
    Context *homeContext;
    union {
        struct /* MethodContext */ {
            Method *method;
            struct Behavior *methodClass;
            Object *receiver;
        } m;
        struct /* BlockContext */ {
            size_t nargs;
            opcode_t *startpc;
        } b;
    } u;
    Object *variables[1]; /* stretchy */
};


typedef struct Semaphore {
    Object base;
    Fiber *firstLink;
    Fiber *lastLink;
    int excessSignals;
} Semaphore;


struct Fiber {
    Object base;
    Fiber *nextLink;
    Context *suspendedContext;
    int priority;
    Semaphore *myList;
};


typedef struct ProcessorScheduler {
    Object base;
    Semaphore **quiescentFiberLists;
    Fiber *activeFiber;
} ProcessorScheduler;


typedef enum SpkNativeCodeFlags {
    SpkNativeCode_ARGS_0              = 0x0000,
    SpkNativeCode_ARGS_1              = 0x0001,
    SpkNativeCode_ARGS_2              = 0x0002,
    SpkNativeCode_ARGS_TUPLE          = 0x0003,
    SpkNativeCode_ARGS_KEYWORDS       = 0x0004,
    SpkNativeCode_ARGS_TUPLE_KEYWORDS = 0x0007,
    SpkNativeCode_SIGNATURE_MASK      = 0x000f,
    
    SpkNativeCode_LEAF                = 0x0010,
    SpkNativeCode_CALLABLE            = 0x0020
} SpkNativeCodeFlags;

typedef Object *(*SpkNativeCode)(Object *, Object *, Object *);

struct Method {
    Object base;
    SpkNativeCode nativeCode;
    size_t argumentCount;
    size_t localCount;
    size_t stackSize;
    size_t size;
    opcode_t opcodes[1]; /* stretchy */
};

#define contextSizeForMethod(m) ((m)->stackSize + (m)->argumentCount + (m)->localCount)


typedef struct Thunk {
    Object base;
    Object *receiver;
    Method *method;
    struct Behavior *methodClass;
    opcode_t *pc;
} Thunk;

typedef struct ThunkSubclass {
    Thunk base;
    Object *variables[1]; /* stretchy */
} ThunkSubclass;


typedef struct Message {
    Object base;
    struct Symbol *messageSelector;
    Array *argumentArray;
} Message;

typedef struct MessageSubclass {
    Message base;
    Object *variables[1]; /* stretchy */
} MessageSubclass;


typedef struct Interpreter {
    Object base;

    /* fibers */
    ProcessorScheduler *scheduler;
    Semaphore *theInterruptSemaphore;
    int interruptPending;

    /* contexts */
    Context *activeContext;
    Context *newContext;

    /* special objects */
    struct Symbol *selectorCannotReturn;
    struct Symbol *selectorDoesNotUnderstand;
    struct Symbol *selectorMustBeBoolean;
    struct Symbol *selectorNoRunnableFiber;
    struct Symbol *selectorUnknownOpcode;
    struct Symbol *selectorWrongNumberOfArguments;

    /* error handling */
    int printingStack;

} Interpreter;


extern Null *Spk_null;
extern Uninit *Spk_uninit;
extern Void *Spk_void;

extern struct Behavior *ClassMessage, *ClassThunk, *ClassNull, *ClassUninit, *ClassVoid;
extern struct SpkClassTmpl ClassMessageTmpl, ClassThunkTmpl, ClassNullTmpl, ClassUninitTmpl, ClassVoidTmpl;


Object *SpkObject_new(size_t);

Message *SpkMessage_new(void);
Method *SpkMethod_new(size_t, size_t, size_t, size_t);
Method *SpkMethod_newNative(SpkNativeCodeFlags, SpkNativeCode);

Context *SpkContext_new(size_t);
Context *SpkContext_blockCopy(Context *, size_t, opcode_t *);
void SpkContext_init(Context *, Context *);
void SpkContext_initWithArg(Context *, Object *, Context *);

Semaphore *SpkSemaphore_new(void);
int SpkSemaphore_isEmpty(Semaphore *);
void SpkSemaphore_addLast(Semaphore *, Fiber *);
Fiber *SpkSemaphore_removeFirst(Semaphore *);

Object *SpkInterpreter_start(Object *);
void SpkInterpreter_init(Interpreter *, ProcessorScheduler *);
Object *SpkInterpreter_interpret(Interpreter *);
Object **SpkInterpreter_instanceVars(Object *);
void SpkInterpreter_synchronousSignal(Interpreter *, Semaphore *);
void SpkInterpreter_transferTo(Interpreter *, Fiber *);
Fiber *SpkInterpreter_wakeHighestPriority(Interpreter *);
void SpkInterpreter_putToSleep(Interpreter *, Fiber *);
void SpkInterpreter_resume(Interpreter *, Fiber *);
void SpkInterpreter_printCallStack(Interpreter *);
Fiber *SpkInterpreter_trap(Interpreter *, struct Symbol *, Object *);
void SpkInterpreter_unknownOpcode(Interpreter *);
void SpkInterpreter_halt(Interpreter *, struct Symbol *, Object *);


#endif /* __interp_h__ */
