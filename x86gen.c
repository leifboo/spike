
#include "x86gen.h"

#include "char.h"
#include "float.h"
#include "heart.h"
#include "int.h"
#include "module.h"
#include "native.h"
#include "obj.h"
#include "rodata.h"
#include "st.h"
#include "str.h"
#include "sym.h"
#include "tree.h"

/* XXX: cross-compile -- all sizeof/offsetof in this file are suspect */
/*#include "rtl/rtl.h"*/
#define OFFSETOF_CONTEXT_VAR 24

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>


#define ASSERT(c, msg) \
do if (!(c)) { Halt(HALT_ASSERTION_ERROR, (msg)); goto unwind; } \
while (0)

#define _(c) do { \
Unknown *_tmp = (c); \
if (!_tmp) goto unwind; } while (0)


typedef enum CodeGenKind {
    CODE_GEN_BLOCK,
    CODE_GEN_METHOD,
    CODE_GEN_CLASS,
    CODE_GEN_MODULE
} CodeGenKind;

typedef struct BlockCodeGen {
    struct CodeGen *generic;
    struct OpcodeGen *opcodeGen;
} BlockCodeGen;

typedef struct MethodCodeGen {
    struct CodeGen *generic;
    struct OpcodeGen *opcodeGen;
} MethodCodeGen;

typedef struct OpcodeGen {
    struct CodeGen *generic;
    
    size_t minArgumentCount, maxArgumentCount; int varArgList;
    size_t localCount;
    unsigned int blockCount;
    
    Label epilogueLabel;
    
    union {
        BlockCodeGen block;
        MethodCodeGen method;
    } u;
} OpcodeGen;

typedef struct ClassCodeGen {
    struct CodeGen *generic;
    Stmt *classDef;
    String *source;
} ClassCodeGen;

typedef struct ModuleCodeGen {
    struct CodeGen *generic;
    
    Label nextLabel;
    
    long *intData;
    unsigned int intDataSize, intDataAllocSize;
    
    double *floatData;
    unsigned int floatDataSize, floatDataAllocSize;
    
    char charData[256];
    
    String **strData;
    unsigned int strDataSize, strDataAllocSize;
    
    Symbol **symData;
    unsigned int symDataSize, symDataAllocSize;
} ModuleCodeGen;

typedef struct CodeGen {
    CodeGenKind kind;
    struct CodeGen *outer;
    struct ModuleCodeGen *module;
    unsigned int level;
    FILE *out;
    union {
        OpcodeGen o;
        ClassCodeGen klass;
        ModuleCodeGen module;
    } u;
} CodeGen;


/****************************************************************************/
/* opcodes */

static unsigned int willEmitInt(long, CodeGen *);
static unsigned int willEmitFloat(double, CodeGen *);
static void willEmitChar(unsigned int, CodeGen *);
static unsigned int willEmitStr(String *, CodeGen *);
static unsigned int willEmitSym(Symbol *, CodeGen *);


static void emitOpcode(OpcodeGen *cgen,
                       const char *mnemonic,
                       const char *operands, ...)
{
    va_list ap;
    FILE *out;
    
    out = cgen->generic->out;
    fprintf(out, "\t%s", mnemonic);
    if (operands) {
        fputs("\t", out);
        va_start(ap, operands);
        vfprintf(out, operands, ap);
        va_end(ap);
    }
    fputs("\n", out);
}

static Label getLabel(Label *target, OpcodeGen *cgen) {
    if (!*target)
        *target = cgen->generic->module->nextLabel++;
    return *target;
}

static void defineLabel(Label *label, OpcodeGen *cgen) {
    if (!*label)
        *label = cgen->generic->module->nextLabel++;
}

static void maybeEmitLabel(Label *label, OpcodeGen *cgen) {
    if (*label)
        fprintf(cgen->generic->out, ".L%u:\n", *label);
}

static Unknown *emitBranch(Opcode opcode, Label *target, OpcodeGen *cgen) {
    switch (opcode) {
    case OPCODE_BRANCH_IF_FALSE:
        emitOpcode(cgen, "call", "SpikeTest");
        emitOpcode(cgen, "jz", ".L%u", getLabel(target, cgen));
        break;
    case OPCODE_BRANCH_IF_TRUE:
        emitOpcode(cgen, "call", "SpikeTest");
        emitOpcode(cgen, "jnz", ".L%u", getLabel(target, cgen));
        break;
    case OPCODE_BRANCH_ALWAYS:
        emitOpcode(cgen, "jmp", ".L%u", getLabel(target, cgen));
        break;
    }
    return GLOBAL(xvoid);
}

static void dupN(unsigned long n, OpcodeGen *cgen) {
    unsigned long i;
    
    for (i = 0; i < n; ++i) {
        emitOpcode(cgen, "pushl", "%lu(%%esp)", 4 * (n - 1));
    }
}

static int instVarOffset(Expr *def, OpcodeGen *cgen) {
    return sizeof(Object *) * def->u.def.index;
}

static int offsetOfIndex(unsigned int i, OpcodeGen *cgen) {
    int index;
    
    index = (int)i;
    
    if (cgen->blockCount) {
        /* all vars are in MethodContext on heap at %ebp */
        return OFFSETOF_CONTEXT_VAR + sizeof(Object *) * index;
    }
    
    if (cgen->varArgList) {
        /* all vars are below %ebp */
        /* skip saved edx, edi, esi, ebx */
        return -(sizeof(Object *) * (index + 1 + 4));
    }
    
    if (index < cgen->maxArgumentCount) {
        /* argument */
        /* args are pushed in order, so they are reversed in memory */
        /* skip saved ebp, eip */
        return sizeof(Object *) * (cgen->maxArgumentCount - index - 1 + 2);
    }
    /* ordinary local variable */
    /* skip saved edi, esi, ebx */
    index -= cgen->maxArgumentCount;
    return -(sizeof(Object *) * (index + 1 + 3));
}

static int localVarOffset(Expr *def, OpcodeGen *cgen) {
    return offsetOfIndex(def->u.def.index, cgen);
}

static int isVarDef(Expr *def) {
    return
        def->aux.nameKind == EXPR_NAME_DEF &&
        def->u.def.stmt &&
        def->u.def.stmt->kind == STMT_DEF_VAR;
}

static int isVarRef(Expr *expr) {
    return
        expr->aux.nameKind = EXPR_NAME_REF &&
        isVarDef(expr->u.ref.def);
}

static Unknown *emitCodeForName(Expr *expr, int *super, OpcodeGen *cgen) {
    Expr *def;
    const char *builtin;
    
    def = expr->u.ref.def;
    ASSERT(def, "missing definition");
    
    if (def->u.def.level == 0) {
        /* built-in */
        ASSERT(def->u.def.pushOpcode, "missing push opcode for built-in");
        switch (def->u.def.pushOpcode) {
        case OPCODE_PUSH_SELF:
            builtin = "%%esi";
            break;
        case OPCODE_PUSH_SUPER:
            ASSERT(super, "invalid use of 'super'");
            *super = 1;
            builtin = "%%esi";
            break;
        case OPCODE_PUSH_FALSE:
            builtin = "$false";
            break;
        case OPCODE_PUSH_TRUE:
            builtin = "$true";
            break;
        case OPCODE_PUSH_NULL:
            builtin = "$0";
            break;
        case OPCODE_PUSH_VOID:
            builtin = "$void";
            break;
        case OPCODE_PUSH_CONTEXT:
            /* XXX */
            builtin = "%%ebp";
            ASSERT(0, "XXX 'thisContext' not supported by x86 backend");
            break;
        default:
            ASSERT(0, "unexpected push opcode");
            break;
        }
        emitOpcode(cgen, "pushl", builtin);
        goto leave;
    }
    
    switch (def->u.def.pushOpcode) {
    case OPCODE_PUSH_GLOBAL:
        emitOpcode(cgen, "pushl",
                   isVarDef(def) ? "%s%s" : "$%s%s",
                   Symbol_AsCString(def->sym->sym),
                   (IS_EXTERN(def) && IS_CDECL(def)) ? ".thunk" : "");
        break;
    case OPCODE_PUSH_INST_VAR:
        emitOpcode(cgen, "pushl", "%d(%%edi)", instVarOffset(def, cgen));
        break;
    case OPCODE_PUSH_LOCAL:
        emitOpcode(cgen, "pushl", "%d(%%ebp)", localVarOffset(def, cgen));
        break;
    default:
        ASSERT(0, "unexpected push opcode");
        break;
    }
    
 leave:
    return GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static Unknown *emitCodeForLiteral(Unknown *literal, OpcodeGen *cgen) {
    Behavior *klass;
    
    klass = ((Object *)literal)->klass;
    if (klass == CLASS(Symbol)) {
        const char *value = Symbol_AsCString((Symbol *)literal);
        willEmitSym((Symbol *)literal, cgen->generic);
        emitOpcode(cgen, "pushl", "$__sym_%s", value);
    } else if (klass == CLASS(Integer)) {
        long value = Integer_AsCLong((Integer *)literal);
        /* a negative literal int is currently not possible */
        ASSERT(value >= 0, "negative literal int");
        if (value <= 0x1fffffff) {
            emitOpcode(cgen, "pushl", "$%ld", (value << 2) | 0x2);
        } else {
            /* box */
            willEmitInt(value, cgen->generic);
            emitOpcode(cgen, "pushl", "$__int_%ld", value);
        }
    } else if (klass == CLASS(Float)) {
        double value = Float_AsCDouble((Float *)literal);
        unsigned int index = willEmitFloat(value, cgen->generic);
        emitOpcode(cgen, "pushl", "$__float_%u", index);
    } else if (klass == CLASS(Char)) {
        unsigned int value = (unsigned char)Char_AsCChar((Char *)literal);
        willEmitChar(value, cgen->generic);
        emitOpcode(cgen, "pushl", "$__char_%02x", value);
    } else if (klass == CLASS(String)) {
        unsigned int index = willEmitStr((String *)literal, cgen->generic);
        emitOpcode(cgen, "pushl", "$__str_%u", index);
    } else {
        ASSERT(0, "unknown class of literal");
    }
    
    return GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static Unknown *store(Expr *var, OpcodeGen *cgen) {
    Expr *def = var->u.ref.def;
    emitOpcode(cgen, "movl", "(%%esp), %%eax");
    switch (def->u.def.storeOpcode) {
    case OPCODE_STORE_GLOBAL:
        /* currently, the x86 backend is unique in this regard */
        ASSERT(isVarDef(def), "invalid lvalue");
        emitOpcode(cgen, "movl", "%%eax, %s", Symbol_AsCString(def->sym->sym));
        break;
    case OPCODE_STORE_INST_VAR:
        emitOpcode(cgen, "movl", "%%eax, %d(%%edi)", instVarOffset(def, cgen));
        break;
    case OPCODE_STORE_LOCAL:
        emitOpcode(cgen, "movl", "%%eax, %d(%%ebp)", localVarOffset(def, cgen));
        break;
    }
    return GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static void pop(OpcodeGen *cgen) {
    emitOpcode(cgen, "popl", "%%eax");
}

static void prologue(OpcodeGen *cgen) {
    Label loop, skip;
    
    if (!cgen->varArgList && !cgen->blockCount) {
        /* nothing to do -- shared prologue did all the work */
        return;
    }
    
    /* discard local variable area */
    emitOpcode(cgen, "addl", "$%lu, %%esp", (unsigned long)4*cgen->localCount); 
    
    /* save argument count for epilogue */
    emitOpcode(cgen, "pushl", "%%edx");
    
    if (cgen->blockCount) {
        /* XXX: push Method pointer ? */
        emitOpcode(cgen, "pushl", "%%esp"); /* stackp */
        emitOpcode(cgen, "pushl", "%%edi"); /* instVarPointer */
        emitOpcode(cgen, "pushl", "%%esi"); /* receiver */
        emitOpcode(cgen, "pushl", "%%ebx"); /* methodClass */
        emitOpcode(cgen, "leal", "8(%%ebp,%%edx,4), %%eax"); /* pointer to first arg */
        emitOpcode(cgen, "pushl", "%%eax");
        emitOpcode(cgen, "pushl", "%%edx"); /* actual argument count */
        emitOpcode(cgen, "pushl", "$%lu", cgen->localCount);
        emitOpcode(cgen, "pushl", "$%d",  cgen->varArgList);
        emitOpcode(cgen, "pushl", "$%lu", cgen->maxArgumentCount);
        emitOpcode(cgen, "pushl", "$%lu", cgen->minArgumentCount);
        emitOpcode(cgen, "call", "SpikeCreateMethodContext");
        emitOpcode(cgen, "addl", "$%lu, %%esp", 4*10); 
        emitOpcode(cgen, "movl", "%%eax, %%ebp");
        /* pointer to new, heap-allocated, initialized MethodContext
           object is now in %ebp */
        return;
    }
    
    if (cgen->maxArgumentCount) {
        /* copy fixed arguments */
        emitOpcode(cgen, "movl", "$%lu, %%ecx", cgen->maxArgumentCount);
        loop = 0;
        defineLabel(&loop, cgen);
        emitOpcode(cgen, "cmpl", "$%lu, %%edx", cgen->maxArgumentCount);
        emitOpcode(cgen, "jae", ".L%u", getLabel(&loop, cgen));
        emitOpcode(cgen, "movl", "$%lu, %%edx", cgen->maxArgumentCount);
        maybeEmitLabel(&loop, cgen);
        emitOpcode(cgen, "pushl", "4(%%ebp,%%edx,4)");
        emitOpcode(cgen, "decl", "%%edx");
        emitOpcode(cgen, "loop", ".L%u", getLabel(&loop, cgen));
    }
    
    /* create var arg array */
    skip = 0;
    defineLabel(&skip, cgen);
    emitOpcode(cgen, "pushl", "$Array");
    emitOpcode(cgen, "leal", "8(%%ebp), %%eax");
    emitOpcode(cgen, "orl", "$3, %%eax"); /* map to CObject */
    emitOpcode(cgen, "pushl", "%%eax");
    emitOpcode(cgen, "movl", "-16(%%ebp), %%eax"); /* compute excess arg count */
    emitOpcode(cgen, "subl", "$%lu, %%eax", cgen->maxArgumentCount);
    emitOpcode(cgen, "jae", ".L%u", getLabel(&skip, cgen));
    emitOpcode(cgen, "movl", "$0, %%eax");
    maybeEmitLabel(&skip, cgen);
    emitOpcode(cgen, "sall", "$2, %%eax"); /* box it */
    emitOpcode(cgen, "orl", "$2, %%eax");
    emitOpcode(cgen, "pushl", "%%eax");
    emitOpcode(cgen, "movl", "$__sym_withContentsOfStack$size$, %%edx");
    emitOpcode(cgen, "movl", "$2, %%ecx");
    emitOpcode(cgen, "call", "SpikeSendMessage"); /* leave result on stack */
    
    /* restore %edx */
    emitOpcode(cgen, "movl", "-16(%%ebp), %%edx");
    
    if (cgen->localCount > 1) {
        /* reallocate locals */
        emitOpcode(cgen, "movl", "$%lu, %%ecx", cgen->localCount - 1);
        loop = 0;
        defineLabel(&loop, cgen);
        maybeEmitLabel(&loop, cgen);
        emitOpcode(cgen, "pushl", "$0");
        emitOpcode(cgen, "loop", ".L%u", getLabel(&loop, cgen));
    }
}

static void epilogue(OpcodeGen *cgen) {
    /* discard locals */
    if (cgen->blockCount) {
        /* locals on heap */
    } else if (cgen->varArgList) {
        emitOpcode(cgen, "addl", "$%lu, %%esp",
                   (unsigned long)4*(cgen->localCount + cgen->maxArgumentCount)); 
    } else if (cgen->localCount != 0) {
        emitOpcode(cgen, "addl", "$%lu, %%esp", (unsigned long)4*cgen->localCount); 
    }
    
    if (cgen->varArgList || cgen->blockCount) {
        Label skip;
        skip = 0;
        defineLabel(&skip, cgen);
        emitOpcode(cgen, "popl", "%%edx"); /* argumentCount */
        emitOpcode(cgen, "cmpl", "$%lu, %%edx", cgen->maxArgumentCount);
        emitOpcode(cgen, "jae", ".L%u", getLabel(&skip, cgen));
        emitOpcode(cgen, "movl", "$%lu, %%edx", cgen->maxArgumentCount);
        maybeEmitLabel(&skip, cgen);
        emitOpcode(cgen, "shll", "$2, %%edx"); /* convert to byte count */
    }
    
    /* restore registers */
    emitOpcode(cgen, "popl", "%%edi"); /* instVarPointer */
    emitOpcode(cgen, "popl", "%%esi"); /* self */
    emitOpcode(cgen, "popl", "%%ebx"); /* methodClass */
    
    /* save result */
    if (cgen->blockCount)
        emitOpcode(cgen, "movl", "%%eax, 8(%%esp,%%edx)");
    else if (cgen->varArgList)
        emitOpcode(cgen, "movl", "%%eax, 8(%%ebp,%%edx)");
    else
        emitOpcode(cgen, "movl", "%%eax, %d(%%ebp)",
                   sizeof(Object *)*(2+cgen->maxArgumentCount));
    
    /* restore caller's stack frame (%ebp) */
    emitOpcode(cgen, "popl", "%%ebp");
    
    /* pop arguments and return, leaving result on the stack */
    if (cgen->varArgList || cgen->blockCount) {
        /* ret %edx */
        emitOpcode(cgen, "popl", "%%eax"); /* return address */
        emitOpcode(cgen, "addl", "%%edx, %%esp"); 
        emitOpcode(cgen, "jmp", "*%%eax");  /* return */
    } else {
        emitOpcode(cgen, "ret", "$%lu", sizeof(Object *)*cgen->maxArgumentCount);
    }
}

static void oper(Oper code, int isSuper, OpcodeGen *cgen) {
    static const char *table[] = {
    "Succ",
    "Pred",
    "Addr",
    "Ind",
    "Pos",
    "Neg",
    "BNeg",
    "LNeg",
    "Mul",
    "Div",
    "Mod",
    "Add",
    "Sub",
    "LShift",
    "RShift",
    "LT",
    "GT",
    "LE",
    "GE",
    "Eq",
    "NE",
    "BAnd",
    "BXOr",
    "BOr"
    };
    
    emitOpcode(cgen, "call", "SpikeOper%s%s", table[code], (isSuper ? "Super" : ""));
}


/****************************************************************************/
/* rodata */

static unsigned int willEmitInt(long value, CodeGen *gcgen) {
    ModuleCodeGen *cgen;
    unsigned int i;
    
    cgen = gcgen->module;
    
    for (i = 0; i < cgen->intDataSize; ++i) {
        if (cgen->intData[i] == value)
            return i;
    }
    
    ++cgen->intDataSize;
    
    if (cgen->intDataSize >
        cgen->intDataAllocSize) {
        cgen->intDataAllocSize
            = (cgen->intDataAllocSize
               ? cgen->intDataAllocSize * 2
               : 2);
        cgen->intData
            = (long *)realloc(
                cgen->intData,
                cgen->intDataAllocSize * sizeof(long)
                );
    }
    
    cgen->intData[i] = value;
    
    return i;
}

static unsigned int willEmitFloat(double value, CodeGen *gcgen) {
    ModuleCodeGen *cgen;
    unsigned int i;
    
    cgen = gcgen->module;
    
    for (i = 0; i < cgen->floatDataSize; ++i) {
        if (cgen->floatData[i] == value)
            return i;
    }
    
    ++cgen->floatDataSize;
    
    if (cgen->floatDataSize >
        cgen->floatDataAllocSize) {
        cgen->floatDataAllocSize
            = (cgen->floatDataAllocSize
               ? cgen->floatDataAllocSize * 2
               : 2);
        cgen->floatData
            = (double *)realloc(
                cgen->floatData,
                cgen->floatDataAllocSize * sizeof(double)
                );
    }
    
    cgen->floatData[i] = value;
    
    return i;
}

static void willEmitChar(unsigned int value, CodeGen *gcgen) {
    ModuleCodeGen *cgen;
    
    cgen = gcgen->module;
    cgen->charData[value % 256] = 1;
}

static unsigned int willEmitStr(String *value, CodeGen *gcgen) {
    ModuleCodeGen *cgen;
    unsigned int i;
    
    cgen = gcgen->module;
    
    for (i = 0; i < cgen->strDataSize; ++i) {
        if (String_IsEqual(cgen->strData[i], value))
            return i;
    }
    
    ++cgen->strDataSize;
    
    if (cgen->strDataSize >
        cgen->strDataAllocSize) {
        cgen->strDataAllocSize
            = (cgen->strDataAllocSize
               ? cgen->strDataAllocSize * 2
               : 2);
        cgen->strData
            = (String **)realloc(
                cgen->strData,
                cgen->strDataAllocSize * sizeof(String *)
                );
    }
    
    cgen->strData[i] = value;
    
    return i;
}

static unsigned int willEmitSym(Symbol *value, CodeGen *gcgen) {
    ModuleCodeGen *cgen;
    unsigned int i;
    
    cgen = gcgen->module;
    
    for (i = 0; i < cgen->symDataSize; ++i) {
        if (cgen->symData[i] == value)
            return i;
    }
    
    ++cgen->symDataSize;
    
    if (cgen->symDataSize >
        cgen->symDataAllocSize) {
        cgen->symDataAllocSize
            = (cgen->symDataAllocSize
               ? cgen->symDataAllocSize * 2
               : 2);
        cgen->symData
            = (Symbol **)realloc(
                cgen->symData,
                cgen->symDataAllocSize * sizeof(Symbol *)
                );
    }
    
    cgen->symData[i] = value;
    
    return i;
}

static void printStringLiteral(String *value, FILE *out) {
    const char *s, *subst;
    char c;
    
    fputc('\"', out);
    s = String_AsCString(value);
    while ((c = *s++)) {
        /* XXX: numeric escape codes */
        subst = 0;
        switch (c) {
        case '\a': subst = "\\a"; break;
        case '\b': subst = "\\b"; break;
        case '\f': subst = "\\f"; break;
        case '\n': subst = "\\n"; break;
        case '\r': subst = "\\r"; break;
        case '\t': subst = "\\t"; break;
        case '\v': subst = "\\v"; break;
        case '\\': subst = "\\\\"; break;
        case '"':  subst = "\\\""; break;
        default:
            break;
        }
        if (subst)
            fputs(subst, out);
        else
            fputc(c, out);
    }
    fputc('\"', out);
}

static void emitROData(ModuleCodeGen *cgen) {
    FILE *out;
    unsigned int i;
    long value;
    
    out = cgen->generic->out;
    fprintf(out, "\t.section\t.rodata\n");
    
    fprintf(out, "\t.align\t4\n");
    for (i = 0; i < cgen->intDataSize; ++i) {
        value = cgen->intData[i];
        fprintf(out,
                "__int_%ld:\n"
                "\t.globl\t__int_%ld\n"
                "\t.type\t__int_%ld, @object\n"
                "\t.size\t__int_%ld, 8\n"
                "\t.long\tInteger\n"
                "\t.long\t%ld\n",
                value, value, value, value, value);
    }
    
    for (i = 0; i < cgen->floatDataSize; ++i) {
        /*
         * XXX: I would find it comforting if the exact string from
         * the source was emitted here, or something... spitting out
         * "%f" seems fuzzy.
         */
        fprintf(out,
                "\t.align\t16\n"
                "__float_%u:\n"
                "\t.globl\t__float_%u\n"
                "\t.type\t__float_%u, @object\n"
                "\t.size\t__float_%u, 12\n"
                "\t.long\tFloat\n"
                "\t.double\t%f\n",
                i, i, i, i, cgen->floatData[i]);
    }

    fprintf(out, "\t.align\t4\n");
    for (i = 0; i < 256; ++i) {
        if (cgen->charData[i]) {
            fprintf(out,
                    "__char_%02x:\n"
                    "\t.globl\t__char_%02x\n"
                    "\t.type\t__char_%02x, @object\n"
                    "\t.size\t__char_%02x, 8\n"
                    "\t.long\tChar\n"
                    "\t.long\t%u\n",
                    i, i, i, i, i);
        }
    }
    
    for (i = 0; i < cgen->strDataSize; ++i) {
        fprintf(out,
                "\t.align\t4\n"
                "__str_%u:\n"
                "\t.globl\t__str_%u\n"
                "\t.type\t__str_%u, @object\n"
                "\t.long\tString\n"
                "\t.long\t%lu\n"
                "\t.string\t",
                i, i, i,
                (unsigned long)String_Size(cgen->strData[i]));
        printStringLiteral(cgen->strData[i], out);
        fprintf(out,
                "\n"
                "\t.size\t__str_%u, .-__str_%u\n",
                i, i);
    
    }
    
    for (i = 0; i < cgen->symDataSize; ++i) {
        const char *sym = Symbol_AsCString(cgen->symData[i]);
        fprintf(out,
                "\t.align\t4\n"
                "__sym_%s:\n"
                "\t.globl\t__sym_%s\n"
                "\t.type\t__sym_%s, @object\n"
                "\t.long\tSymbol\n"
                "\t.long\t%lu\n"
                "\t.string\t\"%s\"\n"
                "\t.size\t__sym_%s, .-__sym_%s\n",
                sym, sym, sym,
                (unsigned long)Symbol_Hash(cgen->symData[i]),
                sym, sym, sym);
    }
    
    fprintf(out, "\n");
}


/****************************************************************************/
/* pseudo-opcodes */

static Unknown *emitCodeForInt(int intValue, OpcodeGen *cgen) {
    Unknown *intObj = 0;
    
    intObj = (Unknown *)Integer_FromCLong(intValue);
    if (!intObj) {
        goto unwind;
    }
    _(emitCodeForLiteral(intObj, cgen));
    return GLOBAL(xvoid);
    
 unwind:
    return 0;
}


/****************************************************************************/
/* expressions */

static Unknown *emitCodeForOneExpr(Expr *, int *, OpcodeGen *);
static Unknown *emitBranchForExpr(Expr *expr, int, Label *, Label *, int,
                                     OpcodeGen *);
static Unknown *emitBranchForOneExpr(Expr *, int, Label *, Label *, int,
                                        OpcodeGen *);
static Unknown *inPlaceOp(Expr *, size_t, OpcodeGen *);
static Unknown *inPlaceAttrOp(Expr *, OpcodeGen *);
static Unknown *inPlaceIndexOp(Expr *, OpcodeGen *);
static Unknown *emitCodeForBlock(Expr *, CodeGen *);
static Unknown *emitCodeForStmt(Stmt *, Label *, Label *, Label *, OpcodeGen *);

static Unknown *emitCodeForExpr(Expr *expr, int *super, OpcodeGen *cgen) {
    for ( ; expr->next; expr = expr->next) {
        _(emitCodeForOneExpr(expr, super, cgen));
        pop(cgen);
    }
    _(emitCodeForOneExpr(expr, super, cgen));
    
    return GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static Unknown *emitCodeForOneExpr(Expr *expr, int *super, OpcodeGen *cgen) {
    Expr *arg;
    size_t argumentCount;
    int isSuper;
    const char *routine;
    
    if (super) {
        *super = 0;
    }
    maybeEmitLabel(&expr->label, cgen);
    
    switch (expr->kind) {
    case EXPR_LITERAL:
        _(emitCodeForLiteral(expr->aux.literalValue, cgen));
        break;
    case EXPR_NAME:
        _(emitCodeForName(expr, super, cgen));
        break;
    case EXPR_BLOCK:
        emitOpcode(cgen, "pushl", "%%ebp");
        emitOpcode(cgen, "pushl", "$%lu", (unsigned long)expr->aux.block.argumentCount);
        emitOpcode(cgen, "call", "SpikeBlockCopy");
        _(emitBranch(OPCODE_BRANCH_ALWAYS, &expr->endLabel, cgen));
        _(emitCodeForBlock(expr, cgen->generic));
        break;
    case EXPR_COMPOUND:
        emitOpcode(cgen, "pushl", "__spike_xxx_push_context"); /* XXX */
        for (arg = expr->right, argumentCount = 0;
             arg;
             arg = arg->nextArg, ++argumentCount) {
            _(emitCodeForExpr(arg, 0, cgen));
        }
        if (expr->var) {
            _(emitCodeForExpr(expr->var, 0, cgen));
        }
        emitOpcode(cgen, "call", "__spike_xxx_compoundExpression"); /* XXX */
        break;
    case EXPR_CALL:
        /* evaluate receiver */
        switch (expr->left->kind) {
        case EXPR_ATTR:
        case EXPR_ATTR_VAR:
            _(emitCodeForExpr(expr->left->left, &isSuper, cgen));
            break;
        default:
            _(emitCodeForExpr(expr->left, &isSuper, cgen));
            break;
        }
        /* evaluate arguments */
        for (arg = expr->right, argumentCount = 0;
             arg;
             arg = arg->nextArg, ++argumentCount) {
            _(emitCodeForExpr(arg, 0, cgen));
        }
        if (expr->var) {
            _(emitCodeForExpr(expr->var, 0, cgen));
        }
        /* evaluate selector */
        switch (expr->left->kind) {
        case EXPR_ATTR:
            _(emitCodeForLiteral((Unknown *)expr->left->sym->sym, cgen));
            emitOpcode(cgen, "popl", "%%edx");
            routine = "SpikeSendMessage";
            break;
        case EXPR_ATTR_VAR:
            _(emitCodeForExpr(expr->left->right, 0, cgen));
            emitOpcode(cgen, "popl", "%%edx");
            routine = "SpikeSendMessage";
            break;
        default:
            switch (expr->oper) {
            case OPER_APPLY:
                routine = "SpikeCall";
                break;
            case OPER_INDEX:
                routine = "SpikeGetIndex";
                break;
            }
            break;
        }
        if (expr->var) {
            /* push var args */
            emitOpcode(cgen, "call", "SpikePushVarArgs");
            emitOpcode(cgen, "addl", "$%lu, %%ecx", (unsigned long)argumentCount);
        } else {
            emitOpcode(cgen, "movl", "$%lu, %%ecx", (unsigned long)argumentCount);
        }
        /* call RTL routine to send message */
        emitOpcode(cgen, "call", "%s%s", routine, (isSuper ? "Super" : ""));
        break;
    case EXPR_ATTR:
        _(emitCodeForExpr(expr->left, &isSuper, cgen));
        if (expr->sym->sym == klass) {
            /* "foo.class": push 'klass' is-a pointer */
            pop(cgen);
            emitOpcode(cgen, "pushl", "(%%eax)");
        } else {
            _(emitCodeForLiteral((Unknown *)expr->sym->sym, cgen));
            emitOpcode(cgen, "popl", "%%edx");
            emitOpcode(cgen, "call", "SpikeGetAttr%s", (isSuper ? "Super" : ""));
        }
        break;
    case EXPR_ATTR_VAR:
        _(emitCodeForExpr(expr->left, &isSuper, cgen));
        _(emitCodeForExpr(expr->right, 0, cgen));
        emitOpcode(cgen, "popl", "%%edx");
        emitOpcode(cgen, "call", "SpikeGetAttr%s", (isSuper ? "Super" : ""));
        break;
    case EXPR_PREOP:
    case EXPR_POSTOP:
        switch (expr->left->kind) {
        case EXPR_NAME:
            _(emitCodeForExpr(expr->left, 0, cgen));
            if (expr->kind == EXPR_POSTOP) {
                dupN(1, cgen);
            }
            oper(expr->oper, 0, cgen);
            _(store(expr->left, cgen));
            if (expr->kind == EXPR_POSTOP) {
                pop(cgen);
            }
            break;
        case EXPR_ATTR:
        case EXPR_ATTR_VAR:
            _(inPlaceAttrOp(expr, cgen));
            break;
        case EXPR_CALL:
            _(inPlaceIndexOp(expr, cgen));
            break;
        default:
            ASSERT(0, "invalid lvalue");
        }
        break;
    case EXPR_UNARY:
        _(emitCodeForExpr(expr->left, &isSuper, cgen));
        oper(expr->oper, isSuper, cgen);
        break;
    case EXPR_BINARY:
        _(emitCodeForExpr(expr->left, &isSuper, cgen));
        _(emitCodeForExpr(expr->right, 0, cgen));
        oper(expr->oper, isSuper, cgen);
        break;
    case EXPR_ID:
    case EXPR_NI:
        _(emitCodeForExpr(expr->left, 0, cgen));
        _(emitCodeForExpr(expr->right, 0, cgen));
        pop(cgen);
        emitOpcode(cgen, "cmpl", "(%%esp), %%eax");
        emitOpcode(cgen, "movl", "$false, (%%esp)");
        emitOpcode(cgen,
                   expr->kind == EXPR_ID ? "jne" : "je",
                   ".L%u", getLabel(&expr->endLabel, cgen));
        emitOpcode(cgen, "movl", "$true, (%%esp)");
        break;
    case EXPR_AND:
        _(emitBranchForExpr(expr->left, 0, &expr->right->endLabel,
                            &expr->right->label, 1, cgen));
        _(emitCodeForExpr(expr->right, 0, cgen));
        break;
    case EXPR_OR:
        _(emitBranchForExpr(expr->left, 1, &expr->right->endLabel,
                            &expr->right->label, 1, cgen));
        _(emitCodeForExpr(expr->right, 0, cgen));
        break;
    case EXPR_COND:
        _(emitBranchForExpr(expr->cond, 0, &expr->right->label,
                            &expr->left->label, 0, cgen));
        _(emitCodeForExpr(expr->left, 0, cgen));
        _(emitBranch(OPCODE_BRANCH_ALWAYS, &expr->right->endLabel, cgen));
        _(emitCodeForExpr(expr->right, 0, cgen));
        break;
    case EXPR_KEYWORD:
        _(emitCodeForExpr(expr->left, &isSuper, cgen));
        for (arg = expr->right, argumentCount = 0;
             arg;
             arg = arg->nextArg, ++argumentCount) {
            _(emitCodeForExpr(arg, 0, cgen));
        }
        _(emitCodeForLiteral(expr->aux.keywords, cgen));
        emitOpcode(cgen, "popl", "%%edx");
        emitOpcode(cgen, "movl", "$%lu, %%ecx", (unsigned long)argumentCount);
        emitOpcode(cgen, "call", "SpikeSendMessage%s", (isSuper ? "Super" : ""));
        break;
    case EXPR_ASSIGN:
        switch (expr->left->kind) {
        case EXPR_NAME:
            if (expr->oper == OPER_EQ) {
                _(emitCodeForExpr(expr->right, 0, cgen));
            } else {
                _(emitCodeForExpr(expr->left, 0, cgen));
                _(emitCodeForExpr(expr->right, 0, cgen));
                oper(expr->oper, 0, cgen);
            }
            _(store(expr->left, cgen));
            break;
        case EXPR_ATTR:
        case EXPR_ATTR_VAR:
            _(inPlaceAttrOp(expr, cgen));
            break;
        case EXPR_CALL:
            _(inPlaceIndexOp(expr, cgen));
            break;
        default:
            ASSERT(0, "invalid lvalue");
        }
    }
    
    maybeEmitLabel(&expr->endLabel, cgen);
    
    return GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static void squirrel(size_t resultDepth, OpcodeGen *cgen) {
    /* duplicate the last result */
    dupN(1, cgen);
    /* squirrel it away for later */
    emitOpcode(cgen, "movl", "$%lu, %%ecx", (unsigned long)(resultDepth + 1));
    emitOpcode(cgen, "call", "SpikeRotate");
}

static Unknown *inPlaceOp(Expr *expr, size_t resultDepth, OpcodeGen *cgen) {
    if (expr->right) {
        _(emitCodeForExpr(expr->right, 0, cgen));
    } else if (expr->kind == EXPR_POSTOP) {
        /* e.g., "a[i]++" -- squirrel away the original value */
        squirrel(resultDepth, cgen);
    }
    
    oper(expr->oper, 0, cgen);
    
    if (expr->kind != EXPR_POSTOP) {
        /* e.g., "++a[i]" -- squirrel away the new value */
        squirrel(resultDepth, cgen);
    }
    
    return GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static Unknown *inPlaceAttrOp(Expr *expr, OpcodeGen *cgen) {
    size_t argumentCount;
    int isSuper;
    
    argumentCount = 0;
    /* get/set common receiver */
    _(emitCodeForExpr(expr->left->left, &isSuper, cgen));
    /* get/set common attr */
    switch (expr->left->kind) {
    case EXPR_ATTR:
        _(emitCodeForLiteral((Unknown *)expr->left->sym->sym, cgen));
        ++argumentCount;
        break;
    case EXPR_ATTR_VAR:
        _(emitCodeForExpr(expr->left->right, 0, cgen));
        ++argumentCount;
        break;
    default:
        ASSERT(0, "invalid lvalue");
    }
    /* rhs */
    if (expr->oper == OPER_EQ) {
        _(emitCodeForExpr(expr->right, 0, cgen));
        squirrel(1 + argumentCount + 1 /* receiver, args, new value */, cgen);
    } else {
        dupN(argumentCount + 1, cgen);
        switch (expr->left->kind) {
        case EXPR_ATTR:
        case EXPR_ATTR_VAR:
            emitOpcode(cgen, "popl", "%%edx");
            emitOpcode(cgen, "call", "SpikeGetAttr%s", (isSuper ? "Super" : ""));
            break;
        default:
            ASSERT(0, "invalid lvalue");
        }
        _(inPlaceOp(expr, 1 + argumentCount + 1, /* receiver, args, result */
                    cgen));
    }
    switch (expr->left->kind) {
    case EXPR_ATTR:
    case EXPR_ATTR_VAR:
        // XXX: re-think this whole thing
        emitOpcode(cgen, "popl", "%%eax");
        emitOpcode(cgen, "popl", "%%edx");
        emitOpcode(cgen, "pushl", "%%eax");
        emitOpcode(cgen, "call", "SpikeSetAttr%s", (isSuper ? "Super" : ""));
        break;
    default:
        ASSERT(0, "invalid lvalue");
    }
    /* discard 'set' method result, exposing the value that was
       squirrelled away */
    pop(cgen);
    
    return GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static Unknown *inPlaceIndexOp(Expr *expr, OpcodeGen *cgen) {
    Expr *arg;
    size_t argumentCount;
    int isSuper;
    
    ASSERT(expr->left->oper == OPER_INDEX, "invalid lvalue");
    /* get/set common receiver */
    _(emitCodeForExpr(expr->left->left, &isSuper, cgen));
    /* get/set common arguments */
    for (arg = expr->left->right, argumentCount = 0;
         arg;
         arg = arg->nextArg, ++argumentCount) {
        _(emitCodeForExpr(arg, 0, cgen));
    }
    /* rhs */
    if (expr->oper == OPER_EQ) {
        _(emitCodeForExpr(expr->right, 0, cgen));
        squirrel(1 + argumentCount + 1 /* receiver, args, new value */, cgen);
    } else {
        /* get __index__ { */
        dupN(argumentCount + 1, cgen);
        emitOpcode(cgen, "movl", "$%lu, %%ecx", (unsigned long)argumentCount);
        switch (expr->left->oper) {
        case OPER_APPLY:
            emitOpcode(cgen, "call", "SpikeCall%s", (isSuper ? "Super" : ""));
            break;
        case OPER_INDEX:
            emitOpcode(cgen, "call", "SpikeGetIndex%s", (isSuper ? "Super" : ""));
            break;
        default:
            ASSERT(0, "bad operator");
        }
        /* } get __index__ */
        _(inPlaceOp(expr, 1 + argumentCount + 1, /* receiver, args, result */
                    cgen));
    }
    ++argumentCount; /* new item value */
    emitOpcode(cgen, "movl", "$%lu, %%ecx", (unsigned long)argumentCount);
    emitOpcode(cgen, "call", "SpikeSetIndex%s", (isSuper ? "Super" : ""));
    /* discard 'set' method result, exposing the value that was
       squirrelled away */
    pop(cgen);
    
    return GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static Unknown *emitBranchForExpr(Expr *expr, int cond,
                                     Label *label, Label *fallThroughLabel,
                                     int dup, OpcodeGen *cgen)
{
    for ( ; expr->next; expr = expr->next) {
        _(emitCodeForOneExpr(expr, 0, cgen));
        /* XXX: We could elide this 'pop' if 'expr' is a conditional expr. */
        pop(cgen);
    }
    _(emitBranchForOneExpr(expr, cond, label, fallThroughLabel, dup, cgen));
    
    return GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static Unknown *emitBranchForOneExpr(Expr *expr, int cond,
                                        Label *label, Label *fallThroughLabel,
                                        int dup, OpcodeGen *cgen)
{
    Opcode pushOpcode;
    
    switch (expr->kind) {
    case EXPR_NAME:
        pushOpcode = expr->u.ref.def->u.def.pushOpcode;
        if (pushOpcode == OPCODE_PUSH_FALSE ||
            pushOpcode == OPCODE_PUSH_TRUE) {
            int killCode = pushOpcode == OPCODE_PUSH_TRUE ? cond : !cond;
            maybeEmitLabel(&expr->label, cgen);
            if (killCode) {
                if (dup) {
                    emitOpcode(cgen, "pushl", pushOpcode == OPCODE_PUSH_TRUE ? "$true" : "$false");
                }
                _(emitBranch(OPCODE_BRANCH_ALWAYS, label, cgen));
            }
            maybeEmitLabel(&expr->endLabel, cgen);
            break;
        } /* else fall through */
    default:
        _(emitCodeForExpr(expr, 0, cgen));
        /*
         * XXX: This sequence could be replaced by a special set of
         * branch-or-pop opcodes.
         */
        if (dup) {
            dupN(1, cgen);
        }
        _(emitBranch(cond
                     ? OPCODE_BRANCH_IF_TRUE
                     : OPCODE_BRANCH_IF_FALSE,
                     label, cgen));
        if (dup) {
            pop(cgen);
        }
        break;
    case EXPR_AND:
        maybeEmitLabel(&expr->label, cgen);
        if (cond) {
            /* branch if true */
            _(emitBranchForExpr(expr->left, 0, fallThroughLabel,
                                &expr->right->label, 0, cgen));
            _(emitBranchForExpr(expr->right, 1, label, fallThroughLabel,
                                dup, cgen));
        } else {
            /* branch if false */
            _(emitBranchForExpr(expr->left, 0, label, &expr->right->label,
                                dup, cgen));
            _(emitBranchForExpr(expr->right, 0, label, fallThroughLabel,
                                dup, cgen));
        }
        maybeEmitLabel(&expr->endLabel, cgen);
        break;
    case EXPR_OR:
        maybeEmitLabel(&expr->label, cgen);
        if (cond) {
            /* branch if true */
            _(emitBranchForExpr(expr->left, 1, label, &expr->right->label,
                                dup, cgen));
            _(emitBranchForExpr(expr->right, 1, label, fallThroughLabel,
                                dup, cgen));
        } else {
            /* branch if false */
            _(emitBranchForExpr(expr->left, 1, fallThroughLabel,
                                &expr->right->label, 0, cgen));
            _(emitBranchForExpr(expr->right, 0, label, fallThroughLabel,
                                dup, cgen));
        }
        maybeEmitLabel(&expr->endLabel, cgen);
        break;
    case EXPR_COND:
        maybeEmitLabel(&expr->label, cgen);
        _(emitBranchForExpr(expr->cond, 0, &expr->right->label,
                            &expr->left->label, 0, cgen));
        _(emitBranchForExpr(expr->left, cond, label, fallThroughLabel,
                            dup, cgen));
        _(emitBranch(OPCODE_BRANCH_ALWAYS, fallThroughLabel, cgen));
        _(emitBranchForExpr(expr->right, cond, label, fallThroughLabel,
                            dup, cgen));
        maybeEmitLabel(&expr->endLabel, cgen);
        break;
    }
    
    return GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static void blockEpilogue(OpcodeGen *cgen) {
    /* save result */
    pop(cgen);
    emitOpcode(cgen, "movl", "%%eax, %lu(%%esp)",
               4*(6+cgen->maxArgumentCount));
    
    /* pop BlockContext pointer */
    emitOpcode(cgen, "popl", "%%ecx");
    
    /* restore caller's registers */
    emitOpcode(cgen, "popl", "%%edi"); /* instVarPointer */
    emitOpcode(cgen, "popl", "%%esi"); /* self */
    emitOpcode(cgen, "popl", "%%ebx"); /* methodClass */
    emitOpcode(cgen, "popl", "%%ebp"); /* framePointer */
    emitOpcode(cgen, "popl", "%%eax"); /* return address */
    if (cgen->maxArgumentCount)
        emitOpcode(cgen, "addl", "$%lu, %%esp", 4*cgen->maxArgumentCount); 
    
    /* save our %eip in BlockContext.pc and resume caller */
    emitOpcode(cgen, "call", "SpikeResumeCaller");
}

static Unknown *emitCodeForBlock(Expr *expr, CodeGen *outer) {
    CodeGen gcgen;
    OpcodeGen *cgen; BlockCodeGen *bcg;
    Stmt *body;
    Expr *valueExpr, voidDef, voidExpr;
    CodeGen *home;
    Stmt *s;
    Label start;

    memset(&voidDef, 0, sizeof(voidDef));
    memset(&voidExpr, 0, sizeof(voidExpr));
    voidDef.kind = EXPR_NAME;
    voidDef.sym = 0 /*SymbolNode_FromCString(st, "void")*/ ;
    voidDef.u.def.pushOpcode = OPCODE_PUSH_VOID;
    voidExpr.kind = EXPR_NAME;
    voidExpr.sym = voidDef.sym;
    voidExpr.u.ref.def = &voidDef;
    
    valueExpr = expr->right ? expr->right : &voidExpr;
    
    /* push block code generator */
    cgen = &gcgen.u.o;
    cgen->generic = &gcgen;
    cgen->generic->kind = CODE_GEN_BLOCK;
    cgen->generic->outer = outer;
    cgen->generic->module = outer->module;
    cgen->generic->level = outer->level + 1;
    cgen->generic->out = outer->out;
    bcg = &gcgen.u.o.u.block;
    bcg->opcodeGen = &cgen->generic->u.o;
    bcg->opcodeGen->generic = cgen->generic;
    cgen->minArgumentCount = expr->aux.block.argumentCount;
    cgen->maxArgumentCount = expr->aux.block.argumentCount;
    cgen->varArgList = 0;
    cgen->localCount = expr->aux.block.localCount;
    cgen->blockCount = 1; /* trigger alternate %ebp mapping */
    cgen->epilogueLabel = 0;
    
    switch (outer->kind) {
    case CODE_GEN_BLOCK:
    case CODE_GEN_METHOD:
        break;
    default:
        ASSERT(0, "block not allowed here");
    }
    
    /*
     * prologue
     */
    
    start = 0;
    defineLabel(&start, cgen);
    maybeEmitLabel(&start, cgen);
    
    if (expr->aux.block.argumentCount) {
        unsigned int arg, index;
        
        /* store args into local variables */
        for (arg = 0; arg < expr->aux.block.argumentCount; ++arg) {
            index = expr->u.def.index + arg;
            emitOpcode(cgen, "movl", "%lu(%%esp), %%eax", 4*(6+expr->aux.block.argumentCount-arg-1));
            emitOpcode(cgen, "movl", "%%eax, %d(%%ebp)", offsetOfIndex(index, cgen));
        }
    }
    
    /*
     * body
     */
    
    body = expr->aux.block.stmtList;
    
    for (s = body; s; s = s->next) {
        _(emitCodeForStmt(s, &valueExpr->label, 0, 0, cgen));
    }
    _(emitCodeForExpr(valueExpr, 0, cgen));
    
    /*
     * epilogue
     */
    
    blockEpilogue(cgen);
    
    /* upon resume, loop */
    _(emitBranch(OPCODE_BRANCH_ALWAYS, &start, cgen));
    
    return GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static Unknown *emitCodeForInitializer(Expr *expr, OpcodeGen *cgen) {
    Expr *def;
    
    maybeEmitLabel(&expr->label, cgen);
    
    _(emitCodeForExpr(expr->right, 0, cgen));
    /* similar to store(), but with a definition instead of a
       reference */
    def = expr->left;
    ASSERT(def->kind == EXPR_NAME, "name expected");
    ASSERT(def->u.def.storeOpcode == OPCODE_STORE_LOCAL, "local variable expected");
    pop(cgen);
    emitOpcode(cgen, "movl", "%%eax, %d(%%ebp)", localVarOffset(def, cgen));
    
    maybeEmitLabel(&expr->endLabel, cgen);
    
    return GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static Unknown *emitCodeForVarDefList(Expr *defList, OpcodeGen *cgen) {
    Expr *expr;
    
    for (expr = defList; expr; expr = expr->next) {
        if (expr->kind == EXPR_ASSIGN) {
            _(emitCodeForInitializer(expr, cgen));
        }
    }
    return GLOBAL(xvoid);
    
 unwind:
    return 0;
}

/****************************************************************************/
/* statements */

static Unknown *emitCodeForMethod(Stmt *stmt, int meta, CodeGen *cgen);
static Unknown *emitCodeForClass(Stmt *stmt, CodeGen *cgen);

static Unknown *emitCodeForStmt(Stmt *stmt,
                                   Label *parentNextLabel,
                                   Label *breakLabel,
                                   Label *continueLabel,
                                   OpcodeGen *cgen)
{
    Stmt *s;
    Label *nextLabel, *childNextLabel, *elseLabel;
    
    /* for branching to the next statement in the control flow */
    nextLabel = stmt->next ? &stmt->next->label : parentNextLabel;
    
    maybeEmitLabel(&stmt->label, cgen);
    
    switch (stmt->kind) {
    case STMT_BREAK:
        ASSERT(breakLabel, "break not allowed here");
        _(emitBranch(OPCODE_BRANCH_ALWAYS, breakLabel, cgen));
        break;
    case STMT_COMPOUND:
        for (s = stmt->top; s; s = s->next) {
            _(emitCodeForStmt(s, nextLabel, breakLabel, continueLabel, cgen));
        }
        break;
    case STMT_CONTINUE:
        ASSERT(continueLabel, "continue not allowed here");
        _(emitBranch(OPCODE_BRANCH_ALWAYS, continueLabel, cgen));
        break;
    case STMT_DEF_METHOD:
    case STMT_DEF_CLASS:
        break;
    case STMT_DEF_VAR:
        _(emitCodeForVarDefList(stmt->expr, cgen));
        break;
    case STMT_DEF_MODULE:
        ASSERT(0, "unexpected module node");
        break;
    case STMT_DEF_SPEC:
        ASSERT(0, "unexpected spec node");
        break;
    case STMT_DO_WHILE:
        childNextLabel = &stmt->expr->label;
        defineLabel(&stmt->top->label, cgen);
        _(emitCodeForStmt(stmt->top, childNextLabel, nextLabel,
                          childNextLabel, cgen));
        _(emitBranchForExpr(stmt->expr, 1, &stmt->top->label, nextLabel,
                            0, cgen));
        break;
    case STMT_EXPR:
        if (stmt->expr) {
            _(emitCodeForExpr(stmt->expr, 0, cgen));
            pop(cgen);
        }
        break;
    case STMT_FOR:
        childNextLabel = stmt->incr
                         ? &stmt->incr->label
                         : (stmt->expr
                            ? &stmt->expr->label
                            : &stmt->top->label);
        if (stmt->init) {
            _(emitCodeForExpr(stmt->init, 0, cgen));
            pop(cgen);
        }
        if (stmt->expr) {
            _(emitBranch(OPCODE_BRANCH_ALWAYS, &stmt->expr->label,
                         cgen));
        }
        defineLabel(&stmt->top->label, cgen);
        _(emitCodeForStmt(stmt->top, childNextLabel, nextLabel,
                          childNextLabel, cgen));
        if (stmt->incr) {
            _(emitCodeForExpr(stmt->incr, 0, cgen));
            pop(cgen);
        }
        if (stmt->expr) {
            _(emitBranchForExpr(stmt->expr, 1, &stmt->top->label,
                                nextLabel, 0, cgen));
        } else {
            _(emitBranch(OPCODE_BRANCH_ALWAYS, &stmt->top->label,
                         cgen));
        }
        break;
    case STMT_IF_ELSE:
        elseLabel = stmt->bottom ? &stmt->bottom->label : nextLabel;
        _(emitBranchForExpr(stmt->expr, 0, elseLabel, &stmt->top->label,
                            0, cgen));
        _(emitCodeForStmt(stmt->top, nextLabel, breakLabel, continueLabel,
                          cgen));
        if (stmt->bottom) {
            _(emitBranch(OPCODE_BRANCH_ALWAYS, nextLabel, cgen));
            _(emitCodeForStmt(stmt->bottom, nextLabel, breakLabel,
                              continueLabel, cgen));
        }
        break;
    case STMT_PRAGMA_SOURCE:
        break;
    case STMT_RETURN:
        if (cgen->generic->kind == CODE_GEN_BLOCK) {
            /*
             * Here the code has to longjmp (in essence) to the home
             * context, and then branch to the enclosing method's
             * epilogue.
             */
            OpcodeGen *mcg; CodeGen *cg;
            
            /* find the code gen for the enclosing method */
            cg = cgen->generic;
            do cg = cg->outer; while (cg->kind != CODE_GEN_METHOD);
            mcg = &cg->u.o;
            
            /* evaluate result */
            if (stmt->expr) {
                _(emitCodeForExpr(stmt->expr, 0, cgen));
            } else {
                emitOpcode(cgen, "pushl", "$void");
            }
            
            /* resume home context, moving result to home stack */
            emitOpcode(cgen, "call", "SpikeResumeHome");
            
            /* pop result and return from home context */
            pop(cgen);
            _(emitBranch(OPCODE_BRANCH_ALWAYS, &mcg->epilogueLabel, cgen));
        } else {
            if (stmt->expr) {
                _(emitCodeForExpr(stmt->expr, 0, cgen));
                pop(cgen);
            } else {
                emitOpcode(cgen, "movl", "$void, %%eax");
            }
            _(emitBranch(OPCODE_BRANCH_ALWAYS, &cgen->epilogueLabel, cgen));
        }
        break;
    case STMT_WHILE:
        childNextLabel = &stmt->expr->label;
        _(emitBranch(OPCODE_BRANCH_ALWAYS, &stmt->expr->label, cgen));
        defineLabel(&stmt->top->label, cgen);
        _(emitCodeForStmt(stmt->top, childNextLabel, nextLabel,
                          childNextLabel, cgen));
        _(emitBranchForExpr(stmt->expr, 1, &stmt->top->label, nextLabel,
                            0, cgen));
        break;
    case STMT_YIELD:
        if (stmt->expr) {
            _(emitCodeForExpr(stmt->expr, 0, cgen));
        } else {
            emitOpcode(cgen, "pushl", "$void");
        }
        blockEpilogue(cgen);
        break;
    }
    
    return GLOBAL(xvoid);
    
 unwind:
    return 0;
}


/****************************************************************************/
/* methods */

static Unknown *emitCodeForArgList(Stmt *stmt, OpcodeGen *cgen) {
    //ASSERT(!cgen->varArgList, "XXX var args");
    
    if (cgen->minArgumentCount < cgen->maxArgumentCount) {
        /* generate code for default argument initializers */
        Expr *arg, *optionalArgList;
        size_t tally;
        Label skip, table;
        FILE *out;
        
        out = cgen->generic->out;
        
        for (arg = stmt->u.method.argList.fixed;
             arg->kind != EXPR_ASSIGN;
             arg = arg->nextArg)
            ;
        optionalArgList = arg;
        
        skip = table = 0;
        defineLabel(&skip, cgen);
        defineLabel(&table, cgen);
        
        if (cgen->varArgList) {
            /* range check */
            emitOpcode(cgen, "cmpl", "$%lu, %%edx", cgen->maxArgumentCount);
            emitOpcode(cgen, "jae", ".L%u", getLabel(&skip, cgen));
        }
        
        /* switch jump */
        emitOpcode(cgen, "subl", "$%lu, %%edx", cgen->minArgumentCount);
        emitOpcode(cgen, "shll", "$2, %%edx"); /* convert to byte offset */
        emitOpcode(cgen, "movl", ".L%u(%%edx), %%eax", getLabel(&table, cgen));
        emitOpcode(cgen, "jmp", "*%%eax");
        
        /* switch table */
        fprintf(out, "\t.section\t.rodata\n");
        fprintf(out, "\t.align\t4\n");
        maybeEmitLabel(&table, cgen);
        for (arg = optionalArgList, tally = 0; arg; arg = arg->nextArg, ++tally) {
            ASSERT(arg->kind == EXPR_ASSIGN, "assignment expected");
            fprintf(out, "\t.long\t.L%u\n", getLabel(&arg->label, cgen));
        }
        if (!cgen->varArgList)
            fprintf(out, "\t.long\t.L%u\n", getLabel(&skip, cgen));
        fprintf(out, "\t.text\n");
        
        ASSERT(tally == cgen->maxArgumentCount - cgen->minArgumentCount,
               "wrong number of default arguments");
        
        /* assignments */
        for (arg = optionalArgList; arg; arg = arg->nextArg) {
            _(emitCodeForInitializer(arg, cgen));
        }
        
        maybeEmitLabel(&skip, cgen);
    }
    
    return GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static Unknown *emitCodeForMethod(Stmt *stmt, int meta, CodeGen *outer) {
    CodeGen gcgen;
    OpcodeGen *cgen; MethodCodeGen *mcg;
    Stmt *body, *s;
    Stmt sentinel;
    Symbol *selector;
    FILE *out;
    const char *codeObjectClass;
    const char *className, *suffix, *ns, *functionName, *obj, *code;
    
    selector = stmt->u.method.name->sym;
    memset(&sentinel, 0, sizeof(sentinel));
    sentinel.kind = STMT_RETURN;
    
    /* push method code generator */
    cgen = &gcgen.u.o;
    cgen->generic = &gcgen;
    cgen->generic->kind = CODE_GEN_METHOD;
    cgen->generic->outer = outer;
    cgen->generic->module = outer->module;
    cgen->generic->level = outer->level + 1;
    cgen->generic->out = outer->out;
    mcg = &gcgen.u.o.u.method;
    mcg->opcodeGen = &cgen->generic->u.o;
    mcg->opcodeGen->generic = cgen->generic;
    cgen->minArgumentCount = stmt->u.method.minArgumentCount;
    cgen->maxArgumentCount = stmt->u.method.maxArgumentCount;
    cgen->varArgList = stmt->u.method.argList.var ? 1 : 0;
    cgen->localCount = stmt->u.method.localCount + cgen->varArgList;
    cgen->blockCount = stmt->u.method.blockCount;
    cgen->epilogueLabel = 0;
    
    body = stmt->top;
    ASSERT(body->kind == STMT_COMPOUND,
           "compound statement expected");
    
    out = cgen->generic->out;
    
    obj = "";
    code = ".code";
    
    if (outer->kind == CODE_GEN_CLASS && outer->level != 1) {
        codeObjectClass = "Method";
        className = Symbol_AsCString(outer->u.klass.classDef->expr->sym->sym);
        suffix = meta ? ".class" : "";
        switch (stmt->u.method.ns) {
        case METHOD_NAMESPACE_RVALUE: ns = ".0."; break;
        case METHOD_NAMESPACE_LVALUE: ns = ".1."; break;
        }
    } else if (selector == xmain) {
        codeObjectClass = "Function";
        className = "";
        suffix = "";
        ns = "spike.";
        //obj = ".obj";
        //code = "";
    } else {
        codeObjectClass = "Function";
        className = "";
        suffix = "";
        ns = "";
    }
    functionName = Symbol_AsCString(selector);
    
    fprintf(out, "\t.text\n");
    fprintf(out,
            "\t.align\t4\n"
            "%s%s%s%s%s:\n"
            "\t.globl\t%s%s%s%s%s\n"
            "\t.type\t%s%s%s%s%s, @object\n"
            "\t.size\t%s%s%s%s%s, 16\n"
            "\t.long\t%s\n"
            "\t.long\t%lu\n"
            "\t.long\t%lu\n"
            "\t.long\t%lu\n",
            className, suffix, ns, functionName, obj,
            className, suffix, ns, functionName, obj,
            className, suffix, ns, functionName, obj,
            className, suffix, ns, functionName, obj,
            codeObjectClass,
            (unsigned long)cgen->minArgumentCount,
            (unsigned long)(cgen->maxArgumentCount + (cgen->varArgList ? 0x80000000 : 0)),
            (unsigned long)cgen->localCount);
    fprintf(out,
            "%s%s%s%s%s:\n"
            "\t.globl\t%s%s%s%s%s\n"
            "\t.type\t%s%s%s%s%s, @function\n",
            className, suffix, ns, functionName, code,
            className, suffix, ns, functionName, code,
            className, suffix, ns, functionName, code);
    
    prologue(cgen);
    
    _(emitCodeForArgList(stmt, cgen));
    _(emitCodeForStmt(body, &sentinel.label, 0, 0, cgen));
    _(emitCodeForStmt(&sentinel, 0, 0, 0, cgen));
    
    maybeEmitLabel(&cgen->epilogueLabel, cgen);
    epilogue(cgen);
    
    fprintf(out,
            "\t.size\t%s%s%s%s.code, .-%s%s%s%s.code\n"
            "\n",
            className, suffix, ns, functionName,
            className, suffix, ns, functionName);
    
    for (s = body->top; s; s = s->next) {
        switch (s->kind) {
        case STMT_DEF_CLASS:
        case STMT_DEF_METHOD:
            ASSERT(0, "nested class/method not supported by x86 backend");
            break;
        default:
            break;
        }
    }
    
    return GLOBAL(xvoid);
    
 unwind:
    return 0;
}


static Unknown *emitCFunction(Stmt *stmt, CodeGen *cgen) {
    const char *sym, *suffix, *signature;
    FILE *out;
    
    ASSERT(cgen->level == 1, "C functions must be global");
    
    out = cgen->out;
    
    /* XXX: comdat */
    sym = Symbol_AsCString(stmt->u.method.name->sym);
    suffix = ".thunk";
    
    fprintf(out,
            "\t.data\n"
            "\t.align\t4\n"
            "%s%s:\n"
            "\t.type\t%s%s, @object\n",
            sym, suffix, sym, suffix);
    
    /* Currently, the "signature" is simply the return type (used for
       boxing). */
    switch (stmt->expr->specifiers & SPEC_TYPE) {
    case 0:
    case SPEC_TYPE_OBJ:  signature = "Object";  break;
    case SPEC_TYPE_INT:  signature = "Integer"; break;
    case SPEC_TYPE_CHAR: signature = "Char";    break;
    }
    fprintf(out,
            "\t.long\tCFunction\n" /* klass */
            "\t.long\t%s\n" /* signature */
            "\t.long\t%s\n", /* pointer */
            signature,
            sym
        );
    
    fprintf(out,
            "\t.size\t%s%s, .-%s%s\n",
            sym, suffix, sym, suffix);
    fprintf(out, "\n");
    
    return GLOBAL(xvoid);
    
 unwind:
    return 0;
}


/****************************************************************************/
/* classes */

static Unknown *emitCodeForClassBody(Stmt *body, int meta, CodeGen *cgen) {
    Stmt *s;
    Expr *expr;
    
    ASSERT(body->kind == STMT_COMPOUND,
           "compound statement expected");
    
    for (s = body->top; s; s = s->next) {
        switch (s->kind) {
        case STMT_DEF_METHOD:
            if (IS_EXTERN(s->expr)) {
                if (IS_CDECL(s->expr))
                    _(emitCFunction(s, cgen));
            } else {
                _(emitCodeForMethod(s, meta, cgen));
            }
            break;
        case STMT_DEF_VAR:
            if (cgen->level == 1) {
                /* global variable definition */
                FILE *out;
                const char *sym;
                
                out = cgen->out;
                
                fprintf(out, "\t.bss\n");
                fprintf(out, "\t.align\t4\n");
                
                for (expr = s->expr; expr; expr = expr->next) {
                    ASSERT(expr->kind == EXPR_NAME,
                           "initializers not allowed here");
                    if (IS_EXTERN(expr))
                        continue;
                    sym = Symbol_AsCString(expr->sym->sym);
                    fprintf(out,
                            "%s:\n"
                            "\t.globl\t%s\n"
                            "\t.type\t%s, @object\n"
                            "\t.size\t%s, 4\n",
                            sym, sym, sym, sym);
                    fprintf(out, "\t.zero\t4\n");
                }
                fprintf(out, "\n");
            } else {
                for (expr = s->expr; expr; expr = expr->next) {
                    ASSERT(expr->kind == EXPR_NAME,
                           "initializers not allowed here");
                }
            }
            break;
        case STMT_DEF_CLASS:
            _(emitCodeForClass(s, cgen));
            break;
        case STMT_PRAGMA_SOURCE:
            cgen->u.klass.source = s->u.source;
            break;
        default:
            ASSERT(0, "executable code not allowed here");
        }
    }
    
    return GLOBAL(xvoid);
    
unwind:
    return 0;
}

static Unknown *emitCodeForBehaviorObject(Stmt *classDef, int meta, CodeGen *cgen) {
    Stmt *body = meta ? classDef->bottom : classDef->top;
    FILE *out;
    const char *sym, *superSym, *suffix;
    MethodNamespace ns;
    int i;
    int methodTally[NUM_METHOD_NAMESPACES];
    
    out = cgen->out;
    sym = Symbol_AsCString(classDef->expr->sym->sym);
    superSym = classDef->u.klass.superclassName->u.ref.def->u.def.pushOpcode == OPCODE_PUSH_NULL
               ? 0 /* Class 'Object' has no superclass. */
               : Symbol_AsCString(classDef->u.klass.superclassName->sym->sym);
    suffix = meta ? ".class" : "";
    
    for (ns = 0; ns < NUM_METHOD_NAMESPACES; ++ns) {
        Stmt *s;
            
        methodTally[ns] = 0;
        for (s = body ? body->top : 0; s; s = s->next) {
            if (s->kind == STMT_DEF_METHOD &&
                s->u.method.ns == ns)
            {
                ++methodTally[ns];
            }
        }
    }
    
    fprintf(out,
            "\t.data\n"
            "\t.align\t4\n");
    
    fprintf(out,
            "%s%s:\n"
            "\t.globl\t%s%s\n"
            "\t.type\t%s%s, @object\n",
            sym, suffix, sym, suffix, sym, suffix);
    
    if (meta)
        fprintf(out, "\t.long\tMetaclass\n"); /* klass */
    else
        fprintf(out, "\t.long\t%s.class\n", sym); /* klass */
    
    if (superSym) /* The metaclass hierarchy mirrors the class hierarchy. */
        fprintf(out, "\t.long\t%s%s\t/* superclass */\n", superSym, suffix); /* superclass */
    else if (meta) /* The metaclass of 'Object' is a subclass of 'Class'. */
        fprintf(out, "\t.long\tClass\t/* superclass */\n"); /* superclass */
    else /* Class 'Object' has no superclass. */
        fprintf(out, "\t.long\t0\t/* superclass */\n"); /* superclass */
    
#if 0 /* XXX: too complicated; how to hash at compile time? */
    for (ns = 0; ns < NUM_METHOD_NAMESPACES; ++ns) {
        fprintf(out, "\t.long\t0\t/* methodDict[%d] */\n", ns); /* methodDict[ns] */
    }
    for (i = 0; i < NUM_OPER; ++i) {
        fprintf(out, "\t.long\t0\t/* operTable[%d] */\n", i); /* operTable[i] */
    }
    for (i = 0; i < NUM_CALL_OPER; ++i) {
        fprintf(out, "\t.long\t0\t/* operCallTable[%d] */\n", i); /* operCallTable[i] */
    }
    fprintf(out, "\t.long\t0\t/* assignInd */\n"); /* assignInd */
    fprintf(out, "\t.long\t0\t/* assignIndex */\n"); /* assignIndex */
#else
    for (ns = 0; ns < NUM_METHOD_NAMESPACES; ++ns) {
        if (methodTally[ns]) {
            fprintf(out,
                    "\t.long\t%s%s.methodTable.%d\t/* methodTable[%d] */\t\n",
                    sym, suffix, ns, ns); /* methodTable[ns] */
        } else {
            fprintf(out,
                    "\t.long\t0\t/* methodTable[%d] */\t\n",
                    ns); /* methodTable[ns] */
        }
    }
#endif
    /* instVarCount */
    fprintf(out,
            "\t.long\t%lu\t/* instVarCount */\n",
            (unsigned long)
            (meta
             ? classDef->u.klass.classVarCount
             : classDef->u.klass.instVarCount)
        );
    
    if (meta) {
        fprintf(out, "\t.long\t%s\t/* thisClass */\n", sym); /* thisClass */
    } else {
        willEmitSym((Symbol *)classDef->expr->sym->sym, cgen);
        fprintf(out, "\t.long\t__sym_%s\t/* name */\n", sym); /* name */
    }
    
    /*
     * XXX: Class objects need slots for class variables up the
     * superclass chain.  To be totally consistent, and robust with
     * dynamic linking (opaque superclasses), class objects would have
     * to be created at runtime.
     */
    if (!meta)
        fprintf(out, "\t.long\t0,0,0,0,0,0,0,0\t/* XXX class variables */\n");
    
    fprintf(out,
            "\t.size\t%s%s, .-%s%s\n",
            sym, suffix, sym, suffix);
    
    /* A method table is simply an Array of symbol/method key/value pairs. */
    
    for (ns = 0; ns < NUM_METHOD_NAMESPACES; ++ns) {
        Stmt *s;
        
        if (!methodTally[ns])
            continue;
        
        fprintf(out,
                "%s%s.methodTable.%d:\n"
                "\t.globl\t%s%s.methodTable.%d\n"
                "\t.type\t%s%s.methodTable.%d, @object\n",
                sym, suffix, ns,
                sym, suffix, ns,
                sym, suffix, ns);
        
        fprintf(out, "\t.long\tArray\n");
#if 0 /* XXX: more ambivalence */
        willEmitInt(2*methodTally[ns], cgen);
        fprintf(out, "\t.long\t__int_%d\n", 2*methodTally[ns]);
#else
        fprintf(out, "\t.long\t%d\n", 2*methodTally[ns]);
#endif
        
        for (s = body->top; s; s = s->next) {
            if (s->kind == STMT_DEF_METHOD &&
                s->u.method.ns == ns)
            {
                const char *selector = Symbol_AsCString(s->u.method.name->sym);
                willEmitSym((Symbol *)s->u.method.name->sym, cgen);
                fprintf(out,
                        "\t.long\t__sym_%s, %s%s.%d.%s\n",
                        selector,
                        sym, suffix, ns, selector);
                        
            }
        }
        
        fprintf(out,
                "\t.size\t%s%s.methodTable.%d, .-%s%s.methodTable.%d\n",
                sym, suffix, ns,
                sym, suffix, ns);
    }
    
    fprintf(out, "\n");
    
    return GLOBAL(xvoid);
    
unwind:
    return 0;
}

static Unknown *emitCodeForClass(Stmt *stmt, CodeGen *outer) {
    CodeGen gcgen;
    ClassCodeGen *cgen;
    
    /* push class code generator */
    cgen = &gcgen.u.klass;
    cgen->generic = &gcgen;
    cgen->generic->kind = CODE_GEN_CLASS;
    cgen->generic->outer = outer;
    cgen->generic->module = outer->module;
    cgen->classDef = stmt;
    cgen->source = 0;
    cgen->generic->level = outer->level + 1;
    cgen->generic->out = outer->out;
    
    _(emitCodeForBehaviorObject(stmt, 0, cgen->generic));
    _(emitCodeForBehaviorObject(stmt, 1, cgen->generic));
    if (stmt->bottom) {
        _(emitCodeForClassBody(stmt->bottom, 1, cgen->generic));
    }
    _(emitCodeForClassBody(stmt->top, 0, cgen->generic));
    
    return GLOBAL(xvoid);
    
 unwind:
    return 0;
}

/****************************************************************************/

static Unknown *emitCodeForModule(Stmt *stmt, ModuleCodeGen *moduleCodeGen) {
    CodeGen gcgen;
    ClassCodeGen *cgen;
    
    /* push class code generator */
    cgen = &gcgen.u.klass;
    cgen->generic = &gcgen;
    cgen->generic->kind = CODE_GEN_CLASS;
    cgen->generic->outer = 0;
    cgen->generic->module = moduleCodeGen;
    cgen->classDef = stmt;
    cgen->source = 0;
    cgen->generic->level = 1;
    cgen->generic->out = moduleCodeGen->generic->out;
    
    _(emitCodeForClassBody(stmt->top, 0, cgen->generic));
    
    return GLOBAL(xvoid);
    
 unwind:
    return 0;
}

/****************************************************************************/

Unknown *X86CodeGen_GenerateCode(Stmt *tree, FILE *out) {
    Stmt *rootClassList;
    Stmt *s;
    CodeGen gcgen;
    ModuleCodeGen *cgen;
    
    ASSERT(tree->kind == STMT_DEF_MODULE, "module node expected");
    ASSERT(tree->top->kind == STMT_COMPOUND, "compound statement expected");
    
    rootClassList = tree->u.module.rootClassList.first;
    
    memset(&gcgen, 0, sizeof(gcgen));
    cgen = &gcgen.u.module;
    cgen->generic = &gcgen;
    cgen->generic->kind = CODE_GEN_MODULE;
    cgen->generic->out = out;
    cgen->nextLabel = 1;
    
    /* Generate code. */
    _(emitCodeForModule(tree, cgen));
    
    emitROData(cgen);
    
    return GLOBAL(xvoid);
    
 unwind:
    return 0;
}
