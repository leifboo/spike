
#ifndef __behavior_h__
#define __behavior_h__


#include "interp.h"

#include <stddef.h>


typedef struct Behavior Behavior;


typedef struct SpecialSelector {
    char *messageSelectorStr;
    size_t argumentCount;
    Symbol *messageSelector;
} SpecialSelector;

extern SpecialSelector specialSelectors[NUM_OPER];


typedef struct oper_entry_t {
    Method *method;
    struct Behavior *methodClass; /* class in which 'method' is defined */
} oper_entry_t;

typedef oper_entry_t oper_table_t[NUM_OPER];


typedef enum SpkInstVarType {
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
} SpkInstVarType;

enum /*flags*/ {
    SpkAccessor_READ  = 0x1, /* create 'get' accessor */
    SpkAccessor_WRITE = 0x2  /* create 'set' accessor */
};

typedef struct SpkAccessorTmpl {
    const char *name;
    SpkInstVarType type;
    size_t offset;
    unsigned int flags;
} SpkAccessorTmpl;

typedef struct SpkMethodTmpl {
    const char *name;
    SpkNativeCodeFlags flags;
    SpkNativeCode code;
} SpkMethodTmpl;

typedef struct SpkClassTmpl {
    size_t instVarOffset;
    size_t instanceSize;
    SpkAccessorTmpl *accessors;
    SpkMethodTmpl *methods;
} SpkClassTmpl;


struct Behavior {
    Object base;
    Behavior *superclass;
    struct Module *module;
    struct IdentityDictionary *methodDict;
    oper_table_t operTable;
    oper_entry_t operCall;
    
    /* temporary */
    Behavior *next;
    
    /* memory layout of instances */
    size_t instVarCount;
    size_t instVarOffset;
    size_t instanceSize;
    /*SpkAccessorTmpl accessorTmpl[1];*/
};

typedef struct BehaviorSubclass {
    Behavior base;
    Object *variables[1]; /* stretchy */
} BehaviorSubclass;


extern Behavior *ClassBehavior;
extern struct SpkClassTmpl ClassBehaviorTmpl;


Behavior *SpkBehavior_new(void);
Behavior *SpkBehavior_fromTemplate(SpkClassTmpl *template, Behavior *superclass, struct Module *module);
void SpkBehavior_init(Behavior *self, Behavior *superclass, struct Module *module, size_t instVarCount);
void SpkBehavior_initFromTemplate(Behavior *self, SpkClassTmpl *template, Behavior *superclass, struct Module *module);
void SpkBehavior_insertMethod(Behavior *, Symbol *, Method *);
Method *SpkBehavior_lookupMethod(Behavior *, Symbol *);
Symbol *SpkBehavior_findSelectorOfMethod(Behavior *, Method *);


#endif /* __behavior_h__ */
