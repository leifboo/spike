
#ifndef __spk_interp_h__
#define __spk_interp_h__


#include "native.h"
#include "obj.h"
#include "oper.h"

#include <stddef.h>


typedef struct SpkMethod SpkMethod;
typedef struct SpkContext SpkContext;
typedef struct SpkFiber SpkFiber;


typedef unsigned char SpkOpcode;

enum SpkOpcode {
    Spk_OPCODE_NOP,
    Spk_OPCODE_PUSH_LOCAL,
    Spk_OPCODE_PUSH_INST_VAR,
    Spk_OPCODE_PUSH_GLOBAL,
    Spk_OPCODE_PUSH_LITERAL,
    Spk_OPCODE_PUSH_SELF,
    Spk_OPCODE_PUSH_SUPER,
    Spk_OPCODE_PUSH_FALSE,
    Spk_OPCODE_PUSH_TRUE,
    Spk_OPCODE_PUSH_NULL,
    Spk_OPCODE_PUSH_VOID,
    Spk_OPCODE_PUSH_CONTEXT,
    Spk_OPCODE_DUP,
    Spk_OPCODE_DUP_N,
    Spk_OPCODE_STORE_LOCAL,
    Spk_OPCODE_STORE_INST_VAR,
    Spk_OPCODE_STORE_GLOBAL,
    Spk_OPCODE_POP,
    Spk_OPCODE_ROT,
    Spk_OPCODE_BRANCH_IF_FALSE,
    Spk_OPCODE_BRANCH_IF_TRUE,
    Spk_OPCODE_BRANCH_ALWAYS,
    Spk_OPCODE_ID,
    Spk_OPCODE_OPER,
    Spk_OPCODE_OPER_SUPER,
    Spk_OPCODE_CALL,
    Spk_OPCODE_CALL_VAR,
    Spk_OPCODE_CALL_SUPER,
    Spk_OPCODE_CALL_SUPER_VAR,
    Spk_OPCODE_GET_ATTR,
    Spk_OPCODE_GET_ATTR_SUPER,
    Spk_OPCODE_GET_ATTR_VAR,
    Spk_OPCODE_GET_ATTR_VAR_SUPER,
    Spk_OPCODE_SET_ATTR,
    Spk_OPCODE_SET_ATTR_SUPER,
    Spk_OPCODE_SET_ATTR_VAR,
    Spk_OPCODE_SET_ATTR_VAR_SUPER,
    Spk_OPCODE_SEND_MESSAGE,
    Spk_OPCODE_SEND_MESSAGE_SUPER,
    Spk_OPCODE_SEND_MESSAGE_VAR,
    Spk_OPCODE_SEND_MESSAGE_SUPER_VAR,
    Spk_OPCODE_RAISE,
    Spk_OPCODE_RET,
    Spk_OPCODE_RET_TRAMP,
    Spk_OPCODE_LEAF,
    Spk_OPCODE_SAVE,
    Spk_OPCODE_ARG,
    Spk_OPCODE_ARG_VAR,
    Spk_OPCODE_NATIVE,
    Spk_OPCODE_RESTORE_SENDER,
    Spk_OPCODE_RESTORE_CALLER,
    Spk_OPCODE_THUNK,
    Spk_OPCODE_CALL_THUNK,
    Spk_OPCODE_CALL_BLOCK,
    Spk_OPCODE_CHECK_STACKP,
    
    Spk_NUM_OPCODES
};


enum {
    /* These values are arbitrary. */
    Spk_LEAF_MAX_ARGUMENT_COUNT = 3,
    Spk_LEAF_MAX_STACK_SIZE = 5
};

struct SpkContext {
    SpkVariableObject base;
    SpkContext *caller; /* a.k.a. "sender" */
    SpkOpcode *pc;
    SpkUnknown **stackp;
    SpkContext *homeContext;
    union {
        struct /* MethodContext */ {
            SpkMethod *method;
            struct SpkBehavior *methodClass;
            SpkUnknown *receiver;
            SpkUnknown **framep;
        } m;
        struct /* BlockContext */ {
            size_t index;
            size_t nargs;
            SpkOpcode *startpc;
        } b;
    } u;
    int *mark;
};

typedef struct SpkContextSubclass {
    SpkContext base;
    SpkUnknown *variables[1]; /* stretchy */
} SpkContextSubclass;

#define SpkContext_VARIABLES(op) ((SpkUnknown **)SpkVariableObject_ITEM_BASE(op))


typedef struct SpkSemaphore {
    SpkObject base;
    SpkFiber *firstLink;
    SpkFiber *lastLink;
    int excessSignals;
} SpkSemaphore;


struct SpkFiber {
    SpkObject base;
    SpkFiber *nextLink;
    SpkContext *suspendedContext;
    SpkContext *leafContext;
    int priority;
    SpkSemaphore *myList;
};

typedef struct SpkFiberSubclass {
    SpkFiber base;
    SpkUnknown *variables[1]; /* stretchy */
} SpkFiberSubclass;


typedef struct SpkProcessorScheduler {
    SpkObject base;
    SpkSemaphore **quiescentFiberLists;
    SpkFiber *activeFiber;
} SpkProcessorScheduler;

typedef struct SpkProcessorSchedulerSubclass {
    SpkProcessorScheduler base;
    SpkUnknown *variables[1]; /* stretchy */
} SpkProcessorSchedulerSubclass;


struct SpkMethod {
    SpkVariableObject base;
    SpkMethod *nextInScope;
    struct {
        struct SpkBehavior *first;
        struct SpkBehavior *last;
    } nestedClassList;
    struct {
        struct SpkMethod *first;
        struct SpkMethod *last;
    } nestedMethodList;
    SpkNativeCode nativeCode;
};

typedef struct SpkMethodSubclass {
    SpkMethod base;
    SpkUnknown *variables[1]; /* stretchy */
} SpkMethodSubclass;

#define SpkMethod_OPCODES(op) ((SpkOpcode *)SpkVariableObject_ITEM_BASE(op))


typedef struct SpkThunk {
    SpkObject base;
    SpkUnknown *receiver;
    SpkMethod *method;
    struct SpkBehavior *methodClass;
    SpkOpcode *pc;
} SpkThunk;

typedef struct SpkThunkSubclass {
    SpkThunk base;
    SpkUnknown *variables[1]; /* stretchy */
} SpkThunkSubclass;


typedef struct SpkMessage {
    SpkObject base;
    unsigned int namespace;
    SpkUnknown *selector;
    SpkUnknown *arguments;
} SpkMessage;

typedef struct SpkMessageSubclass {
    SpkMessage base;
    SpkUnknown *variables[1]; /* stretchy */
} SpkMessageSubclass;


typedef struct SpkInterpreter {
    SpkObject base;

    /* fibers */
    SpkProcessorScheduler *scheduler;
    SpkSemaphore *theInterruptSemaphore;
    int interruptPending;

    /* contexts */
    SpkContext *activeContext;
    SpkContext *newContext;

    /* error handling */
    int printingStack;

} SpkInterpreter;

typedef struct SpkInterpreterSubclass {
    SpkInterpreter base;
    SpkUnknown *variables[1]; /* stretchy */
} SpkInterpreterSubclass;


extern SpkUnknown *Spk_null, *Spk_uninit, *Spk_void;

extern struct SpkBehavior *Spk_ClassMessage,*Spk_ClassMethod, *Spk_ClassThunk, *Spk_ClassContext, *Spk_ClassMethodContext, *Spk_ClassBlockContext;
extern struct SpkBehavior *Spk_ClassInterpreter, *Spk_ClassProcessorScheduler, *Spk_ClassFiber;
extern struct SpkBehavior *Spk_ClassNull, *Spk_ClassUninit, *Spk_ClassVoid;
extern struct SpkClassTmpl Spk_ClassMessageTmpl, Spk_ClassMethodTmpl, Spk_ClassThunkTmpl, Spk_ClassContextTmpl, Spk_ClassMethodContextTmpl, Spk_ClassBlockContextTmpl;
extern struct SpkClassTmpl Spk_ClassInterpreterTmpl, Spk_ClassProcessorSchedulerTmpl, Spk_ClassFiberTmpl;
extern struct SpkClassTmpl Spk_ClassNullTmpl, Spk_ClassUninitTmpl, Spk_ClassVoidTmpl;
extern SpkInterpreter *theInterpreter; /* XXX */


SpkMessage *SpkMessage_New(void);
SpkMethod *SpkMethod_New(size_t);

SpkContext *SpkContext_New(size_t);

SpkSemaphore *SpkSemaphore_New(void);
int SpkSemaphore_IsEmpty(SpkSemaphore *);
void SpkSemaphore_AddLast(SpkSemaphore *, SpkFiber *);
SpkFiber *SpkSemaphore_RemoveFirst(SpkSemaphore *);

void SpkInterpreter_Boot(void);
SpkInterpreter *SpkInterpreter_New(void);
void SpkInterpreter_Init(SpkInterpreter *, SpkProcessorScheduler *);
SpkUnknown *SpkInterpreter_Interpret(SpkInterpreter *);
void SpkInterpreter_SynchronousSignal(SpkInterpreter *, SpkSemaphore *);
void SpkInterpreter_TransferTo(SpkInterpreter *, SpkFiber *);
SpkFiber *SpkInterpreter_WakeHighestPriority(SpkInterpreter *);
void SpkInterpreter_PutToSleep(SpkInterpreter *, SpkFiber *);
void SpkInterpreter_Resume(SpkInterpreter *, SpkFiber *);
void SpkInterpreter_PrintCallStack(SpkInterpreter *);


#endif /* __spk_interp_h__ */
