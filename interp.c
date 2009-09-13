
#include "interp.h"

#include "behavior.h"
#include "bool.h"
#ifdef MALTIPY
#include "bridge.h"
#endif
#include "class.h"
#include "host.h"
#include "module.h"
#include "native.h"
#include "rodata.h"

#ifdef MALTIPY
#include "compile.h"
#include "frameobject.h"
#include "traceback.h"
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*------------------------------------------------------------------------*/
/* class struct definitions */

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


struct SpkSemaphore {
    SpkObject base;
    SpkFiber *firstLink;
    SpkFiber *lastLink;
    int excessSignals;
};


struct SpkFiber {
    SpkObject base;
    SpkFiber *nextLink;
    SpkContext *suspendedContext;
    SpkContext *leafContext;
    int priority;
    SpkSemaphore *myList;
};


struct SpkProcessorScheduler {
    SpkObject base;
    SpkSemaphore **quiescentFiberLists;
    SpkFiber *activeFiber;
};


struct SpkThunk {
    SpkObject base;
    SpkUnknown *receiver;
    SpkMethod *method;
    struct SpkBehavior *methodClass;
    SpkOpcode *pc;
};


struct SpkInterpreter {
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

};


/*------------------------------------------------------------------------*/

/*** TODO ***/
#define XXX 0
#define HIGHEST_PRIORITY XXX


/* Context pseudo-fields */
#define sender caller


SpkUnknown *Spk_null;
SpkUnknown *Spk_uninit;
SpkUnknown *Spk_void;

SpkBehavior *Spk_ClassMessage, *Spk_ClassMethod, *Spk_ClassThunk, *Spk_ClassContext, *Spk_ClassMethodContext, *Spk_ClassBlockContext;
SpkBehavior *Spk_ClassInterpreter, *Spk_ClassProcessorScheduler, *Spk_ClassFiber;
SpkBehavior *Spk_ClassNull, *Spk_ClassUninit, *Spk_ClassVoid;
SpkInterpreter *theInterpreter; /* XXX */


/*------------------------------------------------------------------------*/
/* class templates */

typedef struct SpkMessageSubclass {
    SpkMessage base;
    SpkUnknown *variables[1]; /* stretchy */
} SpkMessageSubclass;

static SpkAccessorTmpl MessageAccessors[] = {
    { "selector",  Spk_T_OBJECT, offsetof(SpkMessage, selector),  SpkAccessor_READ },
    { "arguments", Spk_T_OBJECT, offsetof(SpkMessage, arguments), SpkAccessor_READ },
    { 0, 0, 0, 0 }
};

static SpkMethodTmpl MessageMethods[] = {
    { 0, 0, 0 }
};

SpkClassTmpl Spk_ClassMessageTmpl = {
    "Message", {
        MessageAccessors,
        MessageMethods,
        /*lvalueMethods*/ 0,
        offsetof(SpkMessageSubclass, variables)
    }, /*meta*/ {
    }
};


static void Method_zero(SpkObject *_self) {
    SpkMethod *self = (SpkMethod *)_self;
    (*Spk_ClassMethod->superclass->zero)(_self);
    self->nextInScope = 0;
    self->nestedClassList.first = 0;
    self->nestedClassList.last = 0;
    self->nestedMethodList.first = 0;
    self->nestedMethodList.last = 0;
    self->debug.source = 0;
    self->debug.lineCodeTally = 0;
    self->debug.lineCodes = 0;
    self->nativeCode = 0;
    return;
}

static void Method_dealloc(SpkObject *_self) {
    SpkMethod *self = (SpkMethod *)_self;
    if (self->debug.source) {
        /* XXX: lazy */
        Spk_DECREF(self->debug.source);
        self->debug.source = 0;
    }
    free(self->debug.lineCodes);
    self->debug.lineCodes = 0;
    (*Spk_ClassMethod->superclass->dealloc)(_self);
}

typedef struct SpkMethodSubclass {
    SpkMethod base;
    SpkUnknown *variables[1]; /* stretchy */
} SpkMethodSubclass;

static SpkMethodTmpl MethodMethods[] = {
    { 0, 0, 0 }
};

SpkClassTmpl Spk_ClassMethodTmpl = {
    "Method", {
        /*accessors*/ 0,
        MethodMethods,
        /*lvalueMethods*/ 0,
        offsetof(SpkMethodSubclass, variables),
        sizeof(SpkOpcode),
        &Method_zero,
        &Method_dealloc
    }, /*meta*/ {
    }
};


typedef struct SpkThunkSubclass {
    SpkThunk base;
    SpkUnknown *variables[1]; /* stretchy */
} SpkThunkSubclass;

static void Thunk_traverse_init(SpkObject *);
static SpkUnknown **Thunk_traverse_current(SpkObject *);
static void Thunk_traverse_next(SpkObject *);

static SpkTraverse Thunk_traverse = {
    &Thunk_traverse_init,
    &Thunk_traverse_current,
    &Thunk_traverse_next,
};

static SpkMethodTmpl ThunkMethods[] = {
    { 0, 0, 0 }
};

SpkClassTmpl Spk_ClassThunkTmpl = {
    "Thunk", {
        /*accessors*/ 0,
        ThunkMethods,
        /*lvalueMethods*/ 0,
        offsetof(SpkThunkSubclass, variables),
        /*itemSize*/ 0,
        /*zero*/ 0,
        /*dealloc*/ 0,
        &Thunk_traverse
    }, /*meta*/ {
    }
};


static void Context_traverse_init(SpkObject *);
static SpkUnknown **Context_traverse_current(SpkObject *);
static void Context_traverse_next(SpkObject *);
static void Context_dealloc(SpkObject *);
static SpkUnknown *Context_blockCopy(SpkUnknown *, SpkUnknown *, SpkUnknown *);
static SpkUnknown *Context_compoundExpression(SpkUnknown *, SpkUnknown *, SpkUnknown *);

typedef struct SpkContextSubclass {
    SpkContext base;
    SpkUnknown *variables[1]; /* stretchy */
} SpkContextSubclass;

static SpkMethodTmpl ContextMethods[] = {
    { "blockCopy", SpkNativeCode_METH_ATTR | SpkNativeCode_ARGS_2, &Context_blockCopy },
    { "compoundExpression", SpkNativeCode_METH_ATTR | SpkNativeCode_ARGS_ARRAY, &Context_compoundExpression },
    { 0, 0, 0 }
};

static SpkTraverse Context_traverse = {
    &Context_traverse_init,
    &Context_traverse_current,
    &Context_traverse_next,
};

SpkClassTmpl Spk_ClassContextTmpl = {
    "Context", {
        /*accessors*/ 0,
        ContextMethods,
        /*lvalueMethods*/ 0,
        offsetof(SpkContextSubclass, variables),
        sizeof(SpkUnknown *),
        /*zero*/ 0,
        &Context_dealloc,
        &Context_traverse
    }, /*meta*/ {
    }
};


static SpkMethodTmpl MethodContextMethods[] = {
    { 0, 0, 0 }
};

SpkClassTmpl Spk_ClassMethodContextTmpl = {
    "MethodContext", {
        /*accessors*/ 0,
        MethodContextMethods,
        /*lvalueMethods*/ 0,
        offsetof(SpkContextSubclass, variables),
        sizeof(SpkUnknown *)
    }, /*meta*/ {
    }
};


static SpkAccessorTmpl BlockContextAccessors[] = {
    { "numArgs", Spk_T_SIZE, offsetof(SpkContext, u.b.nargs), SpkAccessor_READ },
    { 0, 0, 0, 0 }
};

static SpkMethodTmpl BlockContextMethods[] = {
    { 0, 0, 0 }
};

SpkClassTmpl Spk_ClassBlockContextTmpl = {
    "BlockContext", {
        BlockContextAccessors,
        BlockContextMethods,
        /*lvalueMethods*/ 0,
        offsetof(SpkContextSubclass, variables),
        sizeof(SpkUnknown *)
    }, /*meta*/ {
    }
};


static SpkMethodTmpl NullMethods[] = {
    { 0, 0, 0 }
};

SpkClassTmpl Spk_ClassNullTmpl = {
    "Null", {
        /*accessors*/ 0,
        NullMethods,
        /*lvalueMethods*/ 0
    }, /*meta*/ {
    }
};


static SpkMethodTmpl UninitMethods[] = {
    { 0, 0, 0 }
};

SpkClassTmpl Spk_ClassUninitTmpl = {
    "Uninit", {
        /*accessors*/ 0,
        UninitMethods,
        /*lvalueMethods*/ 0
    }, /*meta*/ {
    }
};


static SpkMethodTmpl VoidMethods[] = {
    { 0, 0, 0 }
};

SpkClassTmpl Spk_ClassVoidTmpl = {
    "Void", {
        /*accessors*/ 0,
        VoidMethods,
        /*lvalueMethods*/ 0
    }, /*meta*/ {
    }
};


typedef struct SpkInterpreterSubclass {
    SpkInterpreter base;
    SpkUnknown *variables[1]; /* stretchy */
} SpkInterpreterSubclass;

static SpkMethodTmpl InterpreterMethods[] = {
    { 0, 0, 0 }
};

SpkClassTmpl Spk_ClassInterpreterTmpl = {
    "Interpreter", {
        /*accessors*/ 0,
        InterpreterMethods,
        /*lvalueMethods*/ 0,
        offsetof(SpkInterpreterSubclass, variables)
    }, /*meta*/ {
    }
};


typedef struct SpkProcessorSchedulerSubclass {
    SpkProcessorScheduler base;
    SpkUnknown *variables[1]; /* stretchy */
} SpkProcessorSchedulerSubclass;

static SpkMethodTmpl ProcessorSchedulerMethods[] = {
    { 0, 0, 0 }
};

SpkClassTmpl Spk_ClassProcessorSchedulerTmpl = {
    "ProcessorScheduler", {
        /*accessors*/ 0,
        ProcessorSchedulerMethods,
        /*lvalueMethods*/ 0,
        offsetof(SpkProcessorSchedulerSubclass, variables)
    }, /*meta*/ {
    }
};


typedef struct SpkFiberSubclass {
    SpkFiber base;
    SpkUnknown *variables[1]; /* stretchy */
} SpkFiberSubclass;

static SpkMethodTmpl FiberMethods[] = {
    { 0, 0, 0 }
};

SpkClassTmpl Spk_ClassFiberTmpl = {
    "Fiber", {
        /*accessors*/ 0,
        FiberMethods,
        /*lvalueMethods*/ 0,
        offsetof(SpkFiberSubclass, variables)
    }, /*meta*/ {
    }
};


/*------------------------------------------------------------------------*/
/* initialization */

void SpkInterpreter_Boot(void) {
    SpkMethod *callThunk, *callBlock;
    
    callThunk = SpkMethod_New(1);
    SpkMethod_OPCODES(callThunk)[0] = Spk_OPCODE_CALL_THUNK;
    SpkBehavior_InsertMethod(Spk_ClassThunk, Spk_METHOD_NAMESPACE_RVALUE, Spk___apply__, callThunk);
    Spk_DECREF(callThunk);
    
    callBlock = SpkMethod_New(1);
    SpkMethod_OPCODES(callBlock)[0] = Spk_OPCODE_CALL_BLOCK;
    SpkBehavior_InsertMethod(Spk_ClassBlockContext, Spk_METHOD_NAMESPACE_RVALUE, Spk___apply__, callBlock);
    Spk_DECREF(callBlock);
}

SpkInterpreter *SpkInterpreter_New(void) {
    size_t contextSize;
    SpkContext *leafContext;
    SpkFiber *fiber;
    SpkProcessorScheduler *scheduler;
    SpkInterpreter *newInterpreter;
    
    contextSize = Spk_LEAF_MAX_STACK_SIZE +
                  Spk_LEAF_MAX_ARGUMENT_COUNT;
    ++contextSize; /* XXX: doesNotUnderstand overhead */
    leafContext = SpkContext_New(contextSize);
    leafContext->pc = 0;
    leafContext->stackp = 0;
    leafContext->homeContext = 0;
    leafContext->u.m.method = 0;
    leafContext->u.m.methodClass = 0;
    leafContext->u.m.receiver = 0;
    leafContext->u.m.framep = 0;
    
    fiber =(SpkFiber *)SpkObject_New(Spk_ClassFiber);
    fiber->nextLink = 0;
    fiber->suspendedContext = 0;
    fiber->leafContext = leafContext; /* steal reference */
    fiber->priority = 0;
    fiber->myList = 0;
    
    scheduler =(SpkProcessorScheduler *)SpkObject_New(Spk_ClassProcessorScheduler);
    scheduler->quiescentFiberLists = 0;
    scheduler->activeFiber = fiber; /* steal reference */
    
    newInterpreter = (SpkInterpreter *)SpkObject_New(Spk_ClassInterpreter);
    SpkInterpreter_Init(newInterpreter, scheduler);
    Spk_DECREF(scheduler);
    
    return newInterpreter;
}

void SpkInterpreter_Init(SpkInterpreter *self, SpkProcessorScheduler *aScheduler) {
    /* fibers */
    self->scheduler = aScheduler;  Spk_INCREF(aScheduler);
    self->theInterruptSemaphore = SpkSemaphore_New();
    self->interruptPending = 0;

    /* contexts */
    self->activeContext = self->scheduler->activeFiber->suspendedContext;
    Spk_XINCREF(self->activeContext);
    self->newContext = 0;

    /* error handling */
    self->printingStack = 0;
}

SpkMessage *SpkMessage_New() {
    return (SpkMessage *)SpkObject_New(Spk_ClassMessage);
}

SpkMethod *SpkMethod_New(size_t size) {
    return (SpkMethod *)SpkObject_NewVar(Spk_ClassMethod, size);
}


/*------------------------------------------------------------------------*/
/* thunks */

SpkThunk *SpkThunk_New(
    SpkUnknown *receiver,
    SpkMethod *method,
    SpkBehavior *methodClass)
{
    SpkThunk *newThunk;
    
    newThunk = (SpkThunk *)SpkObject_New(Spk_ClassThunk);
    if (!newThunk)
        return 0;
    newThunk->receiver = receiver;  Spk_INCREF(receiver);
    newThunk->method = method;  Spk_INCREF(method);
    newThunk->methodClass = methodClass;  Spk_INCREF(methodClass);
    /* skip 'thunk' opcode */
    newThunk->pc = SpkMethod_OPCODES(method) + 1;
    return newThunk;
}

static void Thunk_traverse_init(SpkObject *_self) {
    SpkThunk *self;
    
    self = (SpkThunk *)_self;
    (*Spk_ClassThunk->superclass->traverse.init)(_self);
    self->pc = (SpkOpcode *)0 + 3;
}

static SpkUnknown **Thunk_traverse_current(SpkObject *_self) {
    SpkThunk *self;
    ptrdiff_t i;
    
    self = (SpkThunk *)_self;
    i = self->pc - (SpkOpcode *)0;
    switch (i) {
    case 3: return &self->receiver;
    case 2: return (SpkUnknown **)&self->method;
    case 1: return (SpkUnknown **)&self->methodClass;
    }
    return (*Spk_ClassThunk->superclass->traverse.current)(_self);
}

static void Thunk_traverse_next(SpkObject *_self) {
    SpkThunk *self;
    
    self = (SpkThunk *)_self;
    if (self->pc)
        --self->pc;
    else
        (*Spk_ClassThunk->superclass->traverse.next)(_self);
}


/*------------------------------------------------------------------------*/
/* contexts */

SpkContext *SpkContext_New(size_t size) {
    SpkContext *newContext;
    SpkUnknown **p;
    size_t count;
    
    newContext = (SpkContext *)SpkObject_NewVar(Spk_ClassMethodContext, size);
    if (!newContext) {
        return 0;
    }
    
    for (count = 0, p = SpkContext_VARIABLES(newContext); count < size; ++count, ++p) {
        Spk_INCREF(Spk_uninit);
        *p = Spk_uninit;
    }
    
    newContext->mark = 0;

    return newContext;
}

static void Context_traverse_init(SpkObject *self) {
    (*Spk_ClassContext->superclass->traverse.init)(self);
}

static SpkUnknown **Context_traverse_current(SpkObject *_self) {
    SpkContext *self;
    
    self = (SpkContext *)_self;
    if (self->base.size > 0)
        return &(SpkContext_VARIABLES(self)[self->base.size - 1]);
    return (*Spk_ClassContext->superclass->traverse.current)(_self);
}

static void Context_traverse_next(SpkObject *_self) {
    SpkContext *self;
    
    self = (SpkContext *)_self;
    if (self->base.size > 0)
        --self->base.size;
    else
        (*Spk_ClassContext->superclass->traverse.next)(_self);
}

static void Context_dealloc(SpkObject *_self) {
    SpkContext *self;
    
    self = (SpkContext *)_self;
    
    /* XXX: This cleans up extra stuff missed by the 'traverse'
       routines. */
    
    Spk_XDECREF(self->caller);
    if (self->homeContext) { /* XXX: shady */
        /* block context */
        Spk_DECREF(self->homeContext);
    } else {
        /* method context */
        Spk_DECREF(self->u.m.method);
        Spk_DECREF(self->u.m.methodClass);
        Spk_DECREF(self->u.m.receiver);
    }
    
    (*Spk_ClassContext->superclass->dealloc)(_self);
}

static SpkUnknown *Context_blockCopy(SpkUnknown *_self, SpkUnknown *arg0, SpkUnknown *arg1) {
    SpkContext *self;
    size_t index, numArgs;
    size_t size;
    SpkContext *newContext;
    
    self = (SpkContext *)_self;
    if (!SpkHost_IsInteger(arg0) || !SpkHost_IsInteger(arg1)) {
        Spk_Halt(Spk_HALT_TYPE_ERROR, "an integer object is required");
        return 0;
    }
    index = (size_t)SpkHost_IntegerAsCLong(arg0);
    numArgs = (size_t)SpkHost_IntegerAsCLong(arg1);
    
    /* The compiler guarantees that the home context is at least as
       big as the biggest child block context needs to be. */
    size = self->homeContext->base.size;
    
    newContext = SpkContext_New(size);
    Spk_INCREF(Spk_ClassBlockContext);
    Spk_DECREF(newContext->base.base.klass);
    newContext->base.base.klass = Spk_ClassBlockContext;
    newContext->caller = 0;
    newContext->stackp = &SpkContext_VARIABLES(newContext)[newContext->base.size];
    newContext->homeContext = self->homeContext;  Spk_INCREF(self->homeContext);
    newContext->u.b.index = index;
    newContext->u.b.nargs = numArgs;
    newContext->u.b.startpc = newContext->pc = self->pc + 3; /* skip branch */
    
    return (SpkUnknown *)newContext;
}

static SpkUnknown *Context_compoundExpression(SpkUnknown *_self, SpkUnknown *arg0, SpkUnknown *arg1) {
    SpkContext *self = (SpkContext *)_self;
    return Spk_CallAttrWithArguments(theInterpreter,
                                     self->homeContext->u.m.receiver,
                                     Spk_compoundExpression,
                                     arg0);
}


/*------------------------------------------------------------------------*/
/* utils */

#ifdef MALTIPY
static void addTraceback(SpkContext *ctxt) {
    /* derived from __Pyx_AddTraceback */
    SpkContext *home;
    SpkMethod *method;
    SpkBehavior *methodClass;
    PyObject *methodSel;
    PyObject *savedType, *savedValue, *savedTraceback;
    
    PyErr_Fetch(&savedType, &savedValue, &savedTraceback);
    
    int lineno = 0; /* XXX */
    
    PyObject *py_srcfile = 0;
    PyObject *py_funcname = 0;
    PyObject *py_globals = 0;
    PyObject *empty_tuple = 0;
    PyObject *empty_string = 0;
    PyCodeObject *py_code = 0;
    PyFrameObject *py_frame = 0;
    
    home = ctxt->homeContext;
    method = home->u.m.method;
    methodClass = home->u.m.methodClass;
    
    methodSel = SpkBehavior_FindSelectorOfMethod(methodClass, method);
    if (!methodSel) {
        methodSel = Spk_unknownSelector;
        Spk_INCREF(methodSel);
    } else if (!SpkHost_IsSymbol(methodSel)) {
        PyObject *tmp = PyObject_Str(methodSel);
        Spk_DECREF(methodSel);
        methodSel = tmp;
    }
    
    py_srcfile = Spk_unknownSourcePathname;
    Spk_INCREF(py_srcfile);
    py_funcname = methodSel;
    if (!py_funcname) goto bad;
#if XXX
    py_globals = PyModule_GetDict(__pyx_m);
#else
    py_globals = PyDict_New();
#endif
    if (!py_globals) goto bad;
    empty_tuple = Spk_emptyArgs;
    Spk_INCREF(empty_tuple);
    empty_string = Spk_emptyString;
    Spk_INCREF(empty_string);
    py_code = PyCode_New(
        0,            /*int argcount,*/
        0,            /*int nlocals,*/
        0,            /*int stacksize,*/
        0,            /*int flags,*/
        empty_string, /*PyObject *code,*/
        empty_tuple,  /*PyObject *consts,*/
        empty_tuple,  /*PyObject *names,*/
        empty_tuple,  /*PyObject *varnames,*/
        empty_tuple,  /*PyObject *freevars,*/
        empty_tuple,  /*PyObject *cellvars,*/
        py_srcfile,   /*PyObject *filename,*/
        py_funcname,  /*PyObject *name,*/
        lineno,       /*int firstlineno,*/
        empty_string  /*PyObject *lnotab*/
        );
    if (!py_code) goto bad;
    py_frame = PyFrame_New(
        PyThreadState_Get(), /*PyThreadState *tstate,*/
        py_code,             /*PyCodeObject *code,*/
        py_globals,          /*PyObject *globals,*/
        0                    /*PyObject *locals*/
        );
    if (!py_frame) goto bad;
    py_frame->f_lineno = lineno;
    PyTraceBack_Here(py_frame);
 bad:
    Py_XDECREF(py_srcfile);
    Py_XDECREF(py_funcname);
    Py_XDECREF(empty_tuple);
    Py_XDECREF(empty_string);
    Py_XDECREF(py_code);
    Py_XDECREF(py_frame);
    
    PyErr_Restore(savedType, savedValue, savedTraceback);
}
#endif /* MALTIPY */


/*------------------------------------------------------------------------*/
/* interpreter loop */

/* stackPointer */
static SpkUnknown *popObject(SpkUnknown **stackPointer) {
    SpkUnknown *tmp = stackPointer[-1];
    Spk_INCREF(Spk_uninit);
    stackPointer[-1] = Spk_uninit;
    return tmp;
}

#define POP_OBJECT() (popObject(++stackPointer))

#define POP(nItems) \
do { SpkUnknown **end = stackPointer + (nItems); \
     while (stackPointer < end) { \
         Spk_INCREF(Spk_uninit); \
         *stackPointer++ = Spk_uninit; } } while (0)

#define CLEAN(nItems) \
do { SpkUnknown **end = stackPointer + (nItems); \
     while (stackPointer < end) { \
         SpkUnknown *op = *stackPointer; \
         Spk_INCREF(Spk_uninit); \
         *stackPointer++ = Spk_uninit; \
         Spk_DECREF(op); } } while (0)

#define PUSH(object) \
do { --stackPointer; \
     Spk_DECREF(stackPointer[0]); \
     stackPointer[0] = (SpkUnknown *)(object); } while (0)

#define STACK_TOP() (*stackPointer)

#define INSTANCE_VARS(op, mc) ((SpkUnknown **)((char *)(op) + GET_CLASS(op)->instVarOffset) + (mc)->instVarBaseIndex)


#define DECODE_UINT(result) do { \
    SpkOpcode byte; \
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
    SpkOpcode byte; \
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

#ifndef MALTIPY
/* XXX: What about leaf methods? */
#define TRAP(selector, argument) \
    self->activeContext->pc = instructionPointer; \
    self->activeContext->stackp = stackPointer; \
    trap(self, selector, argument); \
    return 0
#else
/* for now */
#define TRAP(selector, argument) \
    self->activeContext->pc = instructionPointer; \
    self->activeContext->stackp = stackPointer; \
    runtimeError(self, selector, argument); \
    goto exception
#endif


/* Python interoperability */
#ifdef MALTIPY
#define GET_CLASS(op) (PyObject_TypeCheck((op), &SpkSpikeObject_Type) ? ((SpkObject *)(op))->klass : Spk_ClassPythonObject)
#else
#define GET_CLASS(op) (((SpkObject *)(op))->klass)
#endif


static SpkFiber *trap(SpkInterpreter *, SpkUnknown *, SpkUnknown *);
#ifdef MALTIPY
static void runtimeError(SpkInterpreter *, SpkUnknown *, SpkUnknown *);
#endif
static void unknownOpcode(SpkInterpreter *);
static void halt(SpkInterpreter *, SpkUnknown *, SpkUnknown *);


SpkUnknown *SpkInterpreter_Interpret(SpkInterpreter *self) {

    /* context registers */
    SpkContext *homeContext, *leafContext;
    SpkUnknown *receiver;
    SpkMethod *method;
    SpkBehavior *methodClass, *mc;
    register SpkOpcode *instructionPointer;
    register SpkUnknown **stackPointer;
    register SpkUnknown **framePointer;
    register SpkUnknown **instVarPointer;
    register SpkUnknown **globalPointer;
    register SpkUnknown **literalPointer;

    size_t index;
    SpkUnknown *tmp;

    /* message sending */
    SpkMethodNamespace namespace;
    SpkUnknown *messageSelector = 0; /* ref counted */
    size_t argumentCount = 0, varArg = 0, variadic = 0, fixedArgumentCount = 0;
    unsigned int operator;
    SpkOpcode *oldIP;
    
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
    globalPointer = SpkModule_VARIABLES(methodClass->module);
    literalPointer = SpkModule_LITERALS(methodClass->module);
    assert(self->newContext == 0);
    
    leafContext = self->scheduler->activeFiber->leafContext;
    
 checkForInterrupts:
    if (self->interruptPending) {
        self->interruptPending = 0;
        SpkInterpreter_SynchronousSignal(self, self->theInterruptSemaphore);
        if (self->newContext) {
            Spk_DECREF(self->activeContext);
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
            unknownOpcode(self);
            break;
            
        case Spk_OPCODE_NOP:
            break;

            /*** push opcodes ***/
        case Spk_OPCODE_PUSH_LOCAL:
            DECODE_UINT(index);
            tmp = framePointer[index];
            Spk_INCREF(tmp);
            PUSH(tmp);
            break;
        case Spk_OPCODE_PUSH_INST_VAR:
            DECODE_UINT(index);
            tmp = instVarPointer[index];
            Spk_INCREF(tmp);
            PUSH(tmp);
            break;
        case Spk_OPCODE_PUSH_GLOBAL:
            DECODE_UINT(index);
            tmp = globalPointer[index];
            Spk_INCREF(tmp);
            PUSH(tmp);
            break;
        case Spk_OPCODE_PUSH_LITERAL:
            DECODE_UINT(index);
            tmp = literalPointer[index];
            Spk_INCREF(tmp);
            PUSH(tmp);
            break;
        case Spk_OPCODE_PUSH_SUPER:
            /* Spk_OPCODE_PUSH_SUPER is a pseudo-op equivalent to
             * Spk_OPCODE_PUSH_SELF.  The receiver is always pushed onto
             * the stack so that the stack clean-up machinery doesn't
             * have to distinguish between sends and super-sends.
             */
        case Spk_OPCODE_PUSH_SELF:     PUSH(receiver);             Spk_INCREF(receiver);             break;
        case Spk_OPCODE_PUSH_FALSE:    PUSH(Spk_false);            Spk_INCREF(Spk_false);            break;
        case Spk_OPCODE_PUSH_TRUE:     PUSH(Spk_true);             Spk_INCREF(Spk_true);             break;
        case Spk_OPCODE_PUSH_NULL:     PUSH(Spk_null);             Spk_INCREF(Spk_null);             break;
        case Spk_OPCODE_PUSH_VOID:     PUSH(Spk_void);             Spk_INCREF(Spk_void);             break;
        case Spk_OPCODE_PUSH_CONTEXT:  PUSH(self->activeContext);  Spk_INCREF(self->activeContext);  break;
                
            /*** store opcodes ***/
        case Spk_OPCODE_STORE_LOCAL:
            DECODE_UINT(index);
            tmp = STACK_TOP();
            Spk_INCREF(tmp);
            Spk_DECREF(framePointer[index]);
            framePointer[index] = tmp;
            break;
        case Spk_OPCODE_STORE_INST_VAR:
            DECODE_UINT(index);
            tmp = STACK_TOP();
            Spk_INCREF(tmp);
            Spk_DECREF(instVarPointer[index]);
            instVarPointer[index] = tmp;
            break;
        case Spk_OPCODE_STORE_GLOBAL:
            DECODE_UINT(index);
            tmp = STACK_TOP();
            Spk_INCREF(tmp);
            Spk_DECREF(globalPointer[index]);
            globalPointer[index] = tmp;
            break;
            
            /*** additional stack opcodes ***/
        case Spk_OPCODE_DUP:
            tmp = STACK_TOP();
            Spk_INCREF(tmp);
            PUSH(tmp);
            break;
        case Spk_OPCODE_DUP_N: {
            SpkUnknown **s;
            size_t n;
            DECODE_UINT(n);
            s = stackPointer + n;
            while (n--) {
                tmp = *--s;
                Spk_INCREF(tmp);
                *--stackPointer = tmp;
            }
            break; }
        case Spk_OPCODE_POP:
            Spk_DECREF(STACK_TOP());
            POP(1);
            break;
        case Spk_OPCODE_ROT: {
            SpkUnknown **s, **end;
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
        case Spk_OPCODE_BRANCH_IF_FALSE: {
            ptrdiff_t displacement;
            SpkUnknown *x, *o, *boolean;
            x = Spk_false;
            o = Spk_true;
            goto branch;
        case Spk_OPCODE_BRANCH_IF_TRUE:
            x = Spk_true;
            o = Spk_false;
 branch:
            boolean = POP_OBJECT();
            if (boolean == x) {
                SpkOpcode *base;
                Spk_DECREF(boolean);
            case Spk_OPCODE_BRANCH_ALWAYS:
                base = instructionPointer - 1;
                DECODE_SINT(displacement);
                instructionPointer = base + displacement;
                if (displacement < 0) {
                    goto checkForInterrupts;
                }
            } else if (boolean != o) {
                --instructionPointer;
                TRAP(Spk_mustBeBoolean, 0);
            } else {
                Spk_DECREF(boolean);
                DECODE_SINT(displacement);
            }
            break; }
            
            /* identity comparison opcode */
        case Spk_OPCODE_ID: {
            SpkUnknown *x, *y, *boolean;
            x = POP_OBJECT();
            y = POP_OBJECT();
            boolean = x == y ? Spk_true : Spk_false;
            PUSH(boolean);
            Spk_INCREF(boolean);
            Spk_DECREF(x);
            Spk_DECREF(y);
            break; }

            /*** send opcodes -- operators ***/
        case Spk_OPCODE_OPER:
            operator = (unsigned int)(*instructionPointer++);
            argumentCount = Spk_operSelectors[operator].argumentCount;
            varArg = 0;
            receiver = stackPointer[argumentCount];
            methodClass = GET_CLASS(receiver);
            goto oper;
        case Spk_OPCODE_OPER_SUPER:
            operator = (unsigned int)(*instructionPointer++);
            argumentCount = Spk_operSelectors[operator].argumentCount;
            varArg = 0;
            methodClass = methodClass->superclass;
 oper:
            for (mc = methodClass; mc; mc = mc->superclass) {
                method = mc->ns[Spk_METHOD_NAMESPACE_RVALUE].operTable[operator];
                if (method) {
                    methodClass = mc;
                    goto callNewMethod;
                }
            }
            oldIP = instructionPointer - 2;
            namespace = Spk_METHOD_NAMESPACE_RVALUE;
            messageSelector = *Spk_operSelectors[operator].selector;
            Spk_INCREF(messageSelector);
            goto createActualMessage;
        case Spk_OPCODE_CALL:
            oldIP = instructionPointer - 1;
            namespace = (unsigned int)(*instructionPointer++);
            operator = (unsigned int)(*instructionPointer++);
            DECODE_UINT(argumentCount);
            varArg = 0;
            receiver = stackPointer[argumentCount];
            methodClass = GET_CLASS(receiver);
            goto call;
        case Spk_OPCODE_CALL_VAR:
            oldIP = instructionPointer - 1;
            namespace = (unsigned int)(*instructionPointer++);
            operator = (unsigned int)(*instructionPointer++);
            DECODE_UINT(argumentCount);
            varArg = 1;
            receiver = stackPointer[argumentCount + 1];
            methodClass = GET_CLASS(receiver);
            goto call;
        case Spk_OPCODE_CALL_SUPER:
            oldIP = instructionPointer - 1;
            namespace = (unsigned int)(*instructionPointer++);
            operator = (unsigned int)(*instructionPointer++);
            DECODE_UINT(argumentCount);
            varArg = 0;
            methodClass = methodClass->superclass;
            goto call;
        case Spk_OPCODE_CALL_SUPER_VAR:
            oldIP = instructionPointer - 1;
            operator = (unsigned int)(*instructionPointer++);
            namespace = (unsigned int)(*instructionPointer++);
            DECODE_UINT(argumentCount);
            varArg = 1;
            methodClass = methodClass->superclass;
 call:
            for (mc = methodClass; mc; mc = mc->superclass) {
                method = methodClass->ns[namespace].operCallTable[operator];
                if (method) {
                    methodClass = mc;
                    goto callNewMethod;
                }
            }
            messageSelector = *Spk_operCallSelectors[operator].selector;
            Spk_INCREF(messageSelector);
            goto createActualMessage;

            /*** send opcodes -- "obj.attr" ***/
        case Spk_OPCODE_SET_ATTR:
            namespace = Spk_METHOD_NAMESPACE_LVALUE;
            argumentCount = 1;
            varArg = 0;
            goto attr;
        case Spk_OPCODE_GET_ATTR:
            namespace = Spk_METHOD_NAMESPACE_RVALUE;
            argumentCount = varArg = 0;
 attr:
            oldIP = instructionPointer - 1;
            receiver = stackPointer[argumentCount];
            methodClass = GET_CLASS(receiver);
            DECODE_UINT(index);
            messageSelector = literalPointer[index];
            Spk_INCREF(messageSelector);
            goto lookupMethodInClass;
        case Spk_OPCODE_SET_ATTR_SUPER:
            namespace = Spk_METHOD_NAMESPACE_LVALUE;
            argumentCount = 1;
            varArg = 0;
            goto superAttr;
        case Spk_OPCODE_GET_ATTR_SUPER:
            namespace = Spk_METHOD_NAMESPACE_RVALUE;
            argumentCount = varArg = 0;
 superAttr:
            oldIP = instructionPointer - 1;
            methodClass = methodClass->superclass;
            DECODE_UINT(index);
            messageSelector = literalPointer[index];
            Spk_INCREF(messageSelector);
 lookupMethodInClass:
            for (mc = methodClass; mc; mc = mc->superclass) {
                method = SpkBehavior_LookupMethod(mc, namespace, messageSelector);
                if (method) {
                    Spk_DECREF(method);
                    methodClass = mc;
                    Spk_DECREF(messageSelector);
                    messageSelector = 0;
 callNewMethod:
                    /* call */
                    self->activeContext->pc = instructionPointer;
                    instructionPointer = SpkMethod_OPCODES(method);
 jump:
                    instVarPointer = INSTANCE_VARS(receiver, methodClass);
                    globalPointer = SpkModule_VARIABLES(methodClass->module);
                    literalPointer = SpkModule_LITERALS(methodClass->module);
                    goto loop;
                }
            }
            
            if (messageSelector == Spk_doesNotUnderstand) {
                /* recursive doesNotUnderstand: */
                instructionPointer = oldIP;
                TRAP(Spk_recursiveDoesNotUnderstand, STACK_TOP());
            }
            
 createActualMessage:
            do {
                SpkMessage *message;
                SpkUnknown *varArgTuple;
                
                if (varArg) {
                    varArgTuple = stackPointer[0];
                    if (!Spk_IsArgs(varArgTuple)) {
                        TRAP(Spk_mustBeTuple, 0);
                    }
                } else {
                    varArgTuple = 0;
                }
                message = SpkMessage_New();
                message->namespace = namespace;
                message->selector = messageSelector; /* steal reference */
                message->arguments
                    = SpkHost_GetArgs(
                        stackPointer + varArg, argumentCount,
                        varArgTuple, 0
                        );
                
                CLEAN(varArg + argumentCount);
                PUSH(message);  /* XXX: doesNotUnderstand overhead  -- see SpkContext_new() */
                argumentCount = 1;
                varArg = 0;
                namespace = Spk_METHOD_NAMESPACE_RVALUE;
            } while (0);
            
            messageSelector = Spk_doesNotUnderstand;
            Spk_INCREF(messageSelector);
            goto lookupMethodInClass;
            
            /*** send opcodes -- "obj.*attr" ***/
        case Spk_OPCODE_SET_ATTR_VAR:
            namespace = Spk_METHOD_NAMESPACE_LVALUE;
            argumentCount = 1;
            varArg = 0;
            goto attrVar;
        case Spk_OPCODE_GET_ATTR_VAR:
            namespace = Spk_METHOD_NAMESPACE_RVALUE;
            argumentCount = varArg = 0;
 attrVar:
            receiver = stackPointer[argumentCount + 1];
            methodClass = GET_CLASS(receiver);
 perform:
            messageSelector = stackPointer[argumentCount]; /* steal reference */
            if (0 /*!SpkHost_IsSelector(messageSelector)*/ ) {
                --instructionPointer;
                TRAP(Spk_mustBeSymbol, 0);
            }
            if (argumentCount == 1) {
                stackPointer[1] = stackPointer[0];
            }
            POP(1);
            oldIP = instructionPointer - 1;
            goto lookupMethodInClass;
        case Spk_OPCODE_SET_ATTR_VAR_SUPER:
            namespace = Spk_METHOD_NAMESPACE_LVALUE;
            argumentCount = 1;
            varArg = 0;
            goto superSetAttr;
        case Spk_OPCODE_GET_ATTR_VAR_SUPER:
            namespace = Spk_METHOD_NAMESPACE_RVALUE;
            argumentCount = varArg = 0;
 superSetAttr:
            methodClass = methodClass->superclass;
            goto perform;
            
            /*** send opcodes -- keyword expressions ***/
        case Spk_OPCODE_SEND_MESSAGE:
            oldIP = instructionPointer - 1;
            DECODE_UINT(index);
            DECODE_UINT(argumentCount);
            receiver = stackPointer[argumentCount];
            methodClass = GET_CLASS(receiver);
 send:
            namespace = Spk_METHOD_NAMESPACE_RVALUE;
            messageSelector = literalPointer[index];
            Spk_INCREF(messageSelector);
            varArg = 0;
            goto lookupMethodInClass;
        case Spk_OPCODE_SEND_MESSAGE_SUPER:
            oldIP = instructionPointer - 1;
            DECODE_UINT(index);
            DECODE_UINT(argumentCount);
            methodClass = methodClass->superclass;
            goto send;
            
            /*** send opcodes -- generic ***/
        case Spk_OPCODE_SEND_MESSAGE_VAR: {
            SpkUnknown *namespaceObj;
            receiver = stackPointer[3];
            methodClass = GET_CLASS(receiver);
 sendVar:
            namespaceObj = stackPointer[2];
            if (!SpkHost_IsInteger(namespaceObj)) {
                assert(XXX); /* trap */
            }
            namespace = SpkHost_IntegerAsCLong(namespaceObj);
            assert(0 <= namespace && namespace < Spk_NUM_METHOD_NAMESPACES);
            messageSelector = stackPointer[1]; /* steal reference */
            if (0 /*!SpkHost_IsSelector(messageSelector)*/ ) {
                --instructionPointer;
                TRAP(Spk_mustBeSymbol, 0);
            }
            stackPointer[2] = stackPointer[0];
            POP(2);
            Spk_DECREF(namespaceObj);
            oldIP = instructionPointer - 1;
            argumentCount = 0;
            varArg = 1;
            goto lookupMethodInClass; }
        case Spk_OPCODE_SEND_MESSAGE_SUPER_VAR:
            methodClass = methodClass->superclass;
            goto sendVar;
            
#ifdef MALTIPY
        case Spk_OPCODE_RAISE: {
            PyObject *type, *value;
            value = POP_OBJECT();
            if (PyInstance_Check(value)) {
                type = (PyObject*)((PyInstanceObject*)value)->in_class;
            } else {
                type = (PyObject *)value->ob_type;
            }
            PyErr_SetObject(type, value);
            Spk_DECREF(value);
            goto exception; }
#endif /* MALTIPY */

            /*** save/restore/return opcodes ***/
        case Spk_OPCODE_RET:
 ret:
            instructionPointer = self->activeContext->pc;
            receiver = homeContext->u.m.receiver;
            method = homeContext->u.m.method;
            methodClass = homeContext->u.m.methodClass;
            instVarPointer = INSTANCE_VARS(receiver, methodClass);
            globalPointer = SpkModule_VARIABLES(methodClass->module);
            literalPointer = SpkModule_LITERALS(methodClass->module);
            if (self->activeContext->mark != &mark) {
                /* suspend */
                self->activeContext->pc = instructionPointer;
                self->activeContext->stackp = stackPointer;
                return 0; /* unwind */
            }
            break;
        case Spk_OPCODE_RET_TRAMP: {
            /* return from trampoline */
            SpkUnknown *result = POP_OBJECT();
            self->activeContext->pc = instructionPointer;
            self->activeContext->stackp = stackPointer;
            return result; }
            
        case Spk_OPCODE_LEAF:
            leafContext->sender = self->activeContext;  Spk_INCREF(self->activeContext);
            leafContext->homeContext = leafContext;     Spk_INCREF(leafContext); /* note cycle */
            
            Spk_XDECREF(leafContext->u.m.method);
            Spk_INCREF(method);
            leafContext->u.m.method = method;
            
            Spk_XDECREF(leafContext->u.m.methodClass);
            Spk_INCREF(methodClass);
            leafContext->u.m.methodClass = methodClass;
            
            /* Sometimes this is the only reference to the receiver. */
            Spk_XDECREF(leafContext->u.m.receiver);
            Spk_INCREF(receiver);
            leafContext->u.m.receiver = receiver;
            
            leafContext->mark = &mark;
            
            Spk_INCREF(leafContext);
            Spk_DECREF(self->activeContext);
            self->activeContext = homeContext = leafContext;
            break;
            
        case Spk_OPCODE_SAVE: {
            size_t contextSize;
            SpkContext *newContext;
            
            /* Create a new context for the currently
             * executing method (cf. activateNewMethod).
             */
            DECODE_UINT(contextSize);
            
            ++contextSize; /* XXX: doesNotUnderstand overhead */
            
            newContext = SpkContext_New(contextSize);
            
            newContext->sender = self->activeContext;   Spk_INCREF(self->activeContext);
            newContext->pc = instructionPointer;
            newContext->stackp = 0;
            newContext->homeContext = newContext;       Spk_INCREF(newContext); /* note cycle */
            newContext->u.m.method = method;            Spk_INCREF(method);
            newContext->u.m.methodClass = methodClass;  Spk_INCREF(methodClass);
            newContext->u.m.receiver = receiver;        Spk_INCREF(receiver);
            newContext->u.m.framep = 0;
            newContext->mark = &mark;
            
            Spk_DECREF(self->activeContext);
            self->activeContext = homeContext = newContext;
            break; }
            
        case Spk_OPCODE_ARG_VAR:
            variadic = 1;
            goto args;
        case Spk_OPCODE_ARG: {
            size_t localCount, stackSize;
            size_t i, varArgTupleSize;
            size_t excessStackArgCount, consumedArrayArgCount;
            SpkContext *newContext;
            SpkUnknown *varArgTuple;
            SpkUnknown **p;
            
            variadic = 0;
 args:
            DECODE_UINT(fixedArgumentCount);
            DECODE_UINT(localCount);
            DECODE_UINT(stackSize);
            
            ++stackSize; /* XXX: doesNotUnderstand overhead */
            
            newContext = self->activeContext;
            
            p = SpkContext_VARIABLES(newContext) + stackSize;
            newContext->stackp = newContext->u.m.framep = p;
            
            /* process arguments */
            if (varArg) {
                varArgTuple = stackPointer[0];
                if (!Spk_IsArgs(varArgTuple)) {
                    TRAP(Spk_mustBeTuple, 0);
                }
                varArgTupleSize = Spk_ArgsSize(varArgTuple);
            } else {
                varArgTuple = 0;
                varArgTupleSize = 0;
            }
                
            if (variadic
                ? argumentCount + varArgTupleSize < fixedArgumentCount
                : argumentCount + varArgTupleSize != fixedArgumentCount) {
                TRAP(Spk_wrongNumberOfArguments, 0);
            }
            if (fixedArgumentCount > argumentCount) {
                /* copy & reverse fixed arguments from stack */
                for (i = 0; i < argumentCount; ++p, ++i) {
                    tmp = *p;
                    *p = stackPointer[varArg + argumentCount - i - 1];
                    Spk_INCREF(*p);
                    Spk_DECREF(tmp);
                }
                excessStackArgCount = 0;
                /* copy fixed arguments from array */
                consumedArrayArgCount = fixedArgumentCount - argumentCount;
                for (i = 0; i < consumedArrayArgCount; ++p, ++i) {
                    tmp = *p;
                    *p = Spk_GetArg(varArgTuple, i);
                    Spk_DECREF(tmp);
                }
            } else {
                /* copy & reverse fixed arguments from stack */
                for (i = 0; i < fixedArgumentCount; ++p, ++i) {
                    tmp = *p;
                    *p = stackPointer[varArg + argumentCount - i - 1];
                    Spk_INCREF(*p);
                    Spk_DECREF(tmp);
                }
                excessStackArgCount = argumentCount - fixedArgumentCount;
                consumedArrayArgCount = 0;
            }
            if (variadic) {
                tmp = *p;
                /* initialize the argument array variable */
                *p++ = SpkHost_GetArgs(
                    stackPointer + varArg, excessStackArgCount,
                    varArgTuple, consumedArrayArgCount
                    );
                Spk_DECREF(tmp);
            }
            
            /* clean up the caller's stack */
            CLEAN(varArg + argumentCount + 1);
            newContext->caller->stackp = stackPointer;
            
            stackPointer = newContext->stackp;
            framePointer = newContext->u.m.framep;
            break; }
            
        case Spk_OPCODE_NATIVE: {
            SpkUnknown *result, *arg1 = 0, *arg2 = 0;
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
#ifdef MALTIPY
            } else if (PyErr_Occurred()) {
 exception:
                while (self->activeContext && self->activeContext->mark == &mark) {
                    SpkContext *thisCntx = self->activeContext;
                    addTraceback(thisCntx);
                    self->activeContext = thisCntx->caller; /* steal reference */
                    thisCntx->caller = 0;
                    thisCntx->pc = 0;
                    if (thisCntx->homeContext == thisCntx) {
                        /* break cycles */
                        tmp = (SpkUnknown *)thisCntx->homeContext;
                        thisCntx->homeContext = 0;
                        Spk_DECREF(tmp);
                    }
                    Spk_DECREF(thisCntx);
                }
                return 0;
#endif /* MALTIPY */
            } else if (self->activeContext->mark != &mark) {
                return 0; /* unwind */
            } else { /* unwinding is done, and the result is already
                        on the stack */
                goto fetchContextRegisters;
            }
            break; }
            
        case Spk_OPCODE_NATIVE_PUSH_INST_VAR: {
            SpkInstVarType type;
            size_t offset;
            char *addr;
            
            DECODE_UINT(type);
            DECODE_UINT(offset);
            addr = (char *)receiver + offset;
            switch (type) {
            case Spk_T_OBJECT:
                tmp = *(SpkUnknown **)addr;
                if (!tmp)
                    tmp = Spk_null;
                Spk_INCREF(tmp);
                break;
            case Spk_T_SIZE:
                tmp = SpkHost_IntegerFromCLong(*(size_t *)addr);
                break;
            default:
                unknownOpcode(self);
                tmp = Spk_uninit;
                Spk_INCREF(tmp);
            }
            PUSH(tmp);
            break; }
            
        case Spk_OPCODE_NATIVE_STORE_INST_VAR: {
            SpkInstVarType type;
            size_t offset;
            char *addr;
            
            DECODE_UINT(type);
            DECODE_UINT(offset);
            addr = (char *)receiver + offset;
            tmp = STACK_TOP();
            switch (type) {
            case Spk_T_OBJECT:
                Spk_INCREF(tmp);
                Spk_XDECREF(*(SpkUnknown **)addr);
                *(SpkUnknown **)addr = tmp;
                break;
            default:
                unknownOpcode(self);
            }
            break; }
            
        case Spk_OPCODE_RESTORE_SENDER: {
            SpkContext *thisCntx, *caller;
            /* restore sender */
            assert(!self->newContext);
            self->newContext = homeContext->sender;
            if (!self->newContext) {
                --instructionPointer;
                TRAP(Spk_cannotReturn, 0);
                break;
            }
            Spk_INCREF(self->newContext);
            if (!self->newContext->pc) {
                --instructionPointer;
                TRAP(Spk_cannotReturn, 0);
                break;
            }
            thisCntx = self->activeContext;
            caller = 0;
            while (thisCntx != self->newContext) {
                Spk_XDECREF(caller);
                caller = thisCntx->caller;
                thisCntx->caller = 0;
                thisCntx->pc = 0;
                if (thisCntx->homeContext == thisCntx) {
                    /* break cycles */
                    tmp = (SpkUnknown *)thisCntx->homeContext;
                    thisCntx->homeContext = 0;
                    Spk_DECREF(tmp);
                }
                thisCntx = caller;
            }
            Spk_XDECREF(caller);
            goto restore; }
        case Spk_OPCODE_RESTORE_CALLER: {
            SpkUnknown *result;
            /* restore caller */
            assert(!self->newContext);
            self->newContext = self->activeContext->caller; /* steal reference */
            self->activeContext->caller = 0;
            /* suspend */
            self->activeContext->pc = instructionPointer + 1; /* skip 'ret' */
            self->activeContext->stackp = stackPointer + 1; /* skip result */
            if (self->activeContext->homeContext == self->activeContext) {
                /* break cycles */
                tmp = (SpkUnknown *)self->activeContext->homeContext;
                self->activeContext->homeContext = 0;
                Spk_DECREF(tmp);
            }
 restore:
            result = POP_OBJECT();
            
            Spk_DECREF(self->activeContext);
            self->activeContext = self->newContext; self->newContext = 0;
            homeContext = self->activeContext->homeContext;
            
            stackPointer = self->activeContext->stackp;
            framePointer = homeContext->u.m.framep;
            
            PUSH(result);
            break; }
            
            /*** thunk opcodes ***/
        case Spk_OPCODE_THUNK: {
            /* new thunk */
            SpkThunk *thunk;
            if (argumentCount != 0) {
                TRAP(Spk_wrongNumberOfArguments, 0);
            }
            if (varArg) {
                SpkUnknown *varArgTuple = stackPointer[0];
                if (!Spk_IsArgs(varArgTuple)) {
                    TRAP(Spk_mustBeTuple, 0);
                }
                if (Spk_ArgsSize(varArgTuple) != 0) {
                    TRAP(Spk_wrongNumberOfArguments, 0);
                }
                Spk_DECREF(varArgTuple);
                POP(1);
            }
            thunk = (SpkThunk *)SpkObject_New(Spk_ClassThunk);
            thunk->receiver = receiver;        /* steal reference from stack */
            thunk->method = method;            Spk_INCREF(method);
            thunk->methodClass = methodClass;  Spk_INCREF(methodClass);
            thunk->pc = instructionPointer;
            stackPointer[0] = (SpkUnknown *)thunk; /* replace receiver */
            goto ret; }
        case Spk_OPCODE_CALL_THUNK: {
            SpkThunk *thunk = (SpkThunk *)(receiver);
            receiver = thunk->receiver;
            method = thunk->method;
            methodClass = thunk->methodClass;
            instructionPointer = thunk->pc;
            goto jump; }
        
            /*** block context opcodes ***/
        case Spk_OPCODE_CALL_BLOCK: {
            SpkContext *blockContext;
            SpkUnknown **p;
            size_t i;
            
            blockContext = (SpkContext *)(receiver);
            if (blockContext->caller || !blockContext->pc) {
                TRAP(Spk_cannotReenterBlock, 0);
            }
            Spk_INCREF(blockContext);
            
            /* process arguments */
            if (varArg) {
                SpkUnknown *varArgTuple;
                
                varArgTuple = stackPointer[0];
                if (!Spk_IsArgs(varArgTuple)) {
                    TRAP(Spk_mustBeTuple, 0);
                }
                if (argumentCount + Spk_ArgsSize(varArgTuple) != blockContext->u.b.nargs) {
                    TRAP(Spk_wrongNumberOfArguments, 0);
                }
                
                /* copy & reverse arguments from stack */
                p = &blockContext->homeContext->u.m.framep[blockContext->u.b.index];
                for (i = 0; i < argumentCount; ++p, ++i) {
                    tmp = *p;
                    *p = stackPointer[1 + argumentCount - i - 1];
                    Spk_INCREF(*p);
                    Spk_DECREF(tmp);
                }
                
                /* copy arguments from array */
                for (i = 0; i < Spk_ArgsSize(varArgTuple); ++p, ++i) {
                    tmp = *p;
                    *p = Spk_GetArg(varArgTuple, i);
                    Spk_DECREF(tmp);
                }
                
            } else {
                if (argumentCount != blockContext->u.b.nargs) {
                    TRAP(Spk_wrongNumberOfArguments, 0);
                }
                
                /* copy & reverse arguments from stack */
                p = &blockContext->homeContext->u.m.framep[blockContext->u.b.index];
                for (i = 0; i < argumentCount; ++p, ++i) {
                    tmp = *p;
                    *p = stackPointer[argumentCount - i - 1];
                    Spk_INCREF(*p);
                    Spk_DECREF(tmp);
                }
                
            }
            
            /* clean up the caller's stack */
            CLEAN(varArg + argumentCount + 1);
            self->activeContext->stackp = stackPointer;
            
            blockContext->caller = self->activeContext;  Spk_INCREF(self->activeContext);
            blockContext->mark = &mark;
            Spk_DECREF(self->activeContext);
            self->activeContext = blockContext;
            goto fetchContextRegisters; }
            
            /*** debugging ***/
        case Spk_OPCODE_CHECK_STACKP: {
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

SpkUnknown *SpkInterpreter_SendMessage(
    SpkInterpreter *interpreter,
    SpkUnknown *obj,
    unsigned int namespace,
    SpkUnknown *selector,
    SpkUnknown *argumentArray
    )
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
        thisContext->pc = SpkMethod_OPCODES(start) + 7;
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
    Spk_INCREF(obj);
    Spk_INCREF(selector);
    Spk_INCREF(argumentArray);
    Spk_DECREF(thisContext->stackp[-1]);
    Spk_DECREF(thisContext->stackp[-2]);
    Spk_DECREF(thisContext->stackp[-3]);
    Spk_DECREF(thisContext->stackp[-4]);
    *--thisContext->stackp = obj;
    *--thisContext->stackp = SpkHost_IntegerFromCLong(namespace);
    *--thisContext->stackp = selector;
    *--thisContext->stackp = argumentArray;
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


/*------------------------------------------------------------------------*/
/* fibers */

SpkSemaphore *SpkSemaphore_New() {
    return XXX /*(Semaphore *)SpkObject_New(Spk_ClassSemaphore) */ ;
}

int SpkSemaphore_IsEmpty(SpkSemaphore *self) {
    return !self->firstLink;
}

void SpkSemaphore_AddLast(SpkSemaphore *self, SpkFiber *link) {
    Spk_INCREF(link);
    Spk_INCREF(link);
    Spk_XDECREF(self->lastLink);
    if (SpkSemaphore_IsEmpty(self)) {
        self->firstLink = link;
    } else {
        Spk_XDECREF(self->lastLink->nextLink);
        self->lastLink->nextLink = link;
    }
    self->lastLink = link;
}

SpkFiber *SpkSemaphore_RemoveFirst(SpkSemaphore *self) {
    SpkFiber *first = self->firstLink;
    if (!first) {
        /* XXX: exception */
        return 0;
    }
    if (first == self->lastLink) {
        Spk_DECREF(first);
        self->firstLink = self->lastLink = 0;
    } else {
        Spk_XINCREF(first->nextLink);
        self->firstLink = first->nextLink;
    }
    Spk_XDECREF(first->nextLink);
    first->nextLink = 0;
    return first;
}

void SpkInterpreter_SynchronousSignal(SpkInterpreter *self, SpkSemaphore *aSemaphore) {
    if (SpkSemaphore_IsEmpty(aSemaphore)) {
        ++aSemaphore->excessSignals;
    } else {
        SpkInterpreter_Resume(self, SpkSemaphore_RemoveFirst(aSemaphore));
    }
}

void SpkInterpreter_TransferTo(SpkInterpreter *self, SpkFiber *newFiber) {
    SpkFiber *oldFiber = self->scheduler->activeFiber;
    oldFiber->suspendedContext = self->activeContext;
    self->scheduler->activeFiber = newFiber;
    assert(!self->newContext);
    self->newContext = newFiber->suspendedContext;
    Spk_INCREF(self->newContext);
}

SpkFiber *SpkInterpreter_WakeHighestPriority(SpkInterpreter *self) {
    int p = HIGHEST_PRIORITY - 1;
    SpkSemaphore **fiberLists = self->scheduler->quiescentFiberLists;
    SpkSemaphore *fiberList = fiberLists[p];
    while (SpkSemaphore_IsEmpty(fiberList)) {
        --p;
        if (p < 0) {
            return trap(self, Spk_noRunnableFiber, 0);
        }
        fiberList = fiberLists[p];
    }
    return SpkSemaphore_RemoveFirst(fiberList);
}

/* ----- private ----- */

void SpkInterpreter_PutToSleep(SpkInterpreter *self, SpkFiber *aFiber) {
    SpkSemaphore **fiberLists = self->scheduler->quiescentFiberLists;
    SpkSemaphore *fiberList = fiberLists[aFiber->priority - 1];
    SpkSemaphore_AddLast(fiberList, aFiber);
    aFiber->myList = fiberList;
}

void SpkInterpreter_Resume(SpkInterpreter *self, SpkFiber *aFiber) {
    SpkFiber *activeFiber = self->scheduler->activeFiber;
    if (aFiber->priority > activeFiber->priority) {
        SpkInterpreter_PutToSleep(self, activeFiber);
        SpkInterpreter_TransferTo(self, aFiber);
    } else {
        SpkInterpreter_PutToSleep(self, aFiber);
    }
}


/*------------------------------------------------------------------------*/
/* debug support */

static unsigned int lineNumber(size_t offset, SpkMethod *method) {
    SpkOpcode *instructionPointer, *end;
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

void SpkInterpreter_PrintCallStack(SpkInterpreter *self) {
    SpkContext *ctxt, *home;
    SpkMethod *method;
    SpkBehavior *methodClass;
    SpkUnknown *methodSel, *source;
    size_t offset;
    unsigned int lineNo;
    
    for (ctxt = self->activeContext; ctxt; ctxt = ctxt->sender) {
        home = ctxt->homeContext;
        method = home->u.m.method;
        methodClass = home->u.m.methodClass;
        methodSel = SpkBehavior_FindSelectorOfMethod(methodClass, method);
        source = method->debug.source;
        offset = ctxt->pc - SpkMethod_OPCODES(method);
        lineNo = lineNumber(offset, method);
        printf("%04lx %p%s%s::%s at %s:%u\n",
               (unsigned long)offset,
               ctxt,
               (ctxt == home ? " " : " [] in "),
               SpkBehavior_NameAsCString(methodClass),
               methodSel ? SpkHost_SelectorAsCString(methodSel) : "<unknown>",
               source ? SpkHost_StringAsCString(source) : "<unknown>",
               lineNo);
        Spk_XDECREF(methodSel);
    }
}


/*------------------------------------------------------------------------*/
/* error handling */

static SpkFiber *trap(SpkInterpreter *self, SpkUnknown *selector, SpkUnknown *argument) {
    /* XXX */
    halt(self, selector, argument);
    return 0;
}

#ifdef MALTIPY
static void runtimeError(SpkInterpreter *self, SpkUnknown *selector, SpkUnknown *argument) {
    SpkUnknown *errorString;
    
    selector = PyObject_Str(selector);
    if (argument) {
        SpkMessage *message = Spk_CAST(Message, argument);
        if (message) {
            SpkUnknown *messageSelector = PyObject_Str(message->selector);
            errorString = PyString_FromFormat("%s %s",
                                              SpkHost_SymbolAsCString(selector),
                                              SpkHost_SymbolAsCString(messageSelector));
            Spk_DECREF(messageSelector);
        } else {
            SpkUnknown *argStr = PyObject_Str(argument);
            errorString = PyString_FromFormat("%s %s",
                                              SpkHost_SymbolAsCString(selector),
                                              PyString_AsString(argStr));
            Spk_DECREF(argStr);
        }
    } else {
        errorString = selector;
        Spk_INCREF(errorString);
    }
    Spk_HaltWithString(Spk_HALT_RUNTIME_ERROR, errorString);
    Spk_DECREF(errorString);
    Spk_DECREF(selector);
}
#endif /* MALTIPY */

static void unknownOpcode(SpkInterpreter *self) {
    trap(self, Spk_unknownOpcode, 0);
}

void halt(SpkInterpreter *self, SpkUnknown *selector, SpkUnknown *argument) {
    if (argument) {
        SpkUnknown *symbol = 0; SpkMessage *message;
        if (SpkHost_IsSymbol(argument)) {
            symbol = argument;
        }
        message = Spk_CAST(Message, argument);
        if (symbol) {
            printf("\n%s '%s'", SpkHost_SymbolAsCString(selector), SpkHost_SymbolAsCString(symbol));
        } else if (message) {
            printf("\n%s '%s'", SpkHost_SymbolAsCString(selector), SpkHost_SymbolAsCString(message->selector));
        } else {
            printf("\n%s", SpkHost_SymbolAsCString(selector));
        }
    } else {
        printf("\n%s", SpkHost_SymbolAsCString(selector));
    }
    printf("\n\n");
    if (!self->printingStack) {
        self->printingStack = 1;
        SpkInterpreter_PrintCallStack(self);
    }
    abort();
}
