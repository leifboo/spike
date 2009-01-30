
#ifndef __behavior_h__
#define __behavior_h__


#include "interp.h"

#include <stddef.h>


typedef struct Behavior Behavior;


typedef struct SpecialSelector {
    char *messageSelectorStr;
    size_t argumentCount;
    struct Symbol *messageSelector;
} SpecialSelector;

extern SpecialSelector Spk_operSelectors[NUM_OPER];
extern SpecialSelector Spk_operCallSelectors[NUM_CALL_OPER];


typedef struct oper_entry_t {
    Method *method;
    struct Behavior *methodClass; /* class in which 'method' is defined */
} oper_entry_t;

typedef oper_entry_t oper_table_t[NUM_OPER];
typedef oper_entry_t oper_call_table_t[NUM_CALL_OPER];

typedef enum MethodNamespace {
    METHOD_NAMESPACE_RVALUE,
    METHOD_NAMESPACE_LVALUE,
    
    NUM_METHOD_NAMESPACES
} MethodNamespace;


typedef struct traverse_t {
    void (*init)(Object *);
    Object **(*current)(Object *);
    void (*next)(Object *);
} traverse_t;


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
    const char *name;
    size_t instVarOffset;
    size_t instanceSize;
    size_t itemSize;
    SpkAccessorTmpl *accessors;
    SpkMethodTmpl *methods;
    SpkMethodTmpl *lvalueMethods;
    traverse_t *traverse;
} SpkClassTmpl;


struct Behavior {
    Object base;
    Behavior *superclass;
    struct Module *module;
    struct {
        struct IdentityDictionary *methodDict;
        oper_table_t operTable;
        oper_call_table_t operCallTable;
    } ns[NUM_METHOD_NAMESPACES];
    
    /* static chain */
    Object *outer;
    Behavior *outerClass;
    
    /* temporary */
    Behavior *next;
    Behavior *nextInScope;
    struct {
        struct Behavior *first;
        struct Behavior *last;
    } nestedClassList;
    
    /* memory layout of instances */
    traverse_t traverse;
    size_t instVarCount;
    size_t instVarOffset;
    size_t instanceSize;
    size_t itemSize;
    /*SpkAccessorTmpl accessorTmpl[1];*/
};

typedef struct BehaviorSubclass {
    Behavior base;
    Object *variables[1]; /* stretchy */
} BehaviorSubclass;


typedef struct BootRec {
    Behavior **klass;
    SpkClassTmpl *klassTmpl;
    Object **var;
    Behavior **superclass;
    SpkClassTmpl *init;
} BootRec;


extern Behavior *Spk_ClassBehavior;
extern struct SpkClassTmpl Spk_ClassBehaviorTmpl;


Behavior *SpkBehavior_new(void);
Behavior *SpkBehavior_fromTemplate(SpkClassTmpl *template, Behavior *superclass, struct Module *module);
void SpkBehavior_init(Behavior *self, Behavior *superclass, struct Module *module, size_t instVarCount);
void SpkBehavior_initFromTemplate(Behavior *self, SpkClassTmpl *template, Behavior *superclass, struct Module *module);
void SpkBehavior_addMethodsFromTemplate(Behavior *self, SpkClassTmpl *template);
void SpkBehavior_insertMethod(Behavior *, MethodNamespace, struct Symbol *, Method *);
Method *SpkBehavior_lookupMethod(Behavior *, MethodNamespace, struct Symbol *);
struct Symbol *SpkBehavior_findSelectorOfMethod(Behavior *, Method *);
char *SpkBehavior_name(Behavior *);


#endif /* __behavior_h__ */
