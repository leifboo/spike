
#include "disasm.h"

#include "behavior.h"
#include "cgen.h"
#include "host.h"
#include "interp.h"
#include "module.h"
#include "rodata.h"


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


static void tab(unsigned int indent, FILE *out) {
    while (indent--)
        fprintf(out, "\t");
}

void SpkDisassembler_DisassembleMethodOpcodes(SpkMethod *method, SpkUnknown **literals, unsigned int indent, FILE *out) {
    SpkOpcode *begin, *ip, *instructionPointer, *end;
    
    begin = SpkMethod_OPCODES(method);
    end = begin + method->base.size;
    instructionPointer = begin;
    
    while (instructionPointer < end) {
        SpkOpcode opcode;
        char *keyword = 0;
        SpkUnknown *selector = 0;
        const char *mnemonic = "unk", *base = 0;
        size_t index = 0;
        ptrdiff_t displacement = 0;
        size_t contextSize = 0;
        size_t argumentCount = 0, *pArgumentCount = 0;
        size_t localCount = 0, stackSize = 0;
        size_t count = 0, *pCount = 0;
        unsigned int namespace = 0, operator = 0;
        size_t label = 0, *pLabel = 0;
        int i;
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
            operator = (unsigned int)(*instructionPointer++);
            selector = *Spk_operSelectors[operator].selector;
            break;
            
        case Spk_OPCODE_CALL:           mnemonic = "call";   goto call;
        case Spk_OPCODE_CALL_VAR:       mnemonic = "callv";  goto call;
        case Spk_OPCODE_CALL_SUPER:     mnemonic = "scall";  goto call;
        case Spk_OPCODE_CALL_SUPER_VAR: mnemonic = "scallv"; goto call;
 call:
            namespace = (unsigned int)(*instructionPointer++);
            operator = (unsigned int)(*instructionPointer++);
            selector = *Spk_operCallSelectors[operator].selector;
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
            
        case Spk_OPCODE_SEND_MESSAGE:        mnemonic = "send";  goto send;
        case Spk_OPCODE_SEND_MESSAGE_SUPER:  mnemonic = "ssend"; goto send;
 send:
            base = "literal";
            DECODE_UINT(index);
            DECODE_UINT(argumentCount);
            pArgumentCount = &argumentCount;
            literal = literals[index];
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
            break;
            
        case Spk_OPCODE_ARG:     mnemonic = "arg";  goto save;
        case Spk_OPCODE_ARG_VAR: mnemonic = "argv"; goto save;
 save:
            DECODE_UINT(argumentCount);
            DECODE_UINT(localCount);
            DECODE_UINT(stackSize);
            break;
            
        case Spk_OPCODE_NATIVE: mnemonic = "ntv"; break;
            
        case Spk_OPCODE_RESTORE_SENDER: mnemonic = "restore"; keyword = "sender"; break;
        case Spk_OPCODE_RESTORE_CALLER: mnemonic = "restore"; keyword = "caller"; break;
        case Spk_OPCODE_THUNK:          mnemonic = "thunk"; break;
        case Spk_OPCODE_CALL_THUNK:     mnemonic = "ct";    break;
        case Spk_OPCODE_CALL_BLOCK:     mnemonic = "cb";    break;
            
        case Spk_OPCODE_CHECK_STACKP:
            mnemonic = "check";
            base = "stackp";
            DECODE_UINT(index);
            break;
        }
        
        tab(indent, out);
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
        } else if (pArgumentCount) {
            SpkUnknown *s = SpkHost_ObjectAsString(selector);
            fprintf(out, "\t[%u].%s %lu", namespace, SpkHost_StringAsCString(s),
                    (unsigned long)*pArgumentCount);
            Spk_DECREF(s);
        } else if (selector) {
            SpkUnknown *s = SpkHost_ObjectAsString(selector);
            fprintf(out, "\t$%s", SpkHost_StringAsCString(s));
            Spk_DECREF(s);
        } else if (pLabel) {
            fprintf(out, "\t%04lx", (unsigned long)*pLabel);
        } else if (base) {
            fprintf(out, "\t%s[%lu]", base, (unsigned long)index);
        } else {
            switch (opcode) {
            case Spk_OPCODE_SAVE:
                fprintf(out, "\t%lu", (unsigned long)contextSize);
                break;
            case Spk_OPCODE_ARG:
            case Spk_OPCODE_ARG_VAR:
                fprintf(out, "\t%lu,%lu,%lu",
                        (unsigned long)argumentCount,
                        (unsigned long)localCount,
                        (unsigned long)stackSize);
                break;
            }
        }
        if (literal) {
            fprintf(out, "\t\t; ");
            SpkHost_PrintObject(literal, out);
        }
        fprintf(out, "\n");
    }
}

void SpkDisassembler_DisassembleMethod(SpkMethod *method, SpkUnknown **literals, unsigned int indent, FILE *out) {
    SpkBehavior *nestedClass;
    SpkMethod *nestedMethod;
    
    for (nestedClass = method->nestedClassList.first; nestedClass; nestedClass = nestedClass->nextInScope) {
        SpkDisassembler_DisassembleClass(nestedClass, indent + 1, out);
        fprintf(out, "\n");
    }
    for (nestedMethod = method->nestedMethodList.first; nestedMethod; nestedMethod = nestedMethod->nextInScope) {
        tab(indent + 1, out);
        fprintf(out, "method\n");
        SpkDisassembler_DisassembleMethod(nestedMethod, literals, indent + 1, out);
        fprintf(out, "\n");
    }
    SpkDisassembler_DisassembleMethodOpcodes(method, literals, indent + 1, out);
}

static void disassembleMethodDict(SpkUnknown *methodDict,
                                  SpkMethodNamespace namespace,
                                  SpkUnknown **literals,
                                  unsigned int indent,
                                  FILE *out)
{
    SpkUnknown *selector, *methodName, *method;
    size_t pos = 0;
    
    while (SpkHost_NextSymbol(methodDict, &pos, &selector, &method)) {
        tab(indent, out);
        methodName = SpkHost_ObjectAsString(selector);
        fprintf(out, "%d.%s\n", namespace, SpkHost_StringAsCString(methodName));
        SpkDisassembler_DisassembleMethod((SpkMethod *)method, literals, indent, out);
        fprintf(out, "\n");
        Spk_DECREF(methodName);
    }
}

void SpkDisassembler_DisassembleClass(SpkBehavior *aClass, unsigned int indent, FILE *out) {
    SpkBehavior *nestedClass;
    SpkMethodNamespace namespace;
    
    tab(indent, out);
    fprintf(out, "class %s\n", SpkBehavior_NameAsCString(aClass));
    for (nestedClass = aClass->nestedClassList.first; nestedClass; nestedClass = nestedClass->nextInScope) {
        SpkDisassembler_DisassembleClass(nestedClass, indent + 1, out);
        fprintf(out, "\n");
    }
    for (namespace = 0; namespace < Spk_NUM_METHOD_NAMESPACES; ++namespace) {
        disassembleMethodDict(aClass->ns[namespace].methodDict, namespace, aClass->module->literals, indent + 1, out);
    }
    return;
}

void SpkDisassembler_DisassembleModule(SpkModule *module, FILE *out) {
    size_t i;
    SpkBehavior *nestedClass;
    SpkMethodNamespace namespace;
    
    fprintf(out, "literal table\n");
    for (i = 0; i < module->literalCount; ++i) {
        fprintf(out, "    %04lu: ", (unsigned long)i);
        SpkHost_PrintObject(module->literals[i], out);
        fprintf(out, "\n");
    }
    fprintf(out, "\n");
    
    for (nestedClass = module->base.klass->nestedClassList.first; nestedClass; nestedClass = nestedClass->nextInScope) {
        SpkDisassembler_DisassembleClass(nestedClass, 0, out);
        fprintf(out, "\n");
    }
    for (namespace = 0; namespace < Spk_NUM_METHOD_NAMESPACES; ++namespace) {
        disassembleMethodDict(module->base.klass->ns[namespace].methodDict, namespace, module->literals, 0, out);
    }
}
