
#include "disasm.h"

#include "cgen.h"
#include "heart.h"
#include "host.h"
#include "module.h"
#include "rodata.h"

#include <string.h>
#include <time.h>


#define DECODE_UINT(result) do { \
    Opcode byte; \
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
    Opcode byte; \
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


typedef struct Level {
    struct Level *outer;
    const char *name;
} Level;

typedef struct Visitor {
    void (*preMethod)(MethodTmpl *, Unknown **, Level *,
                      void *);
    void (*postMethod)(MethodTmpl *, Unknown **, Level *,
                       void *);
    void (*preMethodNamespace)(MethodNamespace, Level *, void *);
    void (*postMethodNamespace)(MethodNamespace, Level *, void *);
    void (*preClassBody)(BehaviorTmpl *, Level *, void *);
    void (*postClassBody)(BehaviorTmpl *, Level *, void *);
    void (*preClass)(ClassTmpl *, Level *, void *);
    void (*postClass)(ClassTmpl *, Level *, void *);
    void (*preModule)(ModuleTmpl *, Level *, void *);
    void (*postModule)(ModuleTmpl *, Level *, void *);
} Visitor;


/*------------------------------------------------------------------------*/
/* utilities */

static void tab(Level *level, FILE *out) {
    for ( ; level->outer; level = level->outer)
        fprintf(out, "\t");
}

static void spaces(Level *level, FILE *out) {
    for ( ; level->outer; level = level->outer)
        fprintf(out, "    ");
}

static void nest(Level *level, FILE *out) {
    if (level->outer)
        nest(level->outer, out);
    fprintf(out, "%s_", level->name);
}


/*------------------------------------------------------------------------*/
/* disassembly */

static void disassembleMethod(MethodTmpl *method,
                              Unknown **literals,
                              Level *level,
                              void *closure)
{
    Opcode *begin, *ip, *instructionPointer, *end;
    FILE *out;
    
    out = (FILE *)closure;
    
    tab(level->outer, out);
    fprintf(out, "%s\n", level->name);
    
    begin = method->bytecode;
    end = begin + method->bytecodeSize;
    instructionPointer = begin;
    
    while (instructionPointer < end) {
        Opcode opcode;
        const char *keyword = 0;
        Unknown *selector = 0;
        const char *mnemonic = "unk", *base = 0;
        size_t index = 0;
        ptrdiff_t displacement = 0;
        size_t contextSize = 0;
        size_t argumentCount = 0, *pArgumentCount = 0;
        size_t minArgumentCount = 0, maxArgumentCount = 0;
        size_t stackSize = 0;
        size_t count = 0, *pCount = 0;
        unsigned int oper = 0;
        size_t label = 0, *pLabel = 0;
        size_t i;
        Unknown *literal = 0;
        
        ip = instructionPointer;
        opcode = *instructionPointer++;
        
        switch (opcode) {
            
        case OPCODE_NOP: mnemonic = "nop"; break;
            
        case OPCODE_PUSH_LOCAL:    mnemonic = "push"; base = "local";    goto push;
        case OPCODE_PUSH_INST_VAR: mnemonic = "push"; base = "receiver"; goto push;
        case OPCODE_PUSH_GLOBAL:   mnemonic = "push"; base = "global";   goto push;
 push:
            DECODE_UINT(index);
            break;
        case OPCODE_PUSH_LITERAL:
            mnemonic = "push";
            base = "literal";
            DECODE_UINT(index);
            literal = literals[index];
            break;
            
        case OPCODE_PUSH_SELF:     mnemonic = "push"; keyword = "self";        break;
        case OPCODE_PUSH_SUPER:    mnemonic = "push"; keyword = "super";       break;
        case OPCODE_PUSH_FALSE:    mnemonic = "push"; keyword = "false";       break;
        case OPCODE_PUSH_TRUE:     mnemonic = "push"; keyword = "true";        break;
        case OPCODE_PUSH_NULL:     mnemonic = "push"; keyword = "null";        break;
        case OPCODE_PUSH_VOID:     mnemonic = "push"; keyword = "void";        break;
        case OPCODE_PUSH_CONTEXT:  mnemonic = "push"; keyword = "thisContext"; break;
            
        case OPCODE_DUP_N:
            DECODE_UINT(count);
            pCount = &count;
        case OPCODE_DUP:
            mnemonic = "dup";
            break;

        case OPCODE_STORE_LOCAL:    mnemonic = "store"; base = "local";    goto store;
        case OPCODE_STORE_INST_VAR: mnemonic = "store"; base = "receiver"; goto store;
        case OPCODE_STORE_GLOBAL:   mnemonic = "store"; base = "global";   goto store;
 store:
            DECODE_UINT(index);
            break;
            
        case OPCODE_POP: mnemonic = "pop"; break;
            
        case OPCODE_ROT:
            mnemonic = "rot";
            DECODE_UINT(count);
            pCount = &count;
            break;

        case OPCODE_BRANCH_IF_FALSE: mnemonic = "brf"; goto branch;
        case OPCODE_BRANCH_IF_TRUE:  mnemonic = "brt"; goto branch;
        case OPCODE_BRANCH_ALWAYS:   mnemonic = "bra"; goto branch;
 branch:
            DECODE_SINT(displacement);
            label = (ip + displacement) - begin;
            pLabel = &label;
            break;
            
        case OPCODE_ID:         mnemonic = "id";    break;
            
        case OPCODE_OPER:       mnemonic = "oper";  goto oper;
        case OPCODE_OPER_SUPER: mnemonic = "soper"; goto oper;
 oper:
            oper = (unsigned int)(*instructionPointer++);
            selector = *operSelectors[oper].selector;
            break;
            
        case OPCODE_CALL:           mnemonic = "call";   goto call;
        case OPCODE_CALL_VA:        mnemonic = "vcall";  goto call;
        case OPCODE_CALL_SUPER:     mnemonic = "scall";  goto call;
        case OPCODE_CALL_SUPER_VA:  mnemonic = "vscall"; goto call;
 call:
            oper = (unsigned int)(*instructionPointer++);
            selector = *operCallSelectors[oper].selector;
            DECODE_UINT(argumentCount);
            pArgumentCount = &argumentCount;
            break;

        case OPCODE_SET_IND:        mnemonic = "ind";  break;
        case OPCODE_SET_IND_SUPER:  mnemonic = "sind"; break;
        
        case OPCODE_SET_INDEX:           mnemonic = "index";   goto index;
        case OPCODE_SET_INDEX_VA:        mnemonic = "vindex";  goto index;
        case OPCODE_SET_INDEX_SUPER:     mnemonic = "sindex";  goto index;
        case OPCODE_SET_INDEX_SUPER_VA:  mnemonic = "vsindex"; goto index;
 index:
            DECODE_UINT(argumentCount);
            pArgumentCount = &argumentCount;
            break;

        case OPCODE_GET_ATTR:       mnemonic = "gattr";  goto attr;
        case OPCODE_GET_ATTR_SUPER: mnemonic = "gsattr"; goto attr;
        case OPCODE_SET_ATTR:       mnemonic = "sattr";  goto attr;
        case OPCODE_SET_ATTR_SUPER: mnemonic = "ssattr"; goto attr;
 attr:
            base = "literal";
            DECODE_UINT(index);
            literal = literals[index];
            break;
            
        case OPCODE_GET_ATTR_VAR:        mnemonic = "gattrv";   break;
        case OPCODE_GET_ATTR_VAR_SUPER:  mnemonic = "gsattrv";  break;
        case OPCODE_SET_ATTR_VAR:        mnemonic = "sattrv";   break;
        case OPCODE_SET_ATTR_VAR_SUPER:  mnemonic = "ssattrv";  break;
            
        case OPCODE_SEND_MESSAGE:           mnemonic = "send";   goto send;
        case OPCODE_SEND_MESSAGE_VA:        mnemonic = "vsend";  goto send;
        case OPCODE_SEND_MESSAGE_SUPER:     mnemonic = "ssend";  goto send;
        case OPCODE_SEND_MESSAGE_SUPER_VA:  mnemonic = "vssend"; goto send;
 send:
            base = "literal";
            DECODE_UINT(index);
            DECODE_UINT(argumentCount);
            pArgumentCount = &argumentCount;
            literal = literals[index];
            break;
            
        case OPCODE_SEND_MESSAGE_VAR:           mnemonic = "sendv";   goto sendVar;
        case OPCODE_SEND_MESSAGE_VAR_VA:        mnemonic = "vsendv";  goto sendVar;
        case OPCODE_SEND_MESSAGE_SUPER_VAR:     mnemonic = "ssendv";  goto sendVar;
        case OPCODE_SEND_MESSAGE_SUPER_VAR_VA:  mnemonic = "vssendv"; goto sendVar;
 sendVar:
            DECODE_UINT(argumentCount);
            pArgumentCount = &argumentCount;
            break;

        case OPCODE_RAISE: mnemonic = "raise"; break;

        case OPCODE_RET:       mnemonic = "ret";   break;
        case OPCODE_RET_TRAMP: mnemonic = "rett";  break;
            
        case OPCODE_LEAF:
            mnemonic = "leaf";
            break;
            
        case OPCODE_SAVE:
            mnemonic = "save";
            DECODE_UINT(contextSize);
            DECODE_UINT(stackSize);
            break;
            
        case OPCODE_ARG:     mnemonic = "arg";  goto arg;
        case OPCODE_ARG_VA:  mnemonic = "varg"; goto arg;
 arg:
            DECODE_UINT(minArgumentCount);
            DECODE_UINT(maxArgumentCount);
            if (minArgumentCount < maxArgumentCount) {
                for (i = 0; i < maxArgumentCount - minArgumentCount + 1; ++i) {
                    DECODE_SINT(displacement);
                }
            }
            break;
            
        case OPCODE_NATIVE: mnemonic = "ntv"; break;
            
        case OPCODE_RESTORE_SENDER: mnemonic = "restore"; keyword = "sender"; break;
        case OPCODE_RESTORE_CALLER: mnemonic = "restore"; keyword = "caller"; break;
        case OPCODE_CALL_BLOCK:     mnemonic = "cb";    break;
            
        case OPCODE_CHECK_STACKP:
            mnemonic = "check";
            base = "stackp";
            DECODE_UINT(index);
            break;
        }
        
        tab(level, out);
        fprintf(out, "%04x", ip - begin);
        for (i = 0; i < 3; ++i, ++ip) {
            if (ip < instructionPointer) {
                fprintf(out, " %02x", *ip);
            } else {
                fprintf(out, "   ");
            }
        }
        
        fprintf(out, "\t%s", mnemonic);
        if (base && pArgumentCount) {
            fprintf(out, "\t%s[%lu] %lu", base,
                    (unsigned long)index, (unsigned long)*pArgumentCount);
        } else if (pCount) {
            fprintf(out, "\t%lu", (unsigned long)*pCount);
        } else if (keyword) {
            fprintf(out, "\t%s", keyword);
        } else if (pArgumentCount && selector) {
            Unknown *s = Host_ObjectAsString(selector);
            fprintf(out, "\t%s %lu", Host_StringAsCString(s),
                    (unsigned long)*pArgumentCount);
        } else if (pArgumentCount) {
            fprintf(out, "\t%lu", (unsigned long)*pArgumentCount);
        } else if (selector) {
            Unknown *s = Host_ObjectAsString(selector);
            fprintf(out, "\t%s", Host_StringAsCString(s));
        } else if (pLabel) {
            fprintf(out, "\t%04lx", (unsigned long)*pLabel);
        } else if (base) {
            fprintf(out, "\t%s[%lu]", base, (unsigned long)index);
        } else {
            switch (opcode) {
            case OPCODE_SAVE:
                fprintf(out, "\t%lu,%lu",
                        (unsigned long)contextSize,
                        (unsigned long)stackSize);
                break;
            case OPCODE_ARG:
            case OPCODE_ARG_VA:
                fprintf(out, "\t%lu,%lu",
                        (unsigned long)minArgumentCount,
                        (unsigned long)maxArgumentCount);
                break;
            }
        }
        if (literal) {
            fprintf(out, "\t\t; ");
            Host_PrintObject(literal, out);
        }
        fprintf(out, "\n");
    }
    
    fprintf(out, "\n");
}

static void disassemblePreClass(ClassTmpl *aClass,
                                Level *level, void *closure)
{
    FILE *out = (FILE *)closure;
    tab(level->outer, out);
    fprintf(out, "class %s\n", level->name);
}

static void disassemblePreMethodNamespace(MethodNamespace ns,
                                          Level *level,
                                          void *closure)
{
    FILE *out;
    const char *name;
    
    out = (FILE *)closure;
    switch (ns) {
    case METHOD_NAMESPACE_RVALUE:
        name = "rvalue";
        break;
    case METHOD_NAMESPACE_LVALUE:
        name = "lvalue";
        break;
    case NUM_METHOD_NAMESPACES: break;
    }
    
    tab(level->outer, out);
    fprintf(out, "%s\n", name);
}

static void disassemblePreClassBody(BehaviorTmpl *aClass,
                                    Level *level, void *closure)
{
    FILE *out = (FILE *)closure;
    tab(level->outer, out);
    fprintf(out, "%s\n", level->name);
}

static void disassemblePostClass(ClassTmpl *aClass,
                                 Level *level, void *closure)
{
    FILE *out = (FILE *)closure;
    fprintf(out, "\n");
}

static void disassemblePreModule(ModuleTmpl *module,
                                 Level *level, void *closure)
{
    FILE *out = (FILE *)closure;
    size_t i;
    
    fprintf(out, "literal table\n");
    for (i = 0; i < module->literalCount; ++i) {
        fprintf(out, "    %04lu: ", (unsigned long)i);
        Host_PrintObject(module->literals[i], out);
        fprintf(out, "\n");
    }
    fprintf(out, "\n");
}

static Visitor disassembler = {
    0,
    &disassembleMethod,
    &disassemblePreMethodNamespace,
    0,
    &disassemblePreClassBody,
    0,
    &disassemblePreClass,
    &disassemblePostClass,
    &disassemblePreModule,
    0
};


/*------------------------------------------------------------------------*/
/* traversal */

static void traverseClass(ClassTmpl *, Level *,
                          ModuleTmpl *, Visitor *, void *);

static void traverseMethod(MethodTmpl *method,
                           Level *level,
                           ModuleTmpl *module,
                           Visitor *visitor,
                           void *closure)
{
    ClassTmpl *nestedClass;
    MethodTmpl *nestedMethod;
    Level nestedMethodLevel;
    
    if (visitor->preMethod)
        (*visitor->preMethod)(method, module->literals, level, closure);
    
    for (nestedClass = method->nestedClassList.first;
         nestedClass;
         nestedClass = nestedClass->nextInScope)
    {
        traverseClass(nestedClass, level, module, visitor, closure);
    }
    
    nestedMethodLevel.outer = level;
    nestedMethodLevel.name = "<method>";
    
    for (nestedMethod = method->nestedMethodList.first;
         nestedMethod;
         nestedMethod = nestedMethod->nextInScope)
    {
        traverseMethod(nestedMethod, &nestedMethodLevel,
                       module, visitor, closure);
    }
    
    if (visitor->postMethod)
        (*visitor->postMethod)(method, module->literals, level, closure);
}

static void traverseMethodList(MethodTmplList *methodList,
                               MethodNamespace ns,
                               Level *outer,
                               ModuleTmpl *module,
                               Visitor *visitor,
                               void *closure)
{
    Unknown *methodName;
    Level namespaceLevel, methodLevel;
    MethodTmpl *method;
    
    namespaceLevel.outer = outer;
    namespaceLevel.name = 0;
    switch (ns) {
    case METHOD_NAMESPACE_RVALUE:
        namespaceLevel.name = "rv";
        break;
    case METHOD_NAMESPACE_LVALUE:
        namespaceLevel.name = "lv";
        break;
    case NUM_METHOD_NAMESPACES: break;
    }
    
    if (visitor->preMethodNamespace)
        (*visitor->preMethodNamespace)(ns, &namespaceLevel, closure);
    
    methodLevel.outer = &namespaceLevel;
    
    for (method = methodList->first; method; method = method->nextInScope) {
        if (method->ns != ns)
            continue;
        
        methodName = Host_ObjectAsString(method->selector);
        /* XXX: Host_SelectorAsMangledName ? */
        methodLevel.name = Host_StringAsCString(methodName);
        if (*methodLevel.name == '$')
            ++methodLevel.name;
        
        traverseMethod(method, &methodLevel,
                       module, visitor, closure);
    }
    
    if (visitor->postMethodNamespace)
        (*visitor->postMethodNamespace)(ns, &namespaceLevel, closure);
}

static void traverseClassBody(BehaviorTmpl *aClass, const char *name,
                              Level *outer,
                              ModuleTmpl *module, Visitor *visitor, void *closure)
{
    ClassTmpl *nestedClass;
    MethodNamespace ns;
    Level level;
    
    level.outer = outer;
    level.name = name;
    
    if (visitor->preClassBody)
        (*visitor->preClassBody)(aClass, &level, closure);
    
    for (nestedClass = aClass->nestedClassList.first;
         nestedClass;
         nestedClass = nestedClass->nextInScope)
    {
        traverseClass(nestedClass, &level, module, visitor, closure);
    }
    
    for (ns = 0; ns < NUM_METHOD_NAMESPACES; ++ns) {
        traverseMethodList(&aClass->methodList, ns, &level,
                           module, visitor, closure);
    }
    
    if (visitor->postClassBody)
        (*visitor->postClassBody)(aClass, &level, closure);
}

static void traverseClass(ClassTmpl *aClass, Level *outer,
                          ModuleTmpl *module, Visitor *visitor, void *closure)
{
    Level level;
    
    level.outer = outer;
    level.name = aClass->name;
    
    if (visitor->preClass)
        (*visitor->preClass)(aClass, &level, closure);
    
    traverseClassBody(&aClass->thisClass, "instance", &level, module, visitor, closure);
    traverseClassBody(&aClass->metaclass, "class", &level, module, visitor, closure);
    
    if (visitor->postClass)
        (*visitor->postClass)(aClass, &level, closure);
}

static void traverseModule(ModuleTmpl *module,
                           Visitor *visitor, void *closure)
{
    ClassTmpl *nestedClass;
    MethodNamespace ns;
    Level level;
    
    level.outer = 0;
    level.name = "module";
    
    if (visitor->preModule)
        (*visitor->preModule)(module, &level, closure);
    
    for (nestedClass = module->moduleClass.thisClass.nestedClassList.first;
         nestedClass;
         nestedClass = nestedClass->nextInScope)
    {
        traverseClass(nestedClass, &level, module, visitor, closure);
    }
    
    for (ns = 0; ns < NUM_METHOD_NAMESPACES; ++ns) {
        traverseMethodList(&module->moduleClass.thisClass.methodList,
                           ns,
                           &level,
                           module,
                           visitor,
                           closure);
    }
    
    if (visitor->postModule)
        (*visitor->postModule)(module, &level, closure);
}


/*------------------------------------------------------------------------*/
/* C code generation */

static const char *opcodeName(Opcode opcode) {
    switch (opcode) {
    case OPCODE_NOP: return "OPCODE_NOP";
    case OPCODE_PUSH_LOCAL: return "OPCODE_PUSH_LOCAL";
    case OPCODE_PUSH_INST_VAR: return "OPCODE_PUSH_INST_VAR";
    case OPCODE_PUSH_GLOBAL: return "OPCODE_PUSH_GLOBAL";
    case OPCODE_PUSH_LITERAL: return "OPCODE_PUSH_LITERAL";
    case OPCODE_PUSH_SELF: return "OPCODE_PUSH_SELF";
    case OPCODE_PUSH_SUPER: return "OPCODE_PUSH_SUPER";
    case OPCODE_PUSH_FALSE: return "OPCODE_PUSH_FALSE";
    case OPCODE_PUSH_TRUE: return "OPCODE_PUSH_TRUE";
    case OPCODE_PUSH_NULL: return "OPCODE_PUSH_NULL";
    case OPCODE_PUSH_VOID: return "OPCODE_PUSH_VOID";
    case OPCODE_PUSH_CONTEXT: return "OPCODE_PUSH_CONTEXT";
    case OPCODE_DUP: return "OPCODE_DUP";
    case OPCODE_DUP_N: return "OPCODE_DUP_N";
    case OPCODE_STORE_LOCAL: return "OPCODE_STORE_LOCAL";
    case OPCODE_STORE_INST_VAR: return "OPCODE_STORE_INST_VAR";
    case OPCODE_STORE_GLOBAL: return "OPCODE_STORE_GLOBAL";
    case OPCODE_POP: return "OPCODE_POP";
    case OPCODE_ROT: return "OPCODE_ROT";
    case OPCODE_BRANCH_IF_FALSE: return "OPCODE_BRANCH_IF_FALSE";
    case OPCODE_BRANCH_IF_TRUE: return "OPCODE_BRANCH_IF_TRUE";
    case OPCODE_BRANCH_ALWAYS: return "OPCODE_BRANCH_ALWAYS";
    case OPCODE_ID: return "OPCODE_ID";
    case OPCODE_OPER: return "OPCODE_OPER";
    case OPCODE_OPER_SUPER: return "OPCODE_OPER_SUPER";
    case OPCODE_CALL: return "OPCODE_CALL";
    case OPCODE_CALL_VA: return "OPCODE_CALL_VA";
    case OPCODE_CALL_SUPER: return "OPCODE_CALL_SUPER";
    case OPCODE_CALL_SUPER_VA: return "OPCODE_CALL_SUPER_VA";
    case OPCODE_SET_IND: return "OPCODE_SET_IND";
    case OPCODE_SET_IND_SUPER: return "OPCODE_SET_IND_SUPER";
    case OPCODE_SET_INDEX: return "OPCODE_SET_INDEX";
    case OPCODE_SET_INDEX_VA: return "OPCODE_SET_INDEX_VA";
    case OPCODE_SET_INDEX_SUPER: return "OPCODE_SET_INDEX_SUPER";
    case OPCODE_SET_INDEX_SUPER_VA: return "OPCODE_SET_INDEX_SUPER_VA";
    case OPCODE_GET_ATTR: return "OPCODE_GET_ATTR";
    case OPCODE_GET_ATTR_SUPER: return "OPCODE_GET_ATTR_SUPER";
    case OPCODE_GET_ATTR_VAR: return "OPCODE_GET_ATTR_VAR";
    case OPCODE_GET_ATTR_VAR_SUPER: return "OPCODE_GET_ATTR_VAR_SUPER";
    case OPCODE_SET_ATTR: return "OPCODE_SET_ATTR";
    case OPCODE_SET_ATTR_SUPER: return "OPCODE_SET_ATTR_SUPER";
    case OPCODE_SET_ATTR_VAR: return "OPCODE_SET_ATTR_VAR";
    case OPCODE_SET_ATTR_VAR_SUPER: return "OPCODE_SET_ATTR_VAR_SUPER";
    case OPCODE_SEND_MESSAGE: return "OPCODE_SEND_MESSAGE";
    case OPCODE_SEND_MESSAGE_SUPER: return "OPCODE_SEND_MESSAGE_SUPER";
    case OPCODE_SEND_MESSAGE_VAR: return "OPCODE_SEND_MESSAGE_VAR";
    case OPCODE_SEND_MESSAGE_VAR_VA: return "OPCODE_SEND_MESSAGE_VAR_VA";
    case OPCODE_SEND_MESSAGE_SUPER_VAR: return "OPCODE_SEND_MESSAGE_SUPER_VAR";
    case OPCODE_SEND_MESSAGE_SUPER_VAR_VA: return "OPCODE_SEND_MESSAGE_SUPER_VAR_VA";
    case OPCODE_SEND_MESSAGE_NS_VAR_VA: return "OPCODE_SEND_MESSAGE_NS_VAR_VA";
    case OPCODE_SEND_MESSAGE_SUPER_NS_VAR_VA: return "OPCODE_SEND_MESSAGE_SUPER_NS_VAR_VA";
    case OPCODE_RAISE: return "OPCODE_RAISE";
    case OPCODE_RET: return "OPCODE_RET";
    case OPCODE_RET_TRAMP: return "OPCODE_RET_TRAMP";
    case OPCODE_LEAF: return "OPCODE_LEAF";
    case OPCODE_SAVE: return "OPCODE_SAVE";
    case OPCODE_ARG: return "OPCODE_ARG";
    case OPCODE_ARG_VA: return "OPCODE_ARG_VA";
    case OPCODE_NATIVE: return "OPCODE_NATIVE";
    case OPCODE_NATIVE_PUSH_INST_VAR: return "OPCODE_NATIVE_PUSH_INST_VAR";
    case OPCODE_NATIVE_STORE_INST_VAR: return "OPCODE_NATIVE_STORE_INST_VAR";
    case OPCODE_RESTORE_SENDER: return "OPCODE_RESTORE_SENDER";
    case OPCODE_RESTORE_CALLER: return "OPCODE_RESTORE_CALLER";
    case OPCODE_CALL_BLOCK: return "OPCODE_CALL_BLOCK";
    case OPCODE_CHECK_STACKP: return "OPCODE_CHECK_STACKP";
    
    case NUM_OPCODES: break;
    }
    return 0;
}

static const char *operName(Oper oper) {
    switch (oper) {
    case OPER_SUCC: return "OPER_SUCC";
    case OPER_PRED: return "OPER_PRED";
    case OPER_ADDR: return "OPER_ADDR";
    case OPER_IND: return "OPER_IND";
    case OPER_POS: return "OPER_POS";
    case OPER_NEG: return "OPER_NEG";
    case OPER_BNEG: return "OPER_BNEG";
    case OPER_LNEG: return "OPER_LNEG";
    case OPER_MUL: return "OPER_MUL";
    case OPER_DIV: return "OPER_DIV";
    case OPER_MOD: return "OPER_MOD";
    case OPER_ADD: return "OPER_ADD";
    case OPER_SUB: return "OPER_SUB";
    case OPER_LSHIFT: return "OPER_LSHIFT";
    case OPER_RSHIFT: return "OPER_RSHIFT";
    case OPER_LT: return "OPER_LT";
    case OPER_GT: return "OPER_GT";
    case OPER_LE: return "OPER_LE";
    case OPER_GE: return "OPER_GE";
    case OPER_EQ: return "OPER_EQ";
    case OPER_NE: return "OPER_NE";
    case OPER_BAND: return "OPER_BAND";
    case OPER_BXOR: return "OPER_BXOR";
    case OPER_BOR: return "OPER_BOR";
    
    case NUM_OPER: break;
    }
    return 0;
}

static const char *callOperName(CallOper oper) {
    switch (oper) {
    case OPER_APPLY: return "OPER_APPLY";
    case OPER_INDEX: return "OPER_INDEX";
    
    case NUM_CALL_OPER: break;
    }
    return 0;
}

static void genCCodeForMethodOpcodes(MethodTmpl *method,
                                     Unknown **literals,
                                     Level *level,
                                     void *closure)
{
    Level *outer;
    Opcode *begin, *ip, *instructionPointer, *end;
    FILE *out;
    
    out = (FILE *)closure;
    
    outer = level->outer;
    if (0) spaces(outer, out);
    fprintf(out, "static Opcode ");
    nest(level, out);
    fprintf(out, "code[] = {\n");
    
    begin = method->bytecode;
    end = begin + method->bytecodeSize;
    instructionPointer = begin;
    
    while (instructionPointer < end) {
        Opcode opcode;
        size_t index = 0;
        ptrdiff_t displacement = 0;
        size_t contextSize = 0;
        size_t argumentCount = 0, minArgumentCount = 0, maxArgumentCount = 0;
        size_t stackSize = 0;
        size_t count = 0;
        Oper oper = 0;
        CallOper callOperator = 0;
        Unknown *literal = 0;
        size_t i;
        
        if (0) spaces(level, out); else fprintf(out, "    ");
        
        opcode = *instructionPointer++;
        fprintf(out, "%s, ", opcodeName(opcode));
        ip = instructionPointer;
        
        switch (opcode) {
        
        case OPCODE_NOP:
            break;
        
        case OPCODE_PUSH_LOCAL:
        case OPCODE_PUSH_INST_VAR:
        case OPCODE_PUSH_GLOBAL:
            DECODE_UINT(index);
            break;
        
        case OPCODE_PUSH_LITERAL:
            DECODE_UINT(index);
            literal = literals[index];
            break;
            
        case OPCODE_PUSH_SELF:
        case OPCODE_PUSH_SUPER:
        case OPCODE_PUSH_FALSE:
        case OPCODE_PUSH_TRUE:
        case OPCODE_PUSH_NULL:
        case OPCODE_PUSH_VOID:
        case OPCODE_PUSH_CONTEXT:
            break;
            
        case OPCODE_DUP_N:
            DECODE_UINT(count);
            break;
        case OPCODE_DUP:
            break;

        case OPCODE_STORE_LOCAL:
        case OPCODE_STORE_INST_VAR:
        case OPCODE_STORE_GLOBAL:
            DECODE_UINT(index);
            break;
            
        case OPCODE_POP:
            break;
            
        case OPCODE_ROT:
            DECODE_UINT(count);
            break;

        case OPCODE_BRANCH_IF_FALSE:
        case OPCODE_BRANCH_IF_TRUE:
        case OPCODE_BRANCH_ALWAYS:
            DECODE_SINT(displacement);
            break;
            
        case OPCODE_ID:
            break;
            
        case OPCODE_OPER:
        case OPCODE_OPER_SUPER:
            oper = (Oper)(*instructionPointer++);
            fprintf(out, "%s, ", operName(oper));
            ip = instructionPointer;
            break;
            
        case OPCODE_CALL:
        case OPCODE_CALL_VA:
        case OPCODE_CALL_SUPER:
        case OPCODE_CALL_SUPER_VA:
            callOperator = (CallOper)(*instructionPointer++);
            fprintf(out, "%s, ", callOperName(callOperator));
            ip = instructionPointer;
            DECODE_UINT(argumentCount);
            break;

        case OPCODE_SET_IND:
        case OPCODE_SET_IND_SUPER:
            break;
        
        case OPCODE_SET_INDEX:
        case OPCODE_SET_INDEX_VA:
        case OPCODE_SET_INDEX_SUPER:
        case OPCODE_SET_INDEX_SUPER_VA:
            ip = instructionPointer;
            DECODE_UINT(argumentCount);
            break;

        case OPCODE_GET_ATTR:
        case OPCODE_GET_ATTR_SUPER:
        case OPCODE_SET_ATTR:
        case OPCODE_SET_ATTR_SUPER:
            DECODE_UINT(index);
            literal = literals[index];
            break;
            
        case OPCODE_GET_ATTR_VAR:
        case OPCODE_GET_ATTR_VAR_SUPER:
        case OPCODE_SET_ATTR_VAR:
        case OPCODE_SET_ATTR_VAR_SUPER:
            break;
            
        case OPCODE_SEND_MESSAGE:
        case OPCODE_SEND_MESSAGE_VA:
        case OPCODE_SEND_MESSAGE_SUPER:
        case OPCODE_SEND_MESSAGE_SUPER_VA:
            DECODE_UINT(index);
            DECODE_UINT(argumentCount);
            literal = literals[index];
            break;
            
        case OPCODE_SEND_MESSAGE_VAR:
        case OPCODE_SEND_MESSAGE_VAR_VA:
        case OPCODE_SEND_MESSAGE_SUPER_VAR:
        case OPCODE_SEND_MESSAGE_SUPER_VAR_VA:
            DECODE_UINT(argumentCount);
            break;
            
        case OPCODE_RAISE:
            break;

        case OPCODE_RET:
        case OPCODE_RET_TRAMP:
            break;
            
        case OPCODE_LEAF:
            break;
            
        case OPCODE_SAVE:
            DECODE_UINT(contextSize);
            DECODE_UINT(stackSize);
            break;
            
        case OPCODE_ARG:
        case OPCODE_ARG_VA:
            DECODE_UINT(minArgumentCount);
            DECODE_UINT(maxArgumentCount);
            if (minArgumentCount < maxArgumentCount) {
                for (i = 0; i < maxArgumentCount - minArgumentCount + 1; ++i) {
                    DECODE_SINT(displacement);
                }
            }
            break;
            
        case OPCODE_NATIVE:
            break;
            
        case OPCODE_RESTORE_SENDER:
        case OPCODE_RESTORE_CALLER:
        case OPCODE_CALL_BLOCK:
            break;
            
        case OPCODE_CHECK_STACKP:
            DECODE_UINT(index);
            break;
        }
        
        for ( ; ip < instructionPointer; ++ip) {
            fprintf(out, "0x%02x, ", *ip);
        }
        fprintf(out, "\n");
    }
    
    if (0) spaces(level, out); else fprintf(out, "    ");
    fprintf(out, "%s\n", opcodeName(OPCODE_NOP));
    
    if (0) spaces(outer, out);
    fprintf(out, "};\n\n");
}

static void genCCodePreMethodNamespace(MethodNamespace ns,
                                       Level *level,
                                       void *closure)
{
    FILE *out = (FILE *)closure;
    fprintf(out, "static MethodTmpl ");
    nest(level, out);
    fprintf(out, "methods[] = {\n");
}

static void genCCodeForMethodTableEntry(MethodTmpl *method,
                                        Unknown **literals,
                                        Level *level,
                                        void *closure)
{
    FILE *out = (FILE *)closure;
    
    fprintf(out, "    { \"%s\", 0, 0, sizeof(", level->name);
    nest(level, out);
    fprintf(out, "code), ");
    nest(level, out);
    fprintf(out, "code },\n");
}

static void genCCodePostMethodNamespace(MethodNamespace ns,
                                        Level *level,
                                        void *closure)
{
    FILE *out = (FILE *)closure;
    
    fprintf(out, "    { 0 }\n};\n\n");
}

static void genCCodeClassTemplate(ClassTmpl *aClass,
                                  Level *level, void *closure)
{
    FILE *out = (FILE *)closure;
    const char *superclassName;
    
    fprintf(out,
            "ClassTmpl ");
    if (1) {
        /* XXX: For now, we are only interested in generating code for
           Spike itself. */
        superclassName = Host_SymbolAsCString(aClass->superclassName);
        fprintf(out, "Class%sTmpl = {\n"
                "    HEART_CLASS_TMPL(%s, %s), {\n",
                level->name, level->name, superclassName);
    } else {
        nest(level, out);
        fprintf(out, "classTmpl = {\n"
                "    \"%s\", 0, 0, {\n", /* XXX */
                level->name);
    }
    fprintf(out,
            "        /*accessors*/ 0,\n"
            "        ");
    nest(level, out);
    fprintf(out, "instance_rv_methods,\n"
            "        ");
    nest(level, out);
    fprintf(out, "instance_lv_methods\n"
            "    }, /*meta*/ {\n"
            "        /*accessors*/ 0,\n"
            "        ");
    nest(level, out);
    fprintf(out, "class_rv_methods,\n"
            "        ");
    nest(level, out);
    fprintf(out, "class_lv_methods\n"
            "    }\n"
            "};\n\n"
            );
}

static void genCCodeLiteralTable(ModuleTmpl *module,
                                 Level *level, void *closure)
{
    FILE *out = (FILE *)closure;
    size_t i;
    Unknown *literal;
    Behavior *klass;
    
    fprintf(out, "static LiteralTmpl ");
    nest(level, out);
    fprintf(out, "literals[] = {\n");
    for (i = 0; i < module->literalCount; ++i) {
        literal = module->literals[i];
        fprintf(out, "    { ");
        /* XXX: It would be nice if there separate tables -- separate
           "data segments" -- for each type of literal. */
        klass = ((Object *)literal)->klass;
        if (klass == CLASS(Symbol)) {
            fprintf(out, "LITERAL_SYMBOL, 0, 0.0, \"%s\"", Host_SymbolAsCString(literal));
        } else if (klass == CLASS(Integer)) {
            fprintf(out, "LITERAL_INTEGER, %ld, 0.0, 0", Host_IntegerAsCLong(literal));
        } else if (klass == CLASS(Float)) {
            fprintf(out, "LITERAL_FLOAT, 0, %g, 0", Host_FloatAsCDouble(literal));
        } else if (klass == CLASS(Char)) {
            fprintf(out, "LITERAL_CHAR, '%c', 0.0, 0", Host_CharAsCChar(literal));
        } else if (klass == CLASS(String)) {
            fprintf(out, "LITERAL_STRING, 0, 0.0, \"%s\"", Host_StringAsCString(literal));
        } else if (1) {
            /* XXX: predefined names are (mis)placed in this table */
            fprintf(out, "LITERAL_INTEGER, 0");
        } else {
            /* cause a compilation error */
            fprintf(out, "unknownClassOfLiteral");
        }
        fprintf(out, " },\n");
    }
    fprintf(out, "    { 0 }\n};\n\n");
}

static void genCCodeModuleTemplate(ModuleTmpl *module,
                                   Level *level, void *closure)
{
    FILE *out = (FILE *)closure;
    
    fprintf(out,
            "ModuleTmpl Module%sTmpl = {\n"
            "    {\n"
            /* there is no class variable */
            /*"        HEART_CLASS_TMPL(%s, Module), {\n",*/
            "        \"%s\", 0, offsetof(Heart, Module), {\n",
            level->name, level->name);
    fprintf(out,
            "            /*accessors*/ 0,\n"
            "            ");
    nest(level, out);
    fprintf(out, "rv_methods,\n"
            "            ");
    nest(level, out);
    fprintf(out, "lv_methods,\n"
            "            /*instVarOffset*/ 0,\n"
            "            /*itemSize*/ 0,\n"
            "            /*zero*/ 0,\n"
            "            /*dealloc*/ 0,\n"
            "            /*instVarCount*/ %lu\n"
            "        }, /*meta*/ {\n"
            "            /*accessors*/ 0,\n"
            "            /*methods*/ 0,\n"
            "            /*lvalueMethods*/ 0\n"
            "        }\n"
            "    },\n"
            "    /*literalCount*/ %lu,\n"
            "    ",
            (unsigned long)module->moduleClass.thisClass.instVarCount,
            (unsigned long)module->literalCount);
    nest(level, out);
    fprintf(out,
            "literals\n"
            "};\n\n");
}


static Visitor cCodeGenOpcodes = {
    0,
    &genCCodeForMethodOpcodes,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0
};

static Visitor cCodeGenTemplates = {
    0,
    &genCCodeForMethodTableEntry,
    &genCCodePreMethodNamespace,
    &genCCodePostMethodNamespace,
    0,
    0,
    0,
    &genCCodeClassTemplate,
    &genCCodeLiteralTable,
    &genCCodeModuleTemplate
};


/*------------------------------------------------------------------------*/
/* C API */

void Disassembler_DisassembleModule(ModuleTmpl *module, FILE *out) {
    traverseModule(module, &disassembler, (void *)out);
}

void Disassembler_DisassembleModuleAsCCode(ModuleTmpl *module, FILE *out) {
    time_t t;
    const char *timestamp;
    
    time(&t);
    timestamp = asctime(gmtime(&t));
    
    fprintf(out,
            "/* Generated by Spike on %.*s */\n"
            "\n"
            "#include \"class.h\"\n"
            "#include \"heart.h\"\n"
            "\n",
            (int)(strlen(timestamp) - 1), /* strip newline */
            timestamp);
    traverseModule(module, &cCodeGenOpcodes, (void *)out);
    traverseModule(module, &cCodeGenTemplates, (void *)out);
}
