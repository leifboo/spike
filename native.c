
#include "native.h"

#include "array.h"
#include "behavior.h"
#include "interp.h"
#include "obj.h"
#include "sym.h"
#include <assert.h>
#include <stdlib.h>


typedef struct NativeAccessor {
    Method base;
    SpkInstVarType type;
    size_t offset;
} NativeAccessor;

typedef struct NativeAccessorSubclass {
    NativeAccessor base;
    Object *variables[1]; /* stretchy */
} NativeAccessorSubclass;


typedef Method *new_method_hook_t(size_t);


static Object *nativeReadAccessor(Object *, Object *, Object *);
static Object *nativeWriteAccessor(Object *, Object *, Object *);


Behavior *Spk_ClassNativeAccessor;


static Method *newNativeMethod(SpkNativeCodeFlags flags, SpkNativeCode nativeCode, new_method_hook_t *newMethodHook) {
    Method *newMethod;
    size_t argumentCount, variadic;
    size_t size;
    opcode_t *ip;
    
    size = 0;
    if (flags & SpkNativeCode_THUNK) {
        size += 1; /* thunk */
    }
    if (flags & SpkNativeCode_LEAF) {
        size += 2; /* leaf */
    } else {
        size += 12; /* save, ... restore */
    }
    size += 1; /* ret/retl */
    
    variadic = 0;
    switch (flags & SpkNativeCode_SIGNATURE_MASK) {
    case SpkNativeCode_ARGS_0: argumentCount = 0; break;
    case SpkNativeCode_ARGS_1: argumentCount = 1; break;
    case SpkNativeCode_ARGS_2: argumentCount = 2; break;
        
    case SpkNativeCode_ARGS_ARRAY:
        argumentCount = 0;
        variadic = 1;
        break;
        
    default: assert(0); /* XXX */
    }
    
    newMethod = (*newMethodHook)(size);
    newMethod->nativeCode = nativeCode;
    
    ip = SpkMethod_OPCODES(newMethod);
    if (flags & SpkNativeCode_THUNK) {
        *ip++ = OPCODE_THUNK;
    }
    if (flags & SpkNativeCode_LEAF) {
        assert(!variadic && "SpkNativeCode_ARGS_ARRAY cannot be combined with SpkNativeCode_LEAF");
        *ip++ = OPCODE_LEAF;
        *ip++ = (opcode_t)argumentCount;
    } else {
        *ip++ = variadic ? OPCODE_SAVE_VAR : OPCODE_SAVE;
        *ip++ = 2; /* displaySize */
        *ip++ = (opcode_t)argumentCount;
        *ip++ = (opcode_t)variadic; /* localCount */
        *ip++ = (opcode_t)3; /* stackSize */
        
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

Method *Spk_newNativeMethod(SpkNativeCodeFlags flags, SpkNativeCode nativeCode) {
    return newNativeMethod(flags, nativeCode, &SpkMethod_new);
}

Method *SpkNativeAccessor_new(size_t size) {
    NativeAccessor *newAccessor;
    
    newAccessor = (NativeAccessor *)malloc(sizeof(NativeAccessor) + size*sizeof(opcode_t));
    newAccessor->base.base.base.klass = Spk_ClassNativeAccessor;
    newAccessor->base.base.size = size;
    return (Method *)newAccessor;
}

static NativeAccessor *newNativeAccessor(unsigned int type, size_t offset,
                                         SpkNativeCodeFlags flags, SpkNativeCode nativeCode) {
    NativeAccessor *newAccessor;
    
    if (0) {
        /* XXX: See the comment in Spk_thisMethod() */
        flags |= SpkNativeCode_LEAF;
    }
    newAccessor = (NativeAccessor *)newNativeMethod(flags, nativeCode, &SpkNativeAccessor_new);
    newAccessor->type = type;
    newAccessor->offset = offset;
    return newAccessor;
}

struct Method *Spk_newNativeReadAccessor(unsigned int type, size_t offset) {
    return (Method *)newNativeAccessor(type, offset, SpkNativeCode_ARGS_0, &nativeReadAccessor);
}

struct Method *Spk_newNativeWriteAccessor(unsigned int type, size_t offset) {
    return (Method *)newNativeAccessor(type, offset, SpkNativeCode_ARGS_1, &nativeWriteAccessor);
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
    assert(thisContext->stackp >= &SpkContext_VARIABLES(thisContext)[0]);
    
    /* interpret */
    result = SpkInterpreter_interpret(interpreter);
    if (!result)
        return 0; /* unwind */
    
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
    return vSendMessage(interpreter, obj, Spk_operSelectors[oper].messageSelector, ap);
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
    return vSendMessage(interpreter, obj, Spk_operCallSelectors[oper].messageSelector, ap);
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


/*------------------------------------------------------------------------*/
/* misc. support routines */

Method *Spk_thisMethod(Interpreter *interpreter) {
    /* XXX: Make this work inside leaf methods. */
    return interpreter->activeContext->homeContext->u.m.method;
}


/*------------------------------------------------------------------------*/
/* class NativeAccessor */

static SpkMethodTmpl NativeAccessorMethods[] = {
    { 0, 0, 0}
};

SpkClassTmpl Spk_ClassNativeAccessorTmpl = {
    "NativeAccessor",
    offsetof(NativeAccessorSubclass, variables),
    sizeof(NativeAccessor),
    sizeof(opcode_t),
    0,
    NativeAccessorMethods
};

static Object *nativeReadAccessor(Object *self, Object *arg0, Object *arg1) {
    NativeAccessor *accessor;
    char *addr;
    Object *result;
    
    assert(accessor = Spk_CAST(NativeAccessor, Spk_thisMethod(theInterpreter)));
    
    addr = (char *)self + accessor->offset;
    
    switch (accessor->type) {
    case Spk_T_OBJECT:
        result = *(Object **)addr;
        if (!result)
            result = Spk_null;
        break;
        
    case Spk_T_SIZE:
        result = (Object *)SpkInteger_fromLong(*(long *)addr);
        break;
        
    default:
        assert(0 && "XXX");
    }
    
    return result;
}

static Object *nativeWriteAccessor(Object *self, Object *arg0, Object *arg1) {
    NativeAccessor *accessor;
    char *addr;
    
    assert(accessor = Spk_CAST(NativeAccessor, Spk_thisMethod(theInterpreter)));
    
    addr = (char *)self + accessor->offset;
    
    switch (accessor->type) {
    case Spk_T_OBJECT:
        *(Object **)addr = arg0;
        break;
        
    default:
        assert(0 && "XXX");
    }
    
    return Spk_void;
}
