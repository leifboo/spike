
#ifndef __spk_interp_h__
#define __spk_interp_h__


#include "native.h"
#include "obj.h"
#include "oper.h"

#include <stddef.h>


struct SpkBehavior;


typedef struct SpkContext SpkContext;
typedef struct SpkFiber SpkFiber;
typedef struct SpkInterpreter SpkInterpreter;
typedef struct SpkMessage SpkMessage;
typedef struct SpkMethod SpkMethod;
typedef struct SpkProcessorScheduler SpkProcessorScheduler;
typedef struct SpkSemaphore SpkSemaphore;
typedef struct SpkThunk SpkThunk;


typedef unsigned char SpkOpcode;

enum /*SpkOpcode*/ {
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
    Spk_OPCODE_SET_IND,
    Spk_OPCODE_SET_IND_SUPER,
    Spk_OPCODE_SET_INDEX,
    Spk_OPCODE_SET_INDEX_VAR,
    Spk_OPCODE_SET_INDEX_SUPER,
    Spk_OPCODE_SET_INDEX_SUPER_VAR,
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
    Spk_OPCODE_NATIVE_PUSH_INST_VAR,
    Spk_OPCODE_NATIVE_STORE_INST_VAR,
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


#define SpkContext_VARIABLES(op) ((SpkUnknown **)SpkVariableObject_ITEM_BASE(op))
#define SpkMethod_OPCODES(op) ((SpkOpcode *)SpkVariableObject_ITEM_BASE(op))


struct SpkMethod {
    SpkVariableObject base;
    struct {
        SpkUnknown *source;
        size_t lineCodeTally;
        SpkOpcode *lineCodes;
    } debug;
    SpkNativeCode nativeCode;
};


struct SpkMessage {
    SpkObject base;
    unsigned int ns;
    SpkUnknown *selector;
    SpkUnknown *arguments;
};


extern struct SpkClassTmpl Spk_ClassMessageTmpl, Spk_ClassMethodTmpl, Spk_ClassThunkTmpl, Spk_ClassContextTmpl, Spk_ClassMethodContextTmpl, Spk_ClassBlockContextTmpl;
extern struct SpkClassTmpl Spk_ClassInterpreterTmpl, Spk_ClassProcessorSchedulerTmpl, Spk_ClassFiberTmpl;
extern struct SpkClassTmpl Spk_ClassNullTmpl, Spk_ClassUninitTmpl, Spk_ClassVoidTmpl;


SpkMessage *SpkMessage_New(void);
SpkMethod *SpkMethod_New(size_t);
SpkThunk *SpkThunk_New(SpkUnknown *, SpkMethod *, struct SpkBehavior *);

SpkContext *SpkContext_New(size_t);

SpkSemaphore *SpkSemaphore_New(void);
int SpkSemaphore_IsEmpty(SpkSemaphore *);
void SpkSemaphore_AddLast(SpkSemaphore *, SpkFiber *);
SpkFiber *SpkSemaphore_RemoveFirst(SpkSemaphore *);

void SpkInterpreter_Boot(void);
SpkInterpreter *SpkInterpreter_New(void);
void SpkInterpreter_Init(SpkInterpreter *, SpkProcessorScheduler *);
SpkUnknown *SpkInterpreter_Interpret(SpkInterpreter *);
SpkUnknown *SpkInterpreter_SendMessage(SpkInterpreter *, SpkUnknown *,
                                       unsigned int, SpkUnknown *, SpkUnknown *);
void SpkInterpreter_SynchronousSignal(SpkInterpreter *, SpkSemaphore *);
void SpkInterpreter_TransferTo(SpkInterpreter *, SpkFiber *);
SpkFiber *SpkInterpreter_WakeHighestPriority(SpkInterpreter *);
void SpkInterpreter_PutToSleep(SpkInterpreter *, SpkFiber *);
void SpkInterpreter_Resume(SpkInterpreter *, SpkFiber *);
void SpkInterpreter_PrintCallStack(SpkInterpreter *);


#endif /* __spk_interp_h__ */
