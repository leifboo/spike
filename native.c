
#include "native.h"

#include "array.h"
#include "behavior.h"
#include "interp.h"
#include "obj.h"
#include "sym.h"
#include <assert.h>


Method *Spk_newNativeMethod(SpkNativeCodeFlags flags, SpkNativeCode nativeCode) {
    Method *newMethod;
    size_t argumentCount;
    size_t size;
    opcode_t *ip;
    
    size = 0;
    if (flags & SpkNativeCode_THUNK) {
        size += 1; /* thunk */
    }
    if (flags & SpkNativeCode_LEAF) {
        size += 2; /* leaf */
    } else {
        size += 11; /* save, ... restore */
    }
    size += 1; /* ret/retl */
    
    switch (flags & SpkNativeCode_SIGNATURE_MASK) {
    case SpkNativeCode_ARGS_0: argumentCount = 0; break;
    case SpkNativeCode_ARGS_1: argumentCount = 1; break;
    case SpkNativeCode_ARGS_2: argumentCount = 2; break;
    default: assert(0); /* XXX */
    }
    
    newMethod = SpkMethod_new(size);
    newMethod->nativeCode = nativeCode;
    
    ip = &newMethod->opcodes[0];
    if (flags & SpkNativeCode_THUNK) {
        *ip++ = OPCODE_THUNK;
    }
    if (flags & SpkNativeCode_LEAF) {
        *ip++ = OPCODE_LEAF;
        *ip++ = (opcode_t)argumentCount;
    } else {
        *ip++ = OPCODE_SAVE;
        *ip++ = (opcode_t)argumentCount;
        *ip++ = (opcode_t)0; /* localCount */
        *ip++ = (opcode_t)3 + LEAF_STACK_SPACE; /* stackSize */
        
        /* skip trampoline code */
        *ip++ = OPCODE_BRANCH_ALWAYS;
        *ip++ = 6;
        
        /* trampolines for re-entering interpreted code */
        *ip++ = OPCODE_SEND_MESSAGE;
        *ip++ = OPCODE_RET_TRAMP;
        *ip++ = OPCODE_SEND_MESSAGE_SUPER;
        *ip++ = OPCODE_RET_TRAMP;
        
        *ip++ = OPCODE_RESTORE_SENDER;
    }
    
    if (flags & SpkNativeCode_LEAF) {
        *ip++ = OPCODE_RET_LEAF;
    } else {
        *ip++ = OPCODE_RET;
    }
    
    return newMethod;
}


/*------------------------------------------------------------------------*/
/* routines to send messages from native code */

static Object *sendMessage(Interpreter *interpreter,
                           Object *obj, Symbol *selector, Array *argumentArray)
{
    Context *thisContext;
    Method *method;
    opcode_t *oldPC; Object **oldSP;
    Object *result;
    
    thisContext = interpreter->activeContext;
    assert(thisContext == thisContext->homeContext);
    method = thisContext->u.m.method;
    
    oldPC = thisContext->pc;
    oldSP = thisContext->stackp;
    
    assert(*thisContext->pc == OPCODE_BRANCH_ALWAYS && "call from non-leaf native method");
    /* move the program counter to the trampoline code */
    thisContext->pc += obj ? 2 : 4;
    
    /* push arguments on the stack */
    *--thisContext->stackp = obj ? obj : thisContext->u.m.receiver;
    *--thisContext->stackp = (Object *)selector;
    *--thisContext->stackp = (Object *)argumentArray;
    assert(thisContext->stackp >= &thisContext->variables[LEAF_STACK_SPACE]);
    
    /* interpret */
    result = SpkInterpreter_interpret(interpreter);
    
    thisContext->pc = oldPC;
    assert(thisContext->stackp == oldSP);
    
    return result;
}

static Object *vSendMessage(Interpreter *interpreter,
                            Object *obj, Symbol *selector, va_list ap)
{
    return sendMessage(interpreter, obj, selector, SpkArray_fromVAList(ap));
}

Object *Spk_oper(Interpreter *interpreter, Object *obj, Oper oper, ...) {
    Object *result;
    va_list ap;
    
    va_start(ap, oper);
    result = Spk_vOper(interpreter, obj, oper, ap);
    va_end(ap);
    return result;
}

Object *Spk_vOper(Interpreter *interpreter, Object *obj, Oper oper, va_list ap) {
    return vSendMessage(interpreter, obj, operSelectors[oper].messageSelector, ap);
}

Object *Spk_call(Interpreter *interpreter, Object *obj, CallOper oper, ...) {
    Object *result;
    va_list ap;
    
    va_start(ap, oper);
    result = Spk_vCall(interpreter, obj, oper, ap);
    va_end(ap);
    return result;
}

Object *Spk_vCall(Interpreter *interpreter, Object *obj, CallOper oper, va_list ap) {
    return vSendMessage(interpreter, obj, operCallSelectors[oper].messageSelector, ap);
}

Object *Spk_attr(Interpreter *interpreter, Object *obj, Symbol *name) {
    return sendMessage(interpreter, obj, name, SpkArray_new(0));
}

Object *Spk_callAttr(Interpreter *interpreter, Object *obj, Symbol *name, ...) {
    Object *result, *thunk;
    va_list ap;
    
    va_start(ap, name);
    thunk = Spk_attr(interpreter, obj, name);
    result = Spk_vCall(interpreter, thunk, OPER_APPLY, ap);
    va_end(ap);
    return result;
}
