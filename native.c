
#include "native.h"

#include "class.h"
#include "host.h"
#include "interp.h"
#include "obj.h"
#include "rodata.h"
#include <assert.h>
#include <stdarg.h>


typedef struct SpkNativeAccessor {
    SpkMethod base;
    SpkInstVarType type;
    size_t offset;
} SpkNativeAccessor;

typedef struct SpkNativeAccessorSubclass {
    SpkNativeAccessor base;
    SpkUnknown *variables[1]; /* stretchy */
} SpkNativeAccessorSubclass;


typedef SpkMethod *NewMethodHook(size_t);


static SpkUnknown *nativeReadAccessor(SpkUnknown *, SpkUnknown *, SpkUnknown *);
static SpkUnknown *nativeWriteAccessor(SpkUnknown *, SpkUnknown *, SpkUnknown *);


SpkBehavior *Spk_ClassNativeAccessor;


static SpkMethod *newNativeMethod(SpkNativeCodeFlags flags, SpkNativeCode nativeCode, NewMethodHook *newMethodHook) {
    SpkMethod *newMethod;
    size_t argumentCount, variadic;
    size_t size;
    SpkOpcode *ip;
    
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
        *ip++ = Spk_OPCODE_THUNK;
    }
    if (flags & SpkNativeCode_LEAF) {
        assert(!variadic && "SpkNativeCode_ARGS_ARRAY cannot be combined with SpkNativeCode_LEAF");
        *ip++ = Spk_OPCODE_LEAF;
        *ip++ = (SpkOpcode)argumentCount;
    } else {
        *ip++ = variadic ? Spk_OPCODE_SAVE_VAR : Spk_OPCODE_SAVE;
        *ip++ = (SpkOpcode)argumentCount;
        *ip++ = (SpkOpcode)variadic; /* localCount */
        *ip++ = (SpkOpcode)4; /* stackSize */
        
        /* skip trampoline code */
        *ip++ = Spk_OPCODE_BRANCH_ALWAYS;
        *ip++ = 6;
        
        /* trampolines for re-entering interpreted code */
        *ip++ = Spk_OPCODE_SEND_MESSAGE_VAR;
        *ip++ = Spk_OPCODE_RET_TRAMP;
        *ip++ = Spk_OPCODE_SEND_MESSAGE_SUPER_VAR;
        *ip++ = Spk_OPCODE_RET_TRAMP;
        
        *ip++ = Spk_OPCODE_RESTORE_SENDER;
    }
    
    if (flags & SpkNativeCode_LEAF) {
        *ip++ = Spk_OPCODE_RET_LEAF;
    } else {
        *ip++ = Spk_OPCODE_RET;
    }
    
    return newMethod;
}

SpkMethod *Spk_NewNativeMethod(SpkNativeCodeFlags flags, SpkNativeCode nativeCode) {
    return newNativeMethod(flags, nativeCode, &SpkMethod_New);
}

SpkMethod *SpkNativeAccessor_New(size_t size) {
    return (SpkMethod *)SpkObject_NewVar(Spk_ClassNativeAccessor, size);
}

static SpkNativeAccessor *newNativeAccessor(unsigned int type, size_t offset,
                                            SpkNativeCodeFlags flags, SpkNativeCode nativeCode) {
    SpkNativeAccessor *newAccessor;
    
    if (0) {
        /* XXX: See the comment in Spk_ThisMethod() */
        flags |= SpkNativeCode_LEAF;
    }
    newAccessor = (SpkNativeAccessor *)newNativeMethod(flags, nativeCode, &SpkNativeAccessor_New);
    newAccessor->type = type;
    newAccessor->offset = offset;
    return newAccessor;
}

struct SpkMethod *Spk_NewNativeReadAccessor(unsigned int type, size_t offset) {
    return (SpkMethod *)newNativeAccessor(type, offset, SpkNativeCode_ARGS_0, &nativeReadAccessor);
}

struct SpkMethod *Spk_NewNativeWriteAccessor(unsigned int type, size_t offset) {
    return (SpkMethod *)newNativeAccessor(type, offset, SpkNativeCode_ARGS_1, &nativeWriteAccessor);
}


/*------------------------------------------------------------------------*/
/* routines to send messages from native code */

SpkUnknown *Spk_SendMessage(SpkInterpreter *interpreter,
                            SpkUnknown *obj, unsigned int namespace, SpkUnknown *selector, SpkUnknown *argumentArray)
{
    static SpkMethod *start;
    SpkContext *thisContext;
    SpkMethod *method;
    SpkOpcode *oldPC; SpkUnknown **oldSP;
    SpkUnknown *result;
    
    thisContext = interpreter->activeContext;
    if (!thisContext) {
        /* call from Python or other foreign code */
        SpkObject *receiver;
        
        receiver = Spk_CAST(Object, obj);
        assert(receiver);
        
        if (!start) {
            start = Spk_NewNativeMethod(SpkNativeCode_ARGS_ARRAY, 0);
        }
        
        thisContext = SpkContext_New(10); /* XXX */
        thisContext->caller = 0;
        thisContext->pc = SpkMethod_OPCODES(start) + 4;
        thisContext->stackp = &SpkContext_VARIABLES(thisContext)[10]; /* XXX */
        thisContext->homeContext = thisContext;            Spk_INCREF(thisContext);
        thisContext->u.m.method = start;                   Spk_INCREF(start);
        thisContext->u.m.methodClass = receiver->klass;    Spk_INCREF(receiver->klass);
        thisContext->u.m.receiver = (SpkUnknown *)obj;     Spk_INCREF(obj);
        thisContext->u.m.framep = thisContext->stackp;
        
        interpreter->activeContext = thisContext; /* steal reference */
    }
    
    assert(thisContext == thisContext->homeContext);
    method = thisContext->u.m.method;
    
    oldPC = thisContext->pc;
    oldSP = thisContext->stackp;
    
    assert(*thisContext->pc == Spk_OPCODE_BRANCH_ALWAYS && "call from non-leaf native method");
    
    /* move the program counter to the trampoline code */
    if (obj) {
        thisContext->pc += 2;
    } else {
        thisContext->pc += 4;
        obj = thisContext->u.m.receiver;
    }
    
    /* push arguments on the stack */
    Spk_DECREF(Spk_uninit);
    Spk_DECREF(Spk_uninit);
    Spk_DECREF(Spk_uninit);
    Spk_DECREF(Spk_uninit);
    *--thisContext->stackp = obj;            Spk_INCREF(obj);
    *--thisContext->stackp = SpkHost_IntegerFromCLong(namespace);
    *--thisContext->stackp = selector;       Spk_INCREF(selector);
    *--thisContext->stackp = argumentArray;  Spk_INCREF(argumentArray);
    assert(thisContext->stackp >= &SpkContext_VARIABLES(thisContext)[0]);
    
    /* interpret */
    result = SpkInterpreter_Interpret(interpreter);
    if (!result)
        return 0; /* unwind */
    
    thisContext->pc = oldPC;
    assert(thisContext->stackp == oldSP);
    
    if (thisContext->u.m.method == start) {
        assert(interpreter->activeContext == thisContext);
        interpreter->activeContext = 0;
        Spk_DECREF(thisContext);
    }
    
    return result;
}

static SpkUnknown *vSendMessage(SpkInterpreter *interpreter,
                                SpkUnknown *obj, unsigned int namespace, SpkUnknown *selector, va_list ap)
{
    SpkUnknown *argumentList, *result;

    argumentList = SpkHost_ArgsFromVAList(ap);
    result = Spk_SendMessage(interpreter, obj, namespace, selector, argumentList);
    Spk_DECREF(argumentList);
    return result;
}

SpkUnknown *Spk_Oper(SpkInterpreter *interpreter, SpkUnknown *obj, SpkOper oper, ...) {
    SpkUnknown *result;
    va_list ap;
    
    va_start(ap, oper);
    result = Spk_VOper(interpreter, obj, oper, ap);
    va_end(ap);
    return result;
}

SpkUnknown *Spk_VOper(SpkInterpreter *interpreter, SpkUnknown *obj, SpkOper oper, va_list ap) {
    return vSendMessage(interpreter, obj, Spk_METHOD_NAMESPACE_RVALUE, *Spk_operSelectors[oper].selector, ap);
}

SpkUnknown *Spk_Call(SpkInterpreter *interpreter, SpkUnknown *obj, SpkCallOper oper, ...) {
    SpkUnknown *result;
    va_list ap;
    
    va_start(ap, oper);
    result = Spk_VCall(interpreter, obj, oper, ap);
    va_end(ap);
    return result;
}

SpkUnknown *Spk_VCall(SpkInterpreter *interpreter, SpkUnknown *obj, SpkCallOper oper, va_list ap) {
    return vSendMessage(interpreter, obj, Spk_METHOD_NAMESPACE_RVALUE, *Spk_operCallSelectors[oper].selector, ap);
}

SpkUnknown *Spk_Attr(SpkInterpreter *interpreter, SpkUnknown *obj, SpkUnknown *name) {
    SpkUnknown *argumentList, *result;

    argumentList = Spk_emptyArgs;
    Spk_INCREF(argumentList);
    result = Spk_SendMessage(interpreter, obj, Spk_METHOD_NAMESPACE_RVALUE, name, argumentList);
    Spk_DECREF(argumentList);
    return result;
}

SpkUnknown *Spk_CallAttr(SpkInterpreter *interpreter, SpkUnknown *obj, SpkUnknown *name, ...) {
    SpkUnknown *result, *thunk;
    va_list ap;
    
    va_start(ap, name);
    thunk = Spk_Attr(interpreter, obj, name);
    if (!thunk) {
        return 0;
    }
    result = Spk_VCall(interpreter, thunk, Spk_OPER_APPLY, ap);
    Spk_DECREF(thunk);
    va_end(ap);
    return result;
}


/*------------------------------------------------------------------------*/
/* halting */

void Spk_Halt(int code, const char *message) {
    SpkHost_Halt(code, message);
}

void Spk_HaltWithFormat(int code, const char *format, ...) {
    va_list args;
    
    va_start(args, format);
    SpkHost_VHaltWithFormat(code, format, args);
    va_end(args);
}

void Spk_HaltWithString(int code, SpkUnknown *message) {
    SpkHost_HaltWithString(code, message);
}


/*------------------------------------------------------------------------*/
/* argument processing */

int Spk_IsArgs(SpkUnknown *op) {
    return SpkHost_IsArgs(op);
}

size_t Spk_ArgsSize(SpkUnknown *args) {
    return SpkHost_ArgsSize(args);
}

SpkUnknown *Spk_GetArg(SpkUnknown *args, size_t index) {
    return SpkHost_GetArg(args, index);
}


/*------------------------------------------------------------------------*/
/* misc. support routines */

SpkMethod *Spk_ThisMethod(SpkInterpreter *interpreter) {
    /* XXX: Make this work inside leaf methods. */
    return interpreter->activeContext->homeContext->u.m.method;
}


/*------------------------------------------------------------------------*/
/* class NativeAccessor */

static SpkMethodTmpl NativeAccessorMethods[] = {
    { 0, 0, 0}
};

SpkClassTmpl Spk_ClassNativeAccessorTmpl = {
    "NativeAccessor", {
        /*accessors*/ 0,
        NativeAccessorMethods,
        /*lvalueMethods*/ 0,
        offsetof(SpkNativeAccessorSubclass, variables),
        sizeof(SpkOpcode)
    }, /*meta*/ {
    }
};

static SpkUnknown *nativeReadAccessor(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    SpkNativeAccessor *accessor;
    char *addr;
    SpkUnknown *result;
    
    accessor = Spk_CAST(NativeAccessor, Spk_ThisMethod(theInterpreter));
    assert(accessor);
    
    addr = (char *)self + accessor->offset;
    
    switch (accessor->type) {
    case Spk_T_OBJECT:
        result = *(SpkUnknown **)addr;
        if (!result) {
            result = Spk_null;
        }
        Spk_INCREF(result);
        break;
        
    case Spk_T_SIZE:
        result = SpkHost_IntegerFromCLong(*(long *)addr);
        break;
        
    default:
        assert(0 && "XXX");
    }
    
    return result;
}

static SpkUnknown *nativeWriteAccessor(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    SpkNativeAccessor *accessor;
    char *addr;
    
    accessor = Spk_CAST(NativeAccessor, Spk_ThisMethod(theInterpreter));
    assert(accessor);
    
    addr = (char *)self + accessor->offset;
    
    switch (accessor->type) {
    case Spk_T_OBJECT:
        Spk_INCREF(arg0);
        Spk_DECREF(*(SpkUnknown **)addr);
        *(SpkUnknown **)addr = arg0;
        break;
        
    default:
        assert(0 && "XXX");
    }
    
    Spk_INCREF(Spk_void);
    return Spk_void;
}
