
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
        size += 6; /* leaf, arg, native */
    } else {
        size += 13; /* save, ... restore */
    }
    size += 2; /* restore, ret */
    
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
        *ip++ = Spk_OPCODE_ARG;
        *ip++ = (SpkOpcode)argumentCount;
        *ip++ = (SpkOpcode)0; /* localCount */
        *ip++ = (SpkOpcode)Spk_LEAF_MAX_STACK_SIZE; /*stackSize*/
        *ip++ = Spk_OPCODE_NATIVE;
    } else {
        size_t stackSize = 4;
        size_t contextSize =
            stackSize +
            argumentCount + variadic +
            0 /*localCount*/ ;
        *ip++ = Spk_OPCODE_SAVE;
        *ip++ = (SpkOpcode)contextSize;
        *ip++ = variadic ? Spk_OPCODE_ARG_VAR : Spk_OPCODE_ARG;
        *ip++ = (SpkOpcode)argumentCount;
        *ip++ = (SpkOpcode)0; /* localCount */
        *ip++ = (SpkOpcode)stackSize;
        *ip++ = Spk_OPCODE_NATIVE;
        
        /* skip trampoline code */
        *ip++ = Spk_OPCODE_BRANCH_ALWAYS;
        *ip++ = 6;
        
        /* trampolines for re-entering interpreted code */
        *ip++ = Spk_OPCODE_SEND_MESSAGE_VAR;
        *ip++ = Spk_OPCODE_RET_TRAMP;
        *ip++ = Spk_OPCODE_SEND_MESSAGE_SUPER_VAR;
        *ip++ = Spk_OPCODE_RET_TRAMP;
    }
    
    *ip++ = Spk_OPCODE_RESTORE_SENDER;
    *ip++ = Spk_OPCODE_RET;
    
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
        /* XXX: See the comment in SpkInterpreter_ThisMethod() */
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
                            SpkUnknown *obj,
                            unsigned int namespace,
                            SpkUnknown *selector,
                            SpkUnknown *argumentArray)
{
    return SpkInterpreter_SendMessage(interpreter, obj, namespace, selector, argumentArray);
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

static SpkUnknown *sendMessage(SpkInterpreter *interpreter,
                               SpkUnknown *obj, unsigned int namespace, SpkUnknown *selector, ...)
{
    SpkUnknown *result;
    va_list ap;
    
    va_start(ap, selector);
    result = vSendMessage(interpreter, obj, namespace, selector, ap);
    va_end(ap);
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
    return Spk_SendMessage(interpreter, obj, Spk_METHOD_NAMESPACE_RVALUE, name, Spk_emptyArgs);
}

SpkUnknown *Spk_SetAttr(SpkInterpreter *interpreter, SpkUnknown *obj, SpkUnknown *name, SpkUnknown *value) {
    return sendMessage(interpreter, obj, Spk_METHOD_NAMESPACE_LVALUE, name, value, 0);
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

SpkUnknown *Spk_CallAttrWithArguments(SpkInterpreter *interpreter, SpkUnknown *obj, SpkUnknown *name,
                                      SpkUnknown *argumentArray)
{
    SpkUnknown *result, *thunk;
    
    thunk = Spk_Attr(interpreter, obj, name);
    if (!thunk) {
        return 0;
    }
    result = Spk_SendMessage(interpreter,
                             thunk,
                             Spk_METHOD_NAMESPACE_RVALUE,
                             *Spk_operCallSelectors[Spk_OPER_APPLY].selector,
                             argumentArray);
    Spk_DECREF(thunk);
    return result;
}

SpkUnknown *Spk_Keyword(SpkInterpreter *interpreter, SpkUnknown *obj, SpkUnknown *selector, ...) {
    SpkUnknown *result;
    va_list ap;
    
    va_start(ap, selector);
    result = Spk_VKeyword(interpreter, obj, selector, ap);
    va_end(ap);
    return result;
}

SpkUnknown *Spk_VKeyword(SpkInterpreter *interpreter, SpkUnknown *obj, SpkUnknown *selector, va_list ap) {
    return vSendMessage(interpreter, obj, Spk_METHOD_NAMESPACE_RVALUE, selector, ap);
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
    
    accessor = Spk_CAST(NativeAccessor, SpkInterpreter_ThisMethod(theInterpreter));
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
    
    accessor = Spk_CAST(NativeAccessor, SpkInterpreter_ThisMethod(theInterpreter));
    assert(accessor);
    
    addr = (char *)self + accessor->offset;
    
    switch (accessor->type) {
    case Spk_T_OBJECT:
        Spk_INCREF(arg0);
        Spk_XDECREF(*(SpkUnknown **)addr);
        *(SpkUnknown **)addr = arg0;
        break;
        
    default:
        assert(0 && "XXX");
    }
    
    Spk_INCREF(Spk_void);
    return Spk_void;
}
