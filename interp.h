
#ifndef __interp_h__
#define __interp_h__


#include "native.h"
#include "obj.h"
#include "oper.h"

#include <stddef.h>


struct Behavior;


typedef struct Context Context;
typedef struct Fiber Fiber;
typedef struct Interpreter Interpreter;
typedef struct Message Message;
typedef struct Method Method;
typedef struct ProcessorScheduler ProcessorScheduler;
typedef struct Semaphore Semaphore;


typedef unsigned char Opcode;

enum /*Opcode*/ {
    OPCODE_NOP,
    OPCODE_PUSH_LOCAL,
    OPCODE_PUSH_INST_VAR,
    OPCODE_PUSH_GLOBAL,
    OPCODE_PUSH_LITERAL,
    OPCODE_PUSH_SELF,
    OPCODE_PUSH_SUPER,
    OPCODE_PUSH_FALSE,
    OPCODE_PUSH_TRUE,
    OPCODE_PUSH_NULL,
    OPCODE_PUSH_VOID,
    OPCODE_PUSH_CONTEXT,
    OPCODE_DUP,
    OPCODE_DUP_N,
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
    OPCODE_CALL_VA,
    OPCODE_CALL_SUPER,
    OPCODE_CALL_SUPER_VA,
    OPCODE_SET_IND,
    OPCODE_SET_IND_SUPER,
    OPCODE_SET_INDEX,
    OPCODE_SET_INDEX_VA,
    OPCODE_SET_INDEX_SUPER,
    OPCODE_SET_INDEX_SUPER_VA,
    OPCODE_GET_ATTR,
    OPCODE_GET_ATTR_SUPER,
    OPCODE_GET_ATTR_VAR,
    OPCODE_GET_ATTR_VAR_SUPER,
    OPCODE_SET_ATTR,
    OPCODE_SET_ATTR_SUPER,
    OPCODE_SET_ATTR_VAR,
    OPCODE_SET_ATTR_VAR_SUPER,
    OPCODE_SEND_MESSAGE,
    OPCODE_SEND_MESSAGE_VA,
    OPCODE_SEND_MESSAGE_SUPER,
    OPCODE_SEND_MESSAGE_SUPER_VA,
    OPCODE_SEND_MESSAGE_VAR,
    OPCODE_SEND_MESSAGE_VAR_VA,
    OPCODE_SEND_MESSAGE_SUPER_VAR,
    OPCODE_SEND_MESSAGE_SUPER_VAR_VA,
    OPCODE_SEND_MESSAGE_NS_VAR_VA,
    OPCODE_SEND_MESSAGE_SUPER_NS_VAR_VA,
    OPCODE_RAISE,
    OPCODE_RET,
    OPCODE_RET_TRAMP,
    OPCODE_LEAF,
    OPCODE_SAVE,
    OPCODE_ARG,
    OPCODE_ARG_VA,
    OPCODE_NATIVE,
    OPCODE_NATIVE_PUSH_INST_VAR,
    OPCODE_NATIVE_STORE_INST_VAR,
    OPCODE_RESTORE_SENDER,
    OPCODE_RESTORE_CALLER,
    OPCODE_CALL_BLOCK,
    OPCODE_CHECK_STACKP,
    
    NUM_OPCODES
};


enum {
    /* These values are arbitrary. */
    LEAF_MAX_ARGUMENT_COUNT = 3,
    LEAF_MAX_STACK_SIZE = 5
};


#define Context_VARIABLES(op) ((Unknown **)VariableObject_ITEM_BASE(op))
#define Method_OPCODES(op) ((Opcode *)VariableObject_ITEM_BASE(op))


struct Method {
    VariableObject base;
    struct {
        Unknown *source;
        size_t lineCodeTally;
        Opcode *lineCodes;
    } debug;
    NativeCode nativeCode;
};


struct Message {
    Object base;
    unsigned int ns;
    Unknown *selector;
    Unknown *arguments;
};


extern struct ClassTmpl ClassMessageTmpl, ClassMethodTmpl, ClassContextTmpl, ClassMethodContextTmpl, ClassBlockContextTmpl;
extern struct ClassTmpl ClassInterpreterTmpl, ClassProcessorSchedulerTmpl, ClassFiberTmpl;
extern struct ClassTmpl ClassNullTmpl, ClassUninitTmpl, ClassVoidTmpl;


Message *Message_New(void);
Method *Method_New(size_t);

Context *Context_New(size_t);

Semaphore *Semaphore_New(void);
int Semaphore_IsEmpty(Semaphore *);
void Semaphore_AddLast(Semaphore *, Fiber *);
Fiber *Semaphore_RemoveFirst(Semaphore *);

void Interpreter_Boot(void);
Interpreter *Interpreter_New(void);
void Interpreter_Init(Interpreter *, ProcessorScheduler *);
Unknown *Interpreter_Interpret(Interpreter *);
Unknown *Interpreter_SendMessage(Interpreter *, Unknown *,
                                       unsigned int, Unknown *, Unknown *);
void Interpreter_SynchronousSignal(Interpreter *, Semaphore *);
void Interpreter_TransferTo(Interpreter *, Fiber *);
Fiber *Interpreter_WakeHighestPriority(Interpreter *);
void Interpreter_PutToSleep(Interpreter *, Fiber *);
void Interpreter_Resume(Interpreter *, Fiber *);
void Interpreter_PrintCallStack(Interpreter *);


#endif /* __interp_h__ */
