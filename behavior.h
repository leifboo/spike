
#ifndef __behavior_h__
#define __behavior_h__


#include "interp.h"

#include <stddef.h>


typedef struct Behavior Behavior;


typedef Method *OperTable[NUM_OPER];
typedef Method *OperCallTable[NUM_CALL_OPER];

typedef unsigned int MethodNamespace;

enum /*MethodNamespace*/ {
    METHOD_NAMESPACE_RVALUE,
    METHOD_NAMESPACE_LVALUE,
    
    NUM_METHOD_NAMESPACES
};


typedef enum InstVarType {
    T_SHORT,
    T_INT,
    T_LONG,
    T_FLOAT,
    T_DOUBLE,
    T_STRING,
    T_OBJECT,
    T_CHAR,
    T_BYTE,
    T_UBYTE,
    T_USHORT,
    T_UINT,
    T_ULONG,
    T_STRING_INPLACE,
    T_LONGLONG,
    T_ULONGLONG,
    T_SIZE
} InstVarType;

enum /*flags*/ {
    Accessor_READ  = 0x1, /* create 'get' accessor */
    Accessor_WRITE = 0x2  /* create 'set' accessor */
};

typedef struct AccessorTmpl {
    const char *name;
    InstVarType type;
    size_t offset;
    unsigned int flags;
} AccessorTmpl;

typedef struct MethodTmplList {
    struct MethodTmpl *first;
    struct MethodTmpl *last;
} MethodTmplList;

typedef struct ClassTmplList {
    struct ClassTmpl *first;
    struct ClassTmpl *last;
} ClassTmplList;

typedef struct MethodTmpl {
    /* C code */
    const char *name;
    NativeCodeFlags nativeFlags;
    NativeCode nativeCode;
    /* bytecode */
    size_t bytecodeSize;
    Opcode *bytecode;
    struct MethodTmpl *nextInScope;
    MethodNamespace ns; /* XXX: suspect */
    Unknown *selector;
    struct {
        Unknown *source;
        size_t lineCodeTally;
        Opcode *lineCodes;
    } debug;
    ClassTmplList nestedClassList;
    MethodTmplList nestedMethodList;
} MethodTmpl;

typedef struct BehaviorTmpl {
    /* C code */
    AccessorTmpl *accessors;
    MethodTmpl *methods;
    MethodTmpl *lvalueMethods;
    size_t instVarOffset;
    size_t itemSize;
    void (*zero)(Object *);
    void (*dealloc)(Object *, Unknown **);
    /* bytecode */
    size_t instVarCount;
    MethodTmplList methodList;
    ClassTmplList nestedClassList;
} BehaviorTmpl;


struct Behavior {
    Object base;
    Behavior *superclass;
    struct Module *module;
    
    Unknown *methodDict[NUM_METHOD_NAMESPACES];
    OperTable operTable;
    OperCallTable operCallTable;
    Method *assignInd;   /*  "*p = v"    */
    Method *assignIndex; /*  "a[i] = v"  */
    
    /* low-level hooks */
    void (*zero)(Object *);
    void (*dealloc)(Object *, Unknown **);
    
    /* memory layout of instances */
    size_t instVarOffset;
    size_t instVarBaseIndex;
    size_t instVarCount;
    size_t instanceSize;
    size_t itemSize;
};


extern struct ClassTmpl ClassBehaviorTmpl;


void Behavior_Init(Behavior *self, Behavior *superclass, struct Module *module, size_t instVarCount);
void Behavior_InitFromTemplate(Behavior *self, BehaviorTmpl *tmpl, Behavior *superclass, struct Module *module);
void Behavior_AddMethodsFromTemplate(Behavior *self, BehaviorTmpl *tmpl);
void Behavior_InsertMethod(Behavior *, MethodNamespace, Unknown *, Method *);
Method *Behavior_LookupMethod(Behavior *, MethodNamespace, Unknown *);
Unknown *Behavior_FindSelectorOfMethod(Behavior *, Method *);
const char *Behavior_NameAsCString(Behavior *);


#endif /* __behavior_h__ */
