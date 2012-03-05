
#ifndef __native_h__
#define __native_h__


#include "obj.h"
#include "oper.h"

#include <stdarg.h>
#include <stddef.h>


struct Array;
struct Interpreter;
struct Method;
struct Symbol;


#define SUPER (0)


typedef unsigned int NativeCodeFlags;

enum /*NativeCodeFlags*/ {
    NativeCode_ARGS_0              = 0x0000,
    NativeCode_ARGS_1              = 0x0001,
    NativeCode_ARGS_2              = 0x0002,
    NativeCode_ARGS_ARRAY          = 0x0003,
    NativeCode_ARGS_KEYWORDS       = 0x0004,
    NativeCode_ARGS_ARRAY_KEYWORDS = 0x0007,
    NativeCode_SIGNATURE_MASK      = 0x000f,
    
    NativeCode_LEAF                = 0x0010,
    
    NativeCode_UNARY_OPER          = NativeCode_ARGS_0,
    NativeCode_BINARY_OPER         = NativeCode_ARGS_1,
    NativeCode_TERNARY_OPER        = NativeCode_ARGS_2
};

enum /*halt codes*/ {
    HALT_ASSERTION_ERROR,
    HALT_ERROR,
    HALT_INDEX_ERROR,
    HALT_KEY_ERROR,
    HALT_MEMORY_ERROR,
    HALT_RUNTIME_ERROR,
    HALT_TYPE_ERROR,
    HALT_VALUE_ERROR
};


typedef Unknown *(*NativeCode)(Unknown *, Unknown *, Unknown *);


struct Method *NewNativeMethod(NativeCodeFlags, NativeCode);

Unknown *SendMessage(struct Interpreter *, Unknown *, unsigned int, struct Symbol *, struct Array *);

Unknown *SendOper(struct Interpreter *, Unknown *, Oper, ...);
Unknown *VSendOper(struct Interpreter *, Unknown *, Oper, va_list);
Unknown *Call(struct Interpreter *, Unknown *, CallOper, ...);
Unknown *VCall(struct Interpreter *, Unknown *, CallOper, va_list);
Unknown *Attr(struct Interpreter *, Unknown *, struct Symbol *);
Unknown *SetAttr(struct Interpreter *, Unknown *, struct Symbol *, Unknown *);
Unknown *Send(struct Interpreter *, Unknown *, struct Symbol *, ...);
Unknown *VSend(struct Interpreter *, Unknown *, struct Symbol *, va_list);
Unknown *SendWithArguments(struct Interpreter *, Unknown *, struct Symbol *, struct Array *);

void Halt(int, const char *);
void HaltWithFormat(int, const char *, ...);
void HaltWithString(int, Unknown *);

int IsArgs(Unknown *);
size_t ArgsSize(Unknown *);
Unknown *GetArg(Unknown *, size_t);


#endif /* __native_h__ */
