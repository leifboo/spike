
#ifndef __spk_behavior_h__
#define __spk_behavior_h__


#include "interp.h"

#include <stddef.h>


typedef struct SpkBehavior SpkBehavior;


typedef SpkMethod *SpkOperTable[Spk_NUM_OPER];
typedef SpkMethod *SpkOperCallTable[Spk_NUM_CALL_OPER];

typedef unsigned int SpkMethodNamespace;

enum /*SpkMethodNamespace*/ {
    Spk_METHOD_NAMESPACE_RVALUE,
    Spk_METHOD_NAMESPACE_LVALUE,
    
    Spk_NUM_METHOD_NAMESPACES
};


typedef enum SpkInstVarType {
    Spk_T_SHORT,
    Spk_T_INT,
    Spk_T_LONG,
    Spk_T_FLOAT,
    Spk_T_DOUBLE,
    Spk_T_STRING,
    Spk_T_OBJECT,
    Spk_T_CHAR,
    Spk_T_BYTE,
    Spk_T_UBYTE,
    Spk_T_USHORT,
    Spk_T_UINT,
    Spk_T_ULONG,
    Spk_T_STRING_INPLACE,
    Spk_T_LONGLONG,
    Spk_T_ULONGLONG,
    Spk_T_SIZE
} SpkInstVarType;

enum /*flags*/ {
    SpkAccessor_READ  = 0x1, /* create 'get' accessor */
    SpkAccessor_WRITE = 0x2  /* create 'set' accessor */
};

typedef struct SpkTraverse {
    void (*init)(SpkObject *);
    SpkUnknown **(*current)(SpkObject *);
    void (*next)(SpkObject *);
} SpkTraverse;

typedef struct SpkAccessorTmpl {
    const char *name;
    SpkInstVarType type;
    size_t offset;
    unsigned int flags;
} SpkAccessorTmpl;

typedef struct SpkMethodTmplList {
    struct SpkMethodTmpl *first;
    struct SpkMethodTmpl *last;
} SpkMethodTmplList;

typedef struct SpkClassTmplList {
    struct SpkClassTmpl *first;
    struct SpkClassTmpl *last;
} SpkClassTmplList;

typedef struct SpkMethodTmpl {
    /* C code */
    const char *name;
    SpkNativeCodeFlags nativeFlags;
    SpkNativeCode nativeCode;
    /* bytecode */
    size_t bytecodeSize;
    SpkOpcode *bytecode;
    struct SpkMethodTmpl *nextInScope;
    SpkMethodNamespace ns; /* XXX: suspect */
    SpkUnknown *selector;
    struct {
        SpkUnknown *source;
        size_t lineCodeTally;
        SpkOpcode *lineCodes;
    } debug;
    SpkClassTmplList nestedClassList;
    SpkMethodTmplList nestedMethodList;
} SpkMethodTmpl;

typedef struct SpkBehaviorTmpl {
    /* C code */
    SpkAccessorTmpl *accessors;
    SpkMethodTmpl *methods;
    SpkMethodTmpl *lvalueMethods;
    size_t instVarOffset;
    size_t itemSize;
    void (*zero)(SpkObject *);
    void (*dealloc)(SpkObject *);
    SpkTraverse *traverse;
    /* bytecode */
    size_t instVarCount;
    SpkMethodTmplList methodList;
    SpkClassTmplList nestedClassList;
} SpkBehaviorTmpl;


struct SpkBehavior {
    SpkObject base;
    SpkBehavior *superclass;
    struct SpkModule *module;
    
    SpkUnknown *methodDict[Spk_NUM_METHOD_NAMESPACES];
    SpkOperTable operTable;
    SpkOperCallTable operCallTable;
    SpkMethod *assignInd;   /*  "*p = v"    */
    SpkMethod *assignIndex; /*  "a[i] = v"  */
    
    /* low-level hooks */
    void (*zero)(SpkObject *);
    void (*dealloc)(SpkObject *);
    SpkTraverse traverse;
    
    /* memory layout of instances */
    size_t instVarOffset;
    size_t instVarBaseIndex;
    size_t instVarCount;
    size_t instanceSize;
    size_t itemSize;
};


extern struct SpkClassTmpl Spk_ClassBehaviorTmpl;


void SpkBehavior_Init(SpkBehavior *self, SpkBehavior *superclass, struct SpkModule *module, size_t instVarCount);
void SpkBehavior_InitFromTemplate(SpkBehavior *self, SpkBehaviorTmpl *tmpl, SpkBehavior *superclass, struct SpkModule *module);
void SpkBehavior_AddMethodsFromTemplate(SpkBehavior *self, SpkBehaviorTmpl *tmpl);
void SpkBehavior_InsertMethod(SpkBehavior *, SpkMethodNamespace, SpkUnknown *, SpkMethod *);
SpkMethod *SpkBehavior_LookupMethod(SpkBehavior *, SpkMethodNamespace, SpkUnknown *);
SpkUnknown *SpkBehavior_FindSelectorOfMethod(SpkBehavior *, SpkMethod *);
const char *SpkBehavior_NameAsCString(SpkBehavior *);


#endif /* __spk_behavior_h__ */
