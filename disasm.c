
#include "disasm.h"

#include "cgen.h"
#include "heart.h"
#include "host.h"
#include "module.h"
#include "rodata.h"

#include <string.h>
#include <time.h>


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


typedef struct Level {
    struct Level *outer;
    const char *name;
} Level;

typedef struct Visitor {
    void (*preMethod)(SpkMethodTmpl *, SpkUnknown **, Level *,
                      void *);
    void (*postMethod)(SpkMethodTmpl *, SpkUnknown **, Level *,
                       void *);
    void (*preMethodNamespace)(SpkMethodNamespace, Level *, void *);
    void (*postMethodNamespace)(SpkMethodNamespace, Level *, void *);
    void (*preClassBody)(SpkBehaviorTmpl *, Level *, void *);
    void (*postClassBody)(SpkBehaviorTmpl *, Level *, void *);
    void (*preClass)(SpkClassTmpl *, Level *, void *);
    void (*postClass)(SpkClassTmpl *, Level *, void *);
    void (*preModule)(SpkModuleTmpl *, Level *, void *);
    void (*postModule)(SpkModuleTmpl *, Level *, void *);
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

static void disassembleMethod(SpkMethodTmpl *method,
                              SpkUnknown **literals,
                              Level *level,
                              void *closure)
{
    SpkOpcode *begin, *ip, *instructionPointer, *end;
    FILE *out;
    
    out = (FILE *)closure;
    
    tab(level->outer, out);
    fprintf(out, "%s\n", level->name);
    
    begin = method->bytecode;
    end = begin + method->bytecodeSize;
    instructionPointer = begin;
    
    while (instructionPointer < end) {
        SpkOpcode opcode;
        const char *keyword = 0;
        SpkUnknown *selector = 0;
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
        SpkUnknown *literal = 0;
        
        ip = instructionPointer;
        opcode = *instructionPointer++;
        
        switch (opcode) {
            
        case Spk_OPCODE_NOP: mnemonic = "nop"; break;
            
        case Spk_OPCODE_PUSH_LOCAL:    mnemonic = "push"; base = "local";    goto push;
        case Spk_OPCODE_PUSH_INST_VAR: mnemonic = "push"; base = "receiver"; goto push;
        case Spk_OPCODE_PUSH_GLOBAL:   mnemonic = "push"; base = "global";   goto push;
 push:
            DECODE_UINT(index);
            break;
        case Spk_OPCODE_PUSH_LITERAL:
            mnemonic = "push";
            base = "literal";
            DECODE_UINT(index);
            literal = literals[index];
            break;
            
        case Spk_OPCODE_PUSH_SELF:     mnemonic = "push"; keyword = "self";        break;
        case Spk_OPCODE_PUSH_SUPER:    mnemonic = "push"; keyword = "super";       break;
        case Spk_OPCODE_PUSH_FALSE:    mnemonic = "push"; keyword = "false";       break;
        case Spk_OPCODE_PUSH_TRUE:     mnemonic = "push"; keyword = "true";        break;
        case Spk_OPCODE_PUSH_NULL:     mnemonic = "push"; keyword = "null";        break;
        case Spk_OPCODE_PUSH_VOID:     mnemonic = "push"; keyword = "void";        break;
        case Spk_OPCODE_PUSH_CONTEXT:  mnemonic = "push"; keyword = "thisContext"; break;
            
        case Spk_OPCODE_DUP_N:
            DECODE_UINT(count);
            pCount = &count;
        case Spk_OPCODE_DUP:
            mnemonic = "dup";
            break;

        case Spk_OPCODE_STORE_LOCAL:    mnemonic = "store"; base = "local";    goto store;
        case Spk_OPCODE_STORE_INST_VAR: mnemonic = "store"; base = "receiver"; goto store;
        case Spk_OPCODE_STORE_GLOBAL:   mnemonic = "store"; base = "global";   goto store;
 store:
            DECODE_UINT(index);
            break;
            
        case Spk_OPCODE_POP: mnemonic = "pop"; break;
            
        case Spk_OPCODE_ROT:
            mnemonic = "rot";
            DECODE_UINT(count);
            pCount = &count;
            break;

        case Spk_OPCODE_BRANCH_IF_FALSE: mnemonic = "brf"; goto branch;
        case Spk_OPCODE_BRANCH_IF_TRUE:  mnemonic = "brt"; goto branch;
        case Spk_OPCODE_BRANCH_ALWAYS:   mnemonic = "bra"; goto branch;
 branch:
            DECODE_SINT(displacement);
            label = (ip + displacement) - begin;
            pLabel = &label;
            break;
            
        case Spk_OPCODE_ID:         mnemonic = "id";    break;
            
        case Spk_OPCODE_OPER:       mnemonic = "oper";  goto oper;
        case Spk_OPCODE_OPER_SUPER: mnemonic = "soper"; goto oper;
 oper:
            oper = (unsigned int)(*instructionPointer++);
            selector = *Spk_operSelectors[oper].selector;
            break;
            
        case Spk_OPCODE_CALL:           mnemonic = "call";   goto call;
        case Spk_OPCODE_CALL_VA:        mnemonic = "vcall";  goto call;
        case Spk_OPCODE_CALL_SUPER:     mnemonic = "scall";  goto call;
        case Spk_OPCODE_CALL_SUPER_VA:  mnemonic = "vscall"; goto call;
 call:
            oper = (unsigned int)(*instructionPointer++);
            selector = *Spk_operCallSelectors[oper].selector;
            DECODE_UINT(argumentCount);
            pArgumentCount = &argumentCount;
            break;

        case Spk_OPCODE_SET_IND:        mnemonic = "ind";  break;
        case Spk_OPCODE_SET_IND_SUPER:  mnemonic = "sind"; break;
        
        case Spk_OPCODE_SET_INDEX:           mnemonic = "index";   goto index;
        case Spk_OPCODE_SET_INDEX_VA:        mnemonic = "vindex";  goto index;
        case Spk_OPCODE_SET_INDEX_SUPER:     mnemonic = "sindex";  goto index;
        case Spk_OPCODE_SET_INDEX_SUPER_VA:  mnemonic = "vsindex"; goto index;
 index:
            DECODE_UINT(argumentCount);
            pArgumentCount = &argumentCount;
            break;

        case Spk_OPCODE_GET_ATTR:       mnemonic = "gattr";  goto attr;
        case Spk_OPCODE_GET_ATTR_SUPER: mnemonic = "gsattr"; goto attr;
        case Spk_OPCODE_SET_ATTR:       mnemonic = "sattr";  goto attr;
        case Spk_OPCODE_SET_ATTR_SUPER: mnemonic = "ssattr"; goto attr;
 attr:
            base = "literal";
            DECODE_UINT(index);
            literal = literals[index];
            break;
            
        case Spk_OPCODE_GET_ATTR_VAR:        mnemonic = "gattrv";   break;
        case Spk_OPCODE_GET_ATTR_VAR_SUPER:  mnemonic = "gsattrv";  break;
        case Spk_OPCODE_SET_ATTR_VAR:        mnemonic = "sattrv";   break;
        case Spk_OPCODE_SET_ATTR_VAR_SUPER:  mnemonic = "ssattrv";  break;
            
        case Spk_OPCODE_SEND_MESSAGE:           mnemonic = "send";   goto send;
        case Spk_OPCODE_SEND_MESSAGE_VA:        mnemonic = "vsend";  goto send;
        case Spk_OPCODE_SEND_MESSAGE_SUPER:     mnemonic = "ssend";  goto send;
        case Spk_OPCODE_SEND_MESSAGE_SUPER_VA:  mnemonic = "vssend"; goto send;
 send:
            base = "literal";
            DECODE_UINT(index);
            DECODE_UINT(argumentCount);
            pArgumentCount = &argumentCount;
            literal = literals[index];
            break;
            
        case Spk_OPCODE_SEND_MESSAGE_VAR:           mnemonic = "sendv";   goto sendVar;
        case Spk_OPCODE_SEND_MESSAGE_VAR_VA:        mnemonic = "vsendv";  goto sendVar;
        case Spk_OPCODE_SEND_MESSAGE_SUPER_VAR:     mnemonic = "ssendv";  goto sendVar;
        case Spk_OPCODE_SEND_MESSAGE_SUPER_VAR_VA:  mnemonic = "vssendv"; goto sendVar;
 sendVar:
            DECODE_UINT(argumentCount);
            pArgumentCount = &argumentCount;
            break;

        case Spk_OPCODE_RAISE: mnemonic = "raise"; break;

        case Spk_OPCODE_RET:       mnemonic = "ret";   break;
        case Spk_OPCODE_RET_TRAMP: mnemonic = "rett";  break;
            
        case Spk_OPCODE_LEAF:
            mnemonic = "leaf";
            break;
            
        case Spk_OPCODE_SAVE:
            mnemonic = "save";
            DECODE_UINT(contextSize);
            DECODE_UINT(stackSize);
            break;
            
        case Spk_OPCODE_ARG:     mnemonic = "arg";  goto arg;
        case Spk_OPCODE_ARG_VA:  mnemonic = "varg"; goto arg;
 arg:
            DECODE_UINT(minArgumentCount);
            DECODE_UINT(maxArgumentCount);
            if (minArgumentCount < maxArgumentCount) {
                for (i = 0; i < maxArgumentCount - minArgumentCount + 1; ++i) {
                    DECODE_SINT(displacement);
                }
            }
            break;
            
        case Spk_OPCODE_NATIVE: mnemonic = "ntv"; break;
            
        case Spk_OPCODE_RESTORE_SENDER: mnemonic = "restore"; keyword = "sender"; break;
        case Spk_OPCODE_RESTORE_CALLER: mnemonic = "restore"; keyword = "caller"; break;
        case Spk_OPCODE_CALL_BLOCK:     mnemonic = "cb";    break;
            
        case Spk_OPCODE_CHECK_STACKP:
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
            SpkUnknown *s = SpkHost_ObjectAsString(selector);
            fprintf(out, "\t%s %lu", SpkHost_StringAsCString(s),
                    (unsigned long)*pArgumentCount);
            Spk_DECREF(s);
        } else if (pArgumentCount) {
            fprintf(out, "\t%lu", (unsigned long)*pArgumentCount);
        } else if (selector) {
            SpkUnknown *s = SpkHost_ObjectAsString(selector);
            fprintf(out, "\t%s", SpkHost_StringAsCString(s));
            Spk_DECREF(s);
        } else if (pLabel) {
            fprintf(out, "\t%04lx", (unsigned long)*pLabel);
        } else if (base) {
            fprintf(out, "\t%s[%lu]", base, (unsigned long)index);
        } else {
            switch (opcode) {
            case Spk_OPCODE_SAVE:
                fprintf(out, "\t%lu,%lu",
                        (unsigned long)contextSize,
                        (unsigned long)stackSize);
                break;
            case Spk_OPCODE_ARG:
            case Spk_OPCODE_ARG_VA:
                fprintf(out, "\t%lu,%lu",
                        (unsigned long)minArgumentCount,
                        (unsigned long)maxArgumentCount);
                break;
            }
        }
        if (literal) {
            fprintf(out, "\t\t; ");
            SpkHost_PrintObject(literal, out);
        }
        fprintf(out, "\n");
    }
    
    fprintf(out, "\n");
}

static void disassemblePreClass(SpkClassTmpl *aClass,
                                Level *level, void *closure)
{
    FILE *out = (FILE *)closure;
    tab(level->outer, out);
    fprintf(out, "class %s\n", level->name);
}

static void disassemblePreMethodNamespace(SpkMethodNamespace ns,
                                          Level *level,
                                          void *closure)
{
    FILE *out;
    const char *name;
    
    out = (FILE *)closure;
    switch (ns) {
    case Spk_METHOD_NAMESPACE_RVALUE:
        name = "rvalue";
        break;
    case Spk_METHOD_NAMESPACE_LVALUE:
        name = "lvalue";
        break;
    case Spk_NUM_METHOD_NAMESPACES: break;
    }
    
    tab(level->outer, out);
    fprintf(out, "%s\n", name);
}

static void disassemblePreClassBody(SpkBehaviorTmpl *aClass,
                                    Level *level, void *closure)
{
    FILE *out = (FILE *)closure;
    tab(level->outer, out);
    fprintf(out, "%s\n", level->name);
}

static void disassemblePostClass(SpkClassTmpl *aClass,
                                 Level *level, void *closure)
{
    FILE *out = (FILE *)closure;
    fprintf(out, "\n");
}

static void disassemblePreModule(SpkModuleTmpl *module,
                                 Level *level, void *closure)
{
    FILE *out = (FILE *)closure;
    size_t i;
    
    fprintf(out, "literal table\n");
    for (i = 0; i < module->literalCount; ++i) {
        fprintf(out, "    %04lu: ", (unsigned long)i);
        SpkHost_PrintObject(module->literals[i], out);
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

static void traverseClass(SpkClassTmpl *, Level *,
                          SpkModuleTmpl *, Visitor *, void *);

static void traverseMethod(SpkMethodTmpl *method,
                           Level *level,
                           SpkModuleTmpl *module,
                           Visitor *visitor,
                           void *closure)
{
    SpkClassTmpl *nestedClass;
    SpkMethodTmpl *nestedMethod;
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

static void traverseMethodList(SpkMethodTmplList *methodList,
                               SpkMethodNamespace ns,
                               Level *outer,
                               SpkModuleTmpl *module,
                               Visitor *visitor,
                               void *closure)
{
    SpkUnknown *methodName;
    Level namespaceLevel, methodLevel;
    SpkMethodTmpl *method;
    
    namespaceLevel.outer = outer;
    namespaceLevel.name = 0;
    switch (ns) {
    case Spk_METHOD_NAMESPACE_RVALUE:
        namespaceLevel.name = "rv";
        break;
    case Spk_METHOD_NAMESPACE_LVALUE:
        namespaceLevel.name = "lv";
        break;
    case Spk_NUM_METHOD_NAMESPACES: break;
    }
    
    if (visitor->preMethodNamespace)
        (*visitor->preMethodNamespace)(ns, &namespaceLevel, closure);
    
    methodLevel.outer = &namespaceLevel;
    
    for (method = methodList->first; method; method = method->nextInScope) {
        if (method->ns != ns)
            continue;
        
        methodName = SpkHost_ObjectAsString(method->selector);
        /* XXX: SpkHost_SelectorAsMangledName ? */
        methodLevel.name = SpkHost_StringAsCString(methodName);
        if (*methodLevel.name == '$')
            ++methodLevel.name;
        
        traverseMethod(method, &methodLevel,
                       module, visitor, closure);
        
        Spk_DECREF(methodName);
    }
    
    if (visitor->postMethodNamespace)
        (*visitor->postMethodNamespace)(ns, &namespaceLevel, closure);
}

static void traverseClassBody(SpkBehaviorTmpl *aClass, const char *name,
                              Level *outer,
                              SpkModuleTmpl *module, Visitor *visitor, void *closure)
{
    SpkClassTmpl *nestedClass;
    SpkMethodNamespace ns;
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
    
    for (ns = 0; ns < Spk_NUM_METHOD_NAMESPACES; ++ns) {
        traverseMethodList(&aClass->methodList, ns, &level,
                           module, visitor, closure);
    }
    
    if (visitor->postClassBody)
        (*visitor->postClassBody)(aClass, &level, closure);
}

static void traverseClass(SpkClassTmpl *aClass, Level *outer,
                          SpkModuleTmpl *module, Visitor *visitor, void *closure)
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

static void traverseModule(SpkModuleTmpl *module,
                           Visitor *visitor, void *closure)
{
    SpkClassTmpl *nestedClass;
    SpkMethodNamespace ns;
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
    
    for (ns = 0; ns < Spk_NUM_METHOD_NAMESPACES; ++ns) {
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

static const char *opcodeName(SpkOpcode opcode) {
    switch (opcode) {
    case Spk_OPCODE_NOP: return "Spk_OPCODE_NOP";
    case Spk_OPCODE_PUSH_LOCAL: return "Spk_OPCODE_PUSH_LOCAL";
    case Spk_OPCODE_PUSH_INST_VAR: return "Spk_OPCODE_PUSH_INST_VAR";
    case Spk_OPCODE_PUSH_GLOBAL: return "Spk_OPCODE_PUSH_GLOBAL";
    case Spk_OPCODE_PUSH_LITERAL: return "Spk_OPCODE_PUSH_LITERAL";
    case Spk_OPCODE_PUSH_SELF: return "Spk_OPCODE_PUSH_SELF";
    case Spk_OPCODE_PUSH_SUPER: return "Spk_OPCODE_PUSH_SUPER";
    case Spk_OPCODE_PUSH_FALSE: return "Spk_OPCODE_PUSH_FALSE";
    case Spk_OPCODE_PUSH_TRUE: return "Spk_OPCODE_PUSH_TRUE";
    case Spk_OPCODE_PUSH_NULL: return "Spk_OPCODE_PUSH_NULL";
    case Spk_OPCODE_PUSH_VOID: return "Spk_OPCODE_PUSH_VOID";
    case Spk_OPCODE_PUSH_CONTEXT: return "Spk_OPCODE_PUSH_CONTEXT";
    case Spk_OPCODE_DUP: return "Spk_OPCODE_DUP";
    case Spk_OPCODE_DUP_N: return "Spk_OPCODE_DUP_N";
    case Spk_OPCODE_STORE_LOCAL: return "Spk_OPCODE_STORE_LOCAL";
    case Spk_OPCODE_STORE_INST_VAR: return "Spk_OPCODE_STORE_INST_VAR";
    case Spk_OPCODE_STORE_GLOBAL: return "Spk_OPCODE_STORE_GLOBAL";
    case Spk_OPCODE_POP: return "Spk_OPCODE_POP";
    case Spk_OPCODE_ROT: return "Spk_OPCODE_ROT";
    case Spk_OPCODE_BRANCH_IF_FALSE: return "Spk_OPCODE_BRANCH_IF_FALSE";
    case Spk_OPCODE_BRANCH_IF_TRUE: return "Spk_OPCODE_BRANCH_IF_TRUE";
    case Spk_OPCODE_BRANCH_ALWAYS: return "Spk_OPCODE_BRANCH_ALWAYS";
    case Spk_OPCODE_ID: return "Spk_OPCODE_ID";
    case Spk_OPCODE_OPER: return "Spk_OPCODE_OPER";
    case Spk_OPCODE_OPER_SUPER: return "Spk_OPCODE_OPER_SUPER";
    case Spk_OPCODE_CALL: return "Spk_OPCODE_CALL";
    case Spk_OPCODE_CALL_VA: return "Spk_OPCODE_CALL_VA";
    case Spk_OPCODE_CALL_SUPER: return "Spk_OPCODE_CALL_SUPER";
    case Spk_OPCODE_CALL_SUPER_VA: return "Spk_OPCODE_CALL_SUPER_VA";
    case Spk_OPCODE_SET_IND: return "Spk_OPCODE_SET_IND";
    case Spk_OPCODE_SET_IND_SUPER: return "Spk_OPCODE_SET_IND_SUPER";
    case Spk_OPCODE_SET_INDEX: return "Spk_OPCODE_SET_INDEX";
    case Spk_OPCODE_SET_INDEX_VA: return "Spk_OPCODE_SET_INDEX_VA";
    case Spk_OPCODE_SET_INDEX_SUPER: return "Spk_OPCODE_SET_INDEX_SUPER";
    case Spk_OPCODE_SET_INDEX_SUPER_VA: return "Spk_OPCODE_SET_INDEX_SUPER_VA";
    case Spk_OPCODE_GET_ATTR: return "Spk_OPCODE_GET_ATTR";
    case Spk_OPCODE_GET_ATTR_SUPER: return "Spk_OPCODE_GET_ATTR_SUPER";
    case Spk_OPCODE_GET_ATTR_VAR: return "Spk_OPCODE_GET_ATTR_VAR";
    case Spk_OPCODE_GET_ATTR_VAR_SUPER: return "Spk_OPCODE_GET_ATTR_VAR_SUPER";
    case Spk_OPCODE_SET_ATTR: return "Spk_OPCODE_SET_ATTR";
    case Spk_OPCODE_SET_ATTR_SUPER: return "Spk_OPCODE_SET_ATTR_SUPER";
    case Spk_OPCODE_SET_ATTR_VAR: return "Spk_OPCODE_SET_ATTR_VAR";
    case Spk_OPCODE_SET_ATTR_VAR_SUPER: return "Spk_OPCODE_SET_ATTR_VAR_SUPER";
    case Spk_OPCODE_SEND_MESSAGE: return "Spk_OPCODE_SEND_MESSAGE";
    case Spk_OPCODE_SEND_MESSAGE_SUPER: return "Spk_OPCODE_SEND_MESSAGE_SUPER";
    case Spk_OPCODE_SEND_MESSAGE_VAR: return "Spk_OPCODE_SEND_MESSAGE_VAR";
    case Spk_OPCODE_SEND_MESSAGE_VAR_VA: return "Spk_OPCODE_SEND_MESSAGE_VAR_VA";
    case Spk_OPCODE_SEND_MESSAGE_SUPER_VAR: return "Spk_OPCODE_SEND_MESSAGE_SUPER_VAR";
    case Spk_OPCODE_SEND_MESSAGE_SUPER_VAR_VA: return "Spk_OPCODE_SEND_MESSAGE_SUPER_VAR_VA";
    case Spk_OPCODE_SEND_MESSAGE_NS_VAR_VA: return "Spk_OPCODE_SEND_MESSAGE_NS_VAR_VA";
    case Spk_OPCODE_SEND_MESSAGE_SUPER_NS_VAR_VA: return "Spk_OPCODE_SEND_MESSAGE_SUPER_NS_VAR_VA";
    case Spk_OPCODE_RAISE: return "Spk_OPCODE_RAISE";
    case Spk_OPCODE_RET: return "Spk_OPCODE_RET";
    case Spk_OPCODE_RET_TRAMP: return "Spk_OPCODE_RET_TRAMP";
    case Spk_OPCODE_LEAF: return "Spk_OPCODE_LEAF";
    case Spk_OPCODE_SAVE: return "Spk_OPCODE_SAVE";
    case Spk_OPCODE_ARG: return "Spk_OPCODE_ARG";
    case Spk_OPCODE_ARG_VA: return "Spk_OPCODE_ARG_VA";
    case Spk_OPCODE_NATIVE: return "Spk_OPCODE_NATIVE";
    case Spk_OPCODE_NATIVE_PUSH_INST_VAR: return "Spk_OPCODE_NATIVE_PUSH_INST_VAR";
    case Spk_OPCODE_NATIVE_STORE_INST_VAR: return "Spk_OPCODE_NATIVE_STORE_INST_VAR";
    case Spk_OPCODE_RESTORE_SENDER: return "Spk_OPCODE_RESTORE_SENDER";
    case Spk_OPCODE_RESTORE_CALLER: return "Spk_OPCODE_RESTORE_CALLER";
    case Spk_OPCODE_CALL_BLOCK: return "Spk_OPCODE_CALL_BLOCK";
    case Spk_OPCODE_CHECK_STACKP: return "Spk_OPCODE_CHECK_STACKP";
    
    case Spk_NUM_OPCODES: break;
    }
    return 0;
}

static const char *operName(SpkOper oper) {
    switch (oper) {
    case Spk_OPER_SUCC: return "Spk_OPER_SUCC";
    case Spk_OPER_PRED: return "Spk_OPER_PRED";
    case Spk_OPER_ADDR: return "Spk_OPER_ADDR";
    case Spk_OPER_IND: return "Spk_OPER_IND";
    case Spk_OPER_POS: return "Spk_OPER_POS";
    case Spk_OPER_NEG: return "Spk_OPER_NEG";
    case Spk_OPER_BNEG: return "Spk_OPER_BNEG";
    case Spk_OPER_LNEG: return "Spk_OPER_LNEG";
    case Spk_OPER_MUL: return "Spk_OPER_MUL";
    case Spk_OPER_DIV: return "Spk_OPER_DIV";
    case Spk_OPER_MOD: return "Spk_OPER_MOD";
    case Spk_OPER_ADD: return "Spk_OPER_ADD";
    case Spk_OPER_SUB: return "Spk_OPER_SUB";
    case Spk_OPER_LSHIFT: return "Spk_OPER_LSHIFT";
    case Spk_OPER_RSHIFT: return "Spk_OPER_RSHIFT";
    case Spk_OPER_LT: return "Spk_OPER_LT";
    case Spk_OPER_GT: return "Spk_OPER_GT";
    case Spk_OPER_LE: return "Spk_OPER_LE";
    case Spk_OPER_GE: return "Spk_OPER_GE";
    case Spk_OPER_EQ: return "Spk_OPER_EQ";
    case Spk_OPER_NE: return "Spk_OPER_NE";
    case Spk_OPER_BAND: return "Spk_OPER_BAND";
    case Spk_OPER_BXOR: return "Spk_OPER_BXOR";
    case Spk_OPER_BOR: return "Spk_OPER_BOR";
    
    case Spk_NUM_OPER: break;
    }
    return 0;
}

static const char *callOperName(SpkCallOper oper) {
    switch (oper) {
    case Spk_OPER_APPLY: return "Spk_OPER_APPLY";
    case Spk_OPER_INDEX: return "Spk_OPER_INDEX";
    
    case Spk_NUM_CALL_OPER: break;
    }
    return 0;
}

static void genCCodeForMethodOpcodes(SpkMethodTmpl *method,
                                     SpkUnknown **literals,
                                     Level *level,
                                     void *closure)
{
    Level *outer;
    SpkOpcode *begin, *ip, *instructionPointer, *end;
    FILE *out;
    
    out = (FILE *)closure;
    
    outer = level->outer;
    if (0) spaces(outer, out);
    fprintf(out, "static SpkOpcode ");
    nest(level, out);
    fprintf(out, "code[] = {\n");
    
    begin = method->bytecode;
    end = begin + method->bytecodeSize;
    instructionPointer = begin;
    
    while (instructionPointer < end) {
        SpkOpcode opcode;
        size_t index = 0;
        ptrdiff_t displacement = 0;
        size_t contextSize = 0;
        size_t argumentCount = 0, minArgumentCount = 0, maxArgumentCount = 0;
        size_t stackSize = 0;
        size_t count = 0;
        SpkOper oper = 0;
        SpkCallOper callOperator = 0;
        SpkUnknown *literal = 0;
        size_t i;
        
        if (0) spaces(level, out); else fprintf(out, "    ");
        
        opcode = *instructionPointer++;
        fprintf(out, "%s, ", opcodeName(opcode));
        ip = instructionPointer;
        
        switch (opcode) {
        
        case Spk_OPCODE_NOP:
            break;
        
        case Spk_OPCODE_PUSH_LOCAL:
        case Spk_OPCODE_PUSH_INST_VAR:
        case Spk_OPCODE_PUSH_GLOBAL:
            DECODE_UINT(index);
            break;
        
        case Spk_OPCODE_PUSH_LITERAL:
            DECODE_UINT(index);
            literal = literals[index];
            break;
            
        case Spk_OPCODE_PUSH_SELF:
        case Spk_OPCODE_PUSH_SUPER:
        case Spk_OPCODE_PUSH_FALSE:
        case Spk_OPCODE_PUSH_TRUE:
        case Spk_OPCODE_PUSH_NULL:
        case Spk_OPCODE_PUSH_VOID:
        case Spk_OPCODE_PUSH_CONTEXT:
            break;
            
        case Spk_OPCODE_DUP_N:
            DECODE_UINT(count);
            break;
        case Spk_OPCODE_DUP:
            break;

        case Spk_OPCODE_STORE_LOCAL:
        case Spk_OPCODE_STORE_INST_VAR:
        case Spk_OPCODE_STORE_GLOBAL:
            DECODE_UINT(index);
            break;
            
        case Spk_OPCODE_POP:
            break;
            
        case Spk_OPCODE_ROT:
            DECODE_UINT(count);
            break;

        case Spk_OPCODE_BRANCH_IF_FALSE:
        case Spk_OPCODE_BRANCH_IF_TRUE:
        case Spk_OPCODE_BRANCH_ALWAYS:
            DECODE_SINT(displacement);
            break;
            
        case Spk_OPCODE_ID:
            break;
            
        case Spk_OPCODE_OPER:
        case Spk_OPCODE_OPER_SUPER:
            oper = (SpkOper)(*instructionPointer++);
            fprintf(out, "%s, ", operName(oper));
            ip = instructionPointer;
            break;
            
        case Spk_OPCODE_CALL:
        case Spk_OPCODE_CALL_VA:
        case Spk_OPCODE_CALL_SUPER:
        case Spk_OPCODE_CALL_SUPER_VA:
            callOperator = (SpkCallOper)(*instructionPointer++);
            fprintf(out, "%s, ", callOperName(callOperator));
            ip = instructionPointer;
            DECODE_UINT(argumentCount);
            break;

        case Spk_OPCODE_SET_IND:
        case Spk_OPCODE_SET_IND_SUPER:
            break;
        
        case Spk_OPCODE_SET_INDEX:
        case Spk_OPCODE_SET_INDEX_VA:
        case Spk_OPCODE_SET_INDEX_SUPER:
        case Spk_OPCODE_SET_INDEX_SUPER_VA:
            ip = instructionPointer;
            DECODE_UINT(argumentCount);
            break;

        case Spk_OPCODE_GET_ATTR:
        case Spk_OPCODE_GET_ATTR_SUPER:
        case Spk_OPCODE_SET_ATTR:
        case Spk_OPCODE_SET_ATTR_SUPER:
            DECODE_UINT(index);
            literal = literals[index];
            break;
            
        case Spk_OPCODE_GET_ATTR_VAR:
        case Spk_OPCODE_GET_ATTR_VAR_SUPER:
        case Spk_OPCODE_SET_ATTR_VAR:
        case Spk_OPCODE_SET_ATTR_VAR_SUPER:
            break;
            
        case Spk_OPCODE_SEND_MESSAGE:
        case Spk_OPCODE_SEND_MESSAGE_VA:
        case Spk_OPCODE_SEND_MESSAGE_SUPER:
        case Spk_OPCODE_SEND_MESSAGE_SUPER_VA:
            DECODE_UINT(index);
            DECODE_UINT(argumentCount);
            literal = literals[index];
            break;
            
        case Spk_OPCODE_SEND_MESSAGE_VAR:
        case Spk_OPCODE_SEND_MESSAGE_VAR_VA:
        case Spk_OPCODE_SEND_MESSAGE_SUPER_VAR:
        case Spk_OPCODE_SEND_MESSAGE_SUPER_VAR_VA:
            DECODE_UINT(argumentCount);
            break;
            
        case Spk_OPCODE_RAISE:
            break;

        case Spk_OPCODE_RET:
        case Spk_OPCODE_RET_TRAMP:
            break;
            
        case Spk_OPCODE_LEAF:
            break;
            
        case Spk_OPCODE_SAVE:
            DECODE_UINT(contextSize);
            DECODE_UINT(stackSize);
            break;
            
        case Spk_OPCODE_ARG:
        case Spk_OPCODE_ARG_VA:
            DECODE_UINT(minArgumentCount);
            DECODE_UINT(maxArgumentCount);
            if (minArgumentCount < maxArgumentCount) {
                for (i = 0; i < maxArgumentCount - minArgumentCount + 1; ++i) {
                    DECODE_SINT(displacement);
                }
            }
            break;
            
        case Spk_OPCODE_NATIVE:
            break;
            
        case Spk_OPCODE_RESTORE_SENDER:
        case Spk_OPCODE_RESTORE_CALLER:
        case Spk_OPCODE_CALL_BLOCK:
            break;
            
        case Spk_OPCODE_CHECK_STACKP:
            DECODE_UINT(index);
            break;
        }
        
        for ( ; ip < instructionPointer; ++ip) {
            fprintf(out, "0x%02x, ", *ip);
        }
        fprintf(out, "\n");
    }
    
    if (0) spaces(level, out); else fprintf(out, "    ");
    fprintf(out, "%s\n", opcodeName(Spk_OPCODE_NOP));
    
    if (0) spaces(outer, out);
    fprintf(out, "};\n\n");
}

static void genCCodePreMethodNamespace(SpkMethodNamespace ns,
                                       Level *level,
                                       void *closure)
{
    FILE *out = (FILE *)closure;
    fprintf(out, "static SpkMethodTmpl ");
    nest(level, out);
    fprintf(out, "methods[] = {\n");
}

static void genCCodeForMethodTableEntry(SpkMethodTmpl *method,
                                        SpkUnknown **literals,
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

static void genCCodePostMethodNamespace(SpkMethodNamespace ns,
                                        Level *level,
                                        void *closure)
{
    FILE *out = (FILE *)closure;
    
    fprintf(out, "    { 0 }\n};\n\n");
}

static void genCCodeClassTemplate(SpkClassTmpl *aClass,
                                  Level *level, void *closure)
{
    FILE *out = (FILE *)closure;
    const char *superclassName;
    
    fprintf(out,
            "SpkClassTmpl ");
    if (1) {
        /* XXX: For now, we are only interested in generating code for
           Spike itself. */
        superclassName = SpkHost_SymbolAsCString(aClass->superclassName);
        fprintf(out, "Spk_Class%sTmpl = {\n"
                "    Spk_HEART_CLASS_TMPL(%s, %s), {\n",
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

static void genCCodeLiteralTable(SpkModuleTmpl *module,
                                 Level *level, void *closure)
{
    FILE *out = (FILE *)closure;
    size_t i;
    SpkUnknown *literal;
    SpkBehavior *klass;
    
    fprintf(out, "static SpkLiteralTmpl ");
    nest(level, out);
    fprintf(out, "literals[] = {\n");
    for (i = 0; i < module->literalCount; ++i) {
        literal = module->literals[i];
        fprintf(out, "    { ");
        /* XXX: It would be nice if there separate tables -- separate
           "data segments" -- for each type of literal. */
        klass = ((SpkObject *)literal)->klass;
        if (klass == Spk_CLASS(Symbol)) {
            fprintf(out, "Spk_LITERAL_SYMBOL, 0, 0.0, \"%s\"", SpkHost_SymbolAsCString(literal));
        } else if (klass == Spk_CLASS(Integer)) {
            fprintf(out, "Spk_LITERAL_INTEGER, %ld, 0.0, 0", SpkHost_IntegerAsCLong(literal));
        } else if (klass == Spk_CLASS(Float)) {
            fprintf(out, "Spk_LITERAL_FLOAT, 0, %g, 0", SpkHost_FloatAsCDouble(literal));
        } else if (klass == Spk_CLASS(Char)) {
            fprintf(out, "Spk_LITERAL_CHAR, '%c', 0.0, 0", SpkHost_CharAsCChar(literal));
        } else if (klass == Spk_CLASS(String)) {
            fprintf(out, "Spk_LITERAL_STRING, 0, 0.0, \"%s\"", SpkHost_StringAsCString(literal));
        } else if (1) {
            /* XXX: predefined names are (mis)placed in this table */
            fprintf(out, "Spk_LITERAL_INTEGER, 0");
        } else {
            /* cause a compilation error */
            fprintf(out, "unknownClassOfLiteral");
        }
        fprintf(out, " },\n");
    }
    fprintf(out, "    { 0 }\n};\n\n");
}

static void genCCodeModuleTemplate(SpkModuleTmpl *module,
                                   Level *level, void *closure)
{
    FILE *out = (FILE *)closure;
    
    fprintf(out,
            "SpkModuleTmpl Spk_Module%sTmpl = {\n"
            "    {\n"
            /* there is no class variable */
            /*"        Spk_HEART_CLASS_TMPL(%s, Module), {\n",*/
            "        \"%s\", 0, offsetof(SpkHeart, Module), {\n",
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
            "            /*traverse*/ 0,\n"
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

void SpkDisassembler_DisassembleModule(SpkModuleTmpl *module, FILE *out) {
    traverseModule(module, &disassembler, (void *)out);
}

void SpkDisassembler_DisassembleModuleAsCCode(SpkModuleTmpl *module, FILE *out) {
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
