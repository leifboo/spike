
#ifndef __native_h__
#define __native_h__


#include "oper.h"

#include <stdarg.h>
#include <stddef.h>


struct Interpreter;
struct Method;
struct Object;
struct Symbol;


#define Spk_super (0)


typedef enum SpkNativeCodeFlags {
    SpkNativeCode_ARGS_0              = 0x0000,
    SpkNativeCode_ARGS_1              = 0x0001,
    SpkNativeCode_ARGS_2              = 0x0002,
    SpkNativeCode_ARGS_ARRAY          = 0x0003,
    SpkNativeCode_ARGS_KEYWORDS       = 0x0004,
    SpkNativeCode_ARGS_ARRAY_KEYWORDS = 0x0007,
    SpkNativeCode_SIGNATURE_MASK      = 0x000f,
    
    SpkNativeCode_LEAF                = 0x0010,
    SpkNativeCode_THUNK               = 0x0020,
    
    SpkNativeCode_UNARY_OPER          = SpkNativeCode_ARGS_0,
    SpkNativeCode_BINARY_OPER         = SpkNativeCode_ARGS_1,
    SpkNativeCode_TERNARY_OPER        = SpkNativeCode_ARGS_2,
    SpkNativeCode_METH_ATTR           = SpkNativeCode_THUNK
} SpkNativeCodeFlags;

typedef struct Object *(*SpkNativeCode)(struct Object *, struct Object *, struct Object *);


struct Method *Spk_newNativeMethod(SpkNativeCodeFlags, SpkNativeCode);
struct Method *Spk_newNativeReadAccessor(unsigned int, size_t);
struct Method *Spk_newNativeWriteAccessor(unsigned int, size_t);

struct Object *Spk_oper(struct Interpreter *, struct Object *, Oper, ...);
struct Object *Spk_vOper(struct Interpreter *, struct Object *, Oper, va_list);
struct Object *Spk_call(struct Interpreter *, struct Object *, CallOper, ...);
struct Object *Spk_vCall(struct Interpreter *, struct Object *, CallOper, va_list);
struct Object *Spk_attr(struct Interpreter *, struct Object *, struct Symbol *);
struct Object *Spk_callAttr(struct Interpreter *, struct Object *, struct Symbol *, ...);

struct Method *Spk_thisMethod(struct Interpreter *);


extern struct Behavior *Spk_ClassNativeAccessor;
extern struct SpkClassTmpl Spk_ClassNativeAccessorTmpl;


#endif /* __native_h__ */
