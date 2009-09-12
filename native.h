
#ifndef __spk_native_h__
#define __spk_native_h__


#include "obj.h"
#include "oper.h"

#include <stdarg.h>
#include <stddef.h>


struct SpkInterpreter;
struct SpkMethod;


#define Spk_SUPER (0)


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

enum /*halt codes*/ {
    Spk_HALT_ASSERTION_ERROR,
    Spk_HALT_ERROR,
    Spk_HALT_INDEX_ERROR,
    Spk_HALT_MEMORY_ERROR,
    Spk_HALT_RUNTIME_ERROR,
    Spk_HALT_TYPE_ERROR,
    Spk_HALT_VALUE_ERROR
};


typedef SpkUnknown *(*SpkNativeCode)(SpkUnknown *, SpkUnknown *, SpkUnknown *);


struct SpkMethod *Spk_NewNativeMethod(SpkNativeCodeFlags, SpkNativeCode);

SpkUnknown *Spk_SendMessage(struct SpkInterpreter *, SpkUnknown *, unsigned int, SpkUnknown *, SpkUnknown *);

SpkUnknown *Spk_Oper(struct SpkInterpreter *, SpkUnknown *, SpkOper, ...);
SpkUnknown *Spk_VOper(struct SpkInterpreter *, SpkUnknown *, SpkOper, va_list);
SpkUnknown *Spk_Call(struct SpkInterpreter *, SpkUnknown *, SpkCallOper, ...);
SpkUnknown *Spk_VCall(struct SpkInterpreter *, SpkUnknown *, SpkCallOper, va_list);
SpkUnknown *Spk_Attr(struct SpkInterpreter *, SpkUnknown *, SpkUnknown *);
SpkUnknown *Spk_SetAttr(struct SpkInterpreter *, SpkUnknown *, SpkUnknown *, SpkUnknown *);
SpkUnknown *Spk_CallAttr(struct SpkInterpreter *, SpkUnknown *, SpkUnknown *, ...);
SpkUnknown *Spk_CallAttrWithArguments(struct SpkInterpreter *, SpkUnknown *, SpkUnknown *, SpkUnknown *);
SpkUnknown *Spk_Keyword(struct SpkInterpreter *, SpkUnknown *, SpkUnknown *, ...);
SpkUnknown *Spk_VKeyword(struct SpkInterpreter *, SpkUnknown *, SpkUnknown *, va_list);

void Spk_Halt(int, const char *);
void Spk_HaltWithFormat(int, const char *, ...);
void Spk_HaltWithString(int, SpkUnknown *);

int Spk_IsArgs(SpkUnknown *);
size_t Spk_ArgsSize(SpkUnknown *);
SpkUnknown *Spk_GetArg(SpkUnknown *, size_t);


#endif /* __spk_native_h__ */
