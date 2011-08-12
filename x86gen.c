
#include "x86gen.h"

#include "heart.h"
#include "host.h"
#include "module.h"
#include "native.h"
#include "obj.h"
#include "rodata.h"
#include "st.h"
#include "str.h"
#include "sym.h"
#include "tree.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>


#define ASSERT(c, msg) \
do if (!(c)) { Spk_Halt(Spk_HALT_ASSERTION_ERROR, (msg)); goto unwind; } \
while (0)

#define _(c) do { \
SpkUnknown *_tmp = (c); \
if (!_tmp) goto unwind; \
Spk_DECREF(_tmp); } while (0)


typedef SpkExprKind ExprKind;
typedef SpkStmtKind StmtKind;
typedef SpkExpr Expr;
typedef SpkExprList ExprList;
typedef SpkArgList ArgList;
typedef SpkStmt Stmt;
typedef SpkStmtList StmtList;


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
    
    Label epilogueLabel;
    
    union {
        BlockCodeGen block;
        MethodCodeGen method;
    } u;
} OpcodeGen;

typedef struct ClassCodeGen {
    struct CodeGen *generic;
    Stmt *classDef;
    SpkUnknown *source;
} ClassCodeGen;

typedef struct ModuleCodeGen {
    struct CodeGen *generic;
    
    Label nextLabel;
    
    long *intData;
    unsigned int intDataSize, intDataAllocSize;
    
    double *floatData;
    unsigned int floatDataSize, floatDataAllocSize;
    
    char charData[256];
    
    SpkString **strData;
    unsigned int strDataSize, strDataAllocSize;
    
    SpkSymbol **symData;
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
static unsigned int willEmitStr(SpkString *, CodeGen *);
static unsigned int willEmitSym(SpkSymbol *, CodeGen *);


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

static SpkUnknown *emitBranch(SpkOpcode opcode, Label *target, OpcodeGen *cgen) {
    switch (opcode) {
    case Spk_OPCODE_BRANCH_IF_FALSE:
        emitOpcode(cgen, "call", "SpikeTest");
        emitOpcode(cgen, "jz", ".L%u", getLabel(target, cgen));
        break;
    case Spk_OPCODE_BRANCH_IF_TRUE:
        emitOpcode(cgen, "call", "SpikeTest");
        emitOpcode(cgen, "jnz", ".L%u", getLabel(target, cgen));
        break;
    case Spk_OPCODE_BRANCH_ALWAYS:
        emitOpcode(cgen, "jmp", ".L%u", getLabel(target, cgen));
        break;
    }
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
}

static void dupN(unsigned long n, OpcodeGen *cgen) {
    unsigned long i;
    
    for (i = 0; i < n; ++i) {
        emitOpcode(cgen, "pushl", "%lu(%%esp)", 4 * (n - 1));
    }
}

static int instVarOffset(Expr *def, OpcodeGen *cgen) {
    return sizeof(SpkObject *) * def->u.def.index;
}

static int localVarOffset(Expr *def, OpcodeGen *cgen) {
    /* XXX: handle variable & default args */
    int index = (int)def->u.def.index;
    if (def->u.def.index < cgen->maxArgumentCount) {
        /* argument */
        /* args are pushed in order, so they are reversed in memory */
        /* skip saved ebp, eip */
        return sizeof(SpkObject *) * (cgen->maxArgumentCount - index - 1 + 2);
    }
    /* ordinary local variable */
    /* skip saved edi, esi, ebx */
    index -= cgen->maxArgumentCount;
    return -(sizeof(SpkObject *) * (index + 1 + 3));
}

static int selfRetOffset(OpcodeGen *cgen) {
    /*
     * Both 'self' and the return value occupy the same slot on the
     * stack.  The "2" accounts for saved %ebp and return address.
     * XXX: handle variable & default args.
     */
    return sizeof(SpkObject *)*(2+cgen->maxArgumentCount);
}

static int isVarDef(Expr *def) {
    return
        def->aux.nameKind == Spk_EXPR_NAME_DEF &&
        def->u.def.stmt &&
        def->u.def.stmt->kind == Spk_STMT_DEF_VAR;
}

static int isVarRef(Expr *expr) {
    return
        expr->aux.nameKind = Spk_EXPR_NAME_REF &&
        isVarDef(expr->u.ref.def);
}

static SpkUnknown *emitCodeForName(Expr *expr, int *super, OpcodeGen *cgen) {
    Expr *def;
    const char *builtin;
    
    def = expr->u.ref.def;
    ASSERT(def, "missing definition");
    
    if (def->u.def.level == 0) {
        /* built-in */
        ASSERT(def->u.def.pushOpcode, "missing push opcode for built-in");
        switch (def->u.def.pushOpcode) {
        case Spk_OPCODE_PUSH_SELF:
            builtin = "%%esi";
            break;
        case Spk_OPCODE_PUSH_SUPER:
            ASSERT(super, "invalid use of 'super'");
            *super = 1;
            builtin = "%%esi";
            break;
        case Spk_OPCODE_PUSH_FALSE:
            builtin = "$false";
            break;
        case Spk_OPCODE_PUSH_TRUE:
            builtin = "$true";
            break;
        case Spk_OPCODE_PUSH_NULL:
            builtin = "$0";
            break;
        case Spk_OPCODE_PUSH_VOID:
            builtin = "$void";
            break;
        case Spk_OPCODE_PUSH_CONTEXT:
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
    case Spk_OPCODE_PUSH_GLOBAL:
        emitOpcode(cgen, "pushl",
                   isVarDef(def) ? "%s%s" : "$%s%s",
                   SpkHost_SymbolAsCString(def->sym->sym),
                   (IS_EXTERN(def) && IS_CDECL(def)) ? ".thunk" : "");
        break;
    case Spk_OPCODE_PUSH_INST_VAR:
        emitOpcode(cgen, "pushl", "%d(%%edi)", instVarOffset(def, cgen));
        break;
    case Spk_OPCODE_PUSH_LOCAL:
        emitOpcode(cgen, "pushl", "%d(%%ebp)", localVarOffset(def, cgen));
        break;
    default:
        ASSERT(0, "unexpected push opcode");
        break;
    }
    
 leave:
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static SpkUnknown *emitCodeForLiteral(SpkUnknown *literal, OpcodeGen *cgen) {
    SpkBehavior *klass;
    
    klass = ((SpkObject *)literal)->klass;
    if (klass == Spk_CLASS(Symbol)) {
        const char *value = SpkHost_SymbolAsCString(literal);
        willEmitSym((SpkSymbol *)literal, cgen->generic);
        emitOpcode(cgen, "pushl", "$__sym_%s", value);
    } else if (klass == Spk_CLASS(Integer)) {
        long value = SpkHost_IntegerAsCLong(literal);
        /* a negative literal int is currently not possible */
        ASSERT(value >= 0, "negative literal int");
        if (value <= 0x1fffffff) {
            emitOpcode(cgen, "pushl", "$%ld", (value << 2) | 0x2);
        } else {
            /* box */
            willEmitInt(value, cgen->generic);
            emitOpcode(cgen, "pushl", "$__int_%ld", value);
        }
    } else if (klass == Spk_CLASS(Float)) {
        double value = SpkHost_FloatAsCDouble(literal);
        unsigned int index = willEmitFloat(value, cgen->generic);
        emitOpcode(cgen, "pushl", "$__float_%u", index);
    } else if (klass == Spk_CLASS(Char)) {
        unsigned int value = (unsigned char)SpkHost_CharAsCChar(literal);
        willEmitChar(value, cgen->generic);
        emitOpcode(cgen, "pushl", "$__char_%02x", value);
    } else if (klass == Spk_CLASS(String)) {
        unsigned int index = willEmitStr((SpkString *)literal, cgen->generic);
        emitOpcode(cgen, "pushl", "$__str_%u", index);
    } else {
        ASSERT(0, "unknown class of literal");
    }
    
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static SpkUnknown *store(Expr *var, OpcodeGen *cgen) {
    Expr *def = var->u.ref.def;
    emitOpcode(cgen, "movl", "(%%esp), %%eax");
    switch (def->u.def.storeOpcode) {
    case Spk_OPCODE_STORE_GLOBAL:
        /* currently, the x86 backend is unique in this regard */
        ASSERT(isVarDef(def), "invalid lvalue");
        emitOpcode(cgen, "movl", "%%eax, %s", SpkHost_SymbolAsCString(def->sym->sym));
        break;
    case Spk_OPCODE_STORE_INST_VAR:
        emitOpcode(cgen, "movl", "%%eax, %d(%%edi)", instVarOffset(def, cgen));
        break;
    case Spk_OPCODE_STORE_LOCAL:
        emitOpcode(cgen, "movl", "%%eax, %d(%%ebp)", localVarOffset(def, cgen));
        break;
    }
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static void pop(OpcodeGen *cgen) {
    emitOpcode(cgen, "popl", "%%eax");
}

static void prologue(OpcodeGen *cgen) {
    /* XXX: account for these */
    size_t variadic = cgen->varArgList ? 1 : 0;
    size_t contextSize =
        cgen->maxArgumentCount + variadic +
        cgen->localCount;
    size_t i;
    
    /* stack frame & registers already set up by SpikeSendMessage glue */
    
#if 0
    /*
     * Allocate and initialize locals.
     * XXX: We could emit a 'loop' opcode for a large numbers of locals.
     */
    for (i = 0; i < cgen->localCount; ++i) {
        emitOpcode(cgen, "pushl", "$0");
    }
#endif
}

static void epilogue(OpcodeGen *cgen) {
    if (cgen->localCount != 0) {
        /* discard locals */
        emitOpcode(cgen, "addl", "$%lu, %%esp", (unsigned long)4*cgen->localCount); 
    }
    /* restore registers */
    emitOpcode(cgen, "popl", "%%edi"); /* instVarPointer */
    emitOpcode(cgen, "popl", "%%esi"); /* self */
    emitOpcode(cgen, "popl", "%%ebx"); /* methodClass */
    
    /* restore caller's stack frame (%ebp) */
    emitOpcode(cgen, "leave", 0);
    
    /* pop arguments and return, leaving result on the stack */
    emitOpcode(cgen, "ret", "$%lu", sizeof(SpkObject *)*cgen->maxArgumentCount);
}

static void oper(SpkOper code, int isSuper, OpcodeGen *cgen) {
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

static unsigned int willEmitStr(SpkString *value, CodeGen *gcgen) {
    ModuleCodeGen *cgen;
    unsigned int i;
    
    cgen = gcgen->module;
    
    for (i = 0; i < cgen->strDataSize; ++i) {
        if (SpkString_IsEqual(cgen->strData[i], value))
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
            = (SpkString **)realloc(
                cgen->strData,
                cgen->strDataAllocSize * sizeof(SpkString *)
                );
    }
    
    cgen->strData[i] = value;
    
    return i;
}

static unsigned int willEmitSym(SpkSymbol *value, CodeGen *gcgen) {
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
            = (SpkSymbol **)realloc(
                cgen->symData,
                cgen->symDataAllocSize * sizeof(SpkSymbol *)
                );
    }
    
    cgen->symData[i] = value;
    
    return i;
}

static void printStringLiteral(SpkString *value, FILE *out) {
    const char *s, *subst;
    char c;
    
    fputc('\"', out);
    s = SpkString_AsCString(value);
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
                (unsigned long)SpkString_Size(cgen->strData[i]));
        printStringLiteral(cgen->strData[i], out);
        fprintf(out,
                "\n"
                "\t.size\t__str_%u, .-__str_%u\n",
                i, i);
    
    }
    
    for (i = 0; i < cgen->symDataSize; ++i) {
        const char *sym = SpkSymbol_AsCString(cgen->symData[i]);
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
                (unsigned long)SpkSymbol_Hash(cgen->symData[i]),
                sym, sym, sym);
    }
    
    fprintf(out, "\n");
}


/****************************************************************************/
/* pseudo-opcodes */

static SpkUnknown *emitCodeForInt(int intValue, OpcodeGen *cgen) {
    SpkUnknown *intObj = 0;
    
    intObj = SpkHost_IntegerFromCLong(intValue);
    if (!intObj) {
        goto unwind;
    }
    _(emitCodeForLiteral(intObj, cgen));
    Spk_DECREF(intObj);
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    Spk_XDECREF(intObj);
    return 0;
}


/****************************************************************************/
/* expressions */

static SpkUnknown *emitCodeForOneExpr(Expr *, int *, OpcodeGen *);
static SpkUnknown *emitBranchForExpr(Expr *expr, int, Label *, Label *, int,
                                     OpcodeGen *);
static SpkUnknown *emitBranchForOneExpr(Expr *, int, Label *, Label *, int,
                                        OpcodeGen *);
static SpkUnknown *inPlaceOp(Expr *, size_t, OpcodeGen *);
static SpkUnknown *inPlaceAttrOp(Expr *, OpcodeGen *);
static SpkUnknown *inPlaceIndexOp(Expr *, OpcodeGen *);
static SpkUnknown *emitCodeForBlock(Expr *, CodeGen *);
static SpkUnknown *emitCodeForStmt(Stmt *, Label *, Label *, Label *, OpcodeGen *);

static SpkUnknown *emitCodeForExpr(Expr *expr, int *super, OpcodeGen *cgen) {
    for ( ; expr->next; expr = expr->next) {
        _(emitCodeForOneExpr(expr, super, cgen));
        pop(cgen);
    }
    _(emitCodeForOneExpr(expr, super, cgen));
    
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static SpkUnknown *emitCodeForOneExpr(Expr *expr, int *super, OpcodeGen *cgen) {
    Expr *arg;
    size_t argumentCount;
    int isSuper;
    const char *routine;
    
    if (super) {
        *super = 0;
    }
    maybeEmitLabel(&expr->label, cgen);
    
    switch (expr->kind) {
    case Spk_EXPR_LITERAL:
        _(emitCodeForLiteral(expr->aux.literalValue, cgen));
        break;
    case Spk_EXPR_NAME:
        _(emitCodeForName(expr, super, cgen));
        break;
    case Spk_EXPR_BLOCK:
        /* thisContext.blockCopy(index, argumentCount) { */
        emitOpcode(cgen, "pushl", "__spike_xxx_push_context"); /* XXX */
        _(emitCodeForInt(expr->u.def.index, cgen));
        _(emitCodeForInt(expr->aux.block.argumentCount, cgen));
        emitOpcode(cgen, "call", "__spike_xxx_blockCopy"); /* XXX */
        /* } */
        _(emitBranch(Spk_OPCODE_BRANCH_ALWAYS, &expr->endLabel, cgen));
        _(emitCodeForBlock(expr, cgen->generic));
        break;
    case Spk_EXPR_COMPOUND:
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
    case Spk_EXPR_CALL:
        /* evaluate receiver */
        switch (expr->left->kind) {
        case Spk_EXPR_ATTR:
        case Spk_EXPR_ATTR_VAR:
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
            emitOpcode(cgen, "call", "SpikePushVarArgs");
        }
        /* evaluate selector */
        switch (expr->left->kind) {
        case Spk_EXPR_ATTR:
            _(emitCodeForLiteral(expr->left->sym->sym, cgen));
            emitOpcode(cgen, "popl", "%%edx");
            routine = "SpikeSendMessage";
            break;
        case Spk_EXPR_ATTR_VAR:
            _(emitCodeForExpr(expr->left->right, 0, cgen));
            emitOpcode(cgen, "popl", "%%edx");
            routine = "SpikeSendMessage";
            break;
        default:
            switch (expr->oper) {
            case Spk_OPER_APPLY:
                routine = "SpikeCall";
                break;
            case Spk_OPER_INDEX:
                routine = "SpikeGetIndex";
                break;
            }
            break;
        }
        /* call RTL routine to send message */
        emitOpcode(cgen, "movl", "$%lu, %%ecx", (unsigned long)argumentCount);
        emitOpcode(cgen, "call", "%s%s", routine, (isSuper ? "Super" : ""));
        break;
    case Spk_EXPR_ATTR:
        _(emitCodeForExpr(expr->left, &isSuper, cgen));
        _(emitCodeForLiteral(expr->sym->sym, cgen));
        emitOpcode(cgen, "call", "SpikeGetAttr%s", (isSuper ? "Super" : ""));
        break;
    case Spk_EXPR_ATTR_VAR:
        _(emitCodeForExpr(expr->left, &isSuper, cgen));
        _(emitCodeForExpr(expr->right, 0, cgen));
        emitOpcode(cgen, "popl", "%%edx");
        emitOpcode(cgen, "call", "SpikeGetAttr%s", (isSuper ? "Super" : ""));
        break;
    case Spk_EXPR_PREOP:
    case Spk_EXPR_POSTOP:
        switch (expr->left->kind) {
        case Spk_EXPR_NAME:
            _(emitCodeForExpr(expr->left, 0, cgen));
            if (expr->kind == Spk_EXPR_POSTOP) {
                dupN(1, cgen);
            }
            oper(expr->oper, 0, cgen);
            _(store(expr->left, cgen));
            if (expr->kind == Spk_EXPR_POSTOP) {
                pop(cgen);
            }
            break;
        case Spk_EXPR_ATTR:
        case Spk_EXPR_ATTR_VAR:
            _(inPlaceAttrOp(expr, cgen));
            break;
        case Spk_EXPR_CALL:
            _(inPlaceIndexOp(expr, cgen));
            break;
        default:
            ASSERT(0, "invalid lvalue");
        }
        break;
    case Spk_EXPR_UNARY:
        _(emitCodeForExpr(expr->left, &isSuper, cgen));
        oper(expr->oper, isSuper, cgen);
        break;
    case Spk_EXPR_BINARY:
        _(emitCodeForExpr(expr->left, &isSuper, cgen));
        _(emitCodeForExpr(expr->right, 0, cgen));
        oper(expr->oper, isSuper, cgen);
        break;
    case Spk_EXPR_ID:
    case Spk_EXPR_NI:
        _(emitCodeForExpr(expr->left, 0, cgen));
        _(emitCodeForExpr(expr->right, 0, cgen));
        pop(cgen);
        emitOpcode(cgen, "cmpl", "(%%esp), %%eax");
        emitOpcode(cgen, "movl", "$false, (%%esp)");
        emitOpcode(cgen,
                   expr->kind == Spk_EXPR_ID ? "jne" : "je",
                   ".L%u", getLabel(&expr->endLabel, cgen));
        emitOpcode(cgen, "movl", "$true, (%%esp)");
        break;
    case Spk_EXPR_AND:
        _(emitBranchForExpr(expr->left, 0, &expr->right->endLabel,
                            &expr->right->label, 1, cgen));
        _(emitCodeForExpr(expr->right, 0, cgen));
        break;
    case Spk_EXPR_OR:
        _(emitBranchForExpr(expr->left, 1, &expr->right->endLabel,
                            &expr->right->label, 1, cgen));
        _(emitCodeForExpr(expr->right, 0, cgen));
        break;
    case Spk_EXPR_COND:
        _(emitBranchForExpr(expr->cond, 0, &expr->right->label,
                            &expr->left->label, 0, cgen));
        _(emitCodeForExpr(expr->left, 0, cgen));
        _(emitBranch(Spk_OPCODE_BRANCH_ALWAYS, &expr->right->endLabel, cgen));
        _(emitCodeForExpr(expr->right, 0, cgen));
        break;
    case Spk_EXPR_KEYWORD:
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
    case Spk_EXPR_ASSIGN:
        switch (expr->left->kind) {
        case Spk_EXPR_NAME:
            if (expr->oper == Spk_OPER_EQ) {
                _(emitCodeForExpr(expr->right, 0, cgen));
            } else {
                _(emitCodeForExpr(expr->left, 0, cgen));
                _(emitCodeForExpr(expr->right, 0, cgen));
                oper(expr->oper, 0, cgen);
            }
            _(store(expr->left, cgen));
            break;
        case Spk_EXPR_ATTR:
        case Spk_EXPR_ATTR_VAR:
            _(inPlaceAttrOp(expr, cgen));
            break;
        case Spk_EXPR_CALL:
            _(inPlaceIndexOp(expr, cgen));
            break;
        default:
            ASSERT(0, "invalid lvalue");
        }
    }
    
    maybeEmitLabel(&expr->endLabel, cgen);
    
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
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

static SpkUnknown *inPlaceOp(Expr *expr, size_t resultDepth, OpcodeGen *cgen) {
    if (expr->right) {
        _(emitCodeForExpr(expr->right, 0, cgen));
    } else if (expr->kind == Spk_EXPR_POSTOP) {
        /* e.g., "a[i]++" -- squirrel away the original value */
        squirrel(resultDepth, cgen);
    }
    
    oper(expr->oper, 0, cgen);
    
    if (expr->kind != Spk_EXPR_POSTOP) {
        /* e.g., "++a[i]" -- squirrel away the new value */
        squirrel(resultDepth, cgen);
    }
    
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static SpkUnknown *inPlaceAttrOp(Expr *expr, OpcodeGen *cgen) {
    size_t argumentCount;
    int isSuper;
    
    argumentCount = 0;
    /* get/set common receiver */
    _(emitCodeForExpr(expr->left->left, &isSuper, cgen));
    /* get/set common attr */
    switch (expr->left->kind) {
    case Spk_EXPR_ATTR:
        _(emitCodeForLiteral(expr->left->sym->sym, cgen));
        ++argumentCount;
        break;
    case Spk_EXPR_ATTR_VAR:
        _(emitCodeForExpr(expr->left->right, 0, cgen));
        ++argumentCount;
        break;
    default:
        ASSERT(0, "invalid lvalue");
    }
    /* rhs */
    if (expr->oper == Spk_OPER_EQ) {
        _(emitCodeForExpr(expr->right, 0, cgen));
        squirrel(1 + argumentCount + 1 /* receiver, args, new value */, cgen);
    } else {
        dupN(argumentCount + 1, cgen);
        switch (expr->left->kind) {
        case Spk_EXPR_ATTR:
        case Spk_EXPR_ATTR_VAR:
            emitOpcode(cgen, "call", "SpikeGetAttr%s", (isSuper ? "Super" : ""));
            break;
        default:
            ASSERT(0, "invalid lvalue");
        }
        _(inPlaceOp(expr, 1 + argumentCount + 1, /* receiver, args, result */
                    cgen));
    }
    switch (expr->left->kind) {
    case Spk_EXPR_ATTR:
    case Spk_EXPR_ATTR_VAR:
        emitOpcode(cgen, "call", "SpikeSetAttr%s", (isSuper ? "Super" : ""));
        break;
    default:
        ASSERT(0, "invalid lvalue");
    }
    /* discard 'set' method result, exposing the value that was
       squirrelled away */
    pop(cgen);
    
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static SpkUnknown *inPlaceIndexOp(Expr *expr, OpcodeGen *cgen) {
    Expr *arg;
    size_t argumentCount;
    int isSuper;
    
    ASSERT(expr->left->oper == Spk_OPER_INDEX, "invalid lvalue");
    /* get/set common receiver */
    _(emitCodeForExpr(expr->left->left, &isSuper, cgen));
    /* get/set common arguments */
    for (arg = expr->left->right, argumentCount = 0;
         arg;
         arg = arg->nextArg, ++argumentCount) {
        _(emitCodeForExpr(arg, 0, cgen));
    }
    /* rhs */
    if (expr->oper == Spk_OPER_EQ) {
        _(emitCodeForExpr(expr->right, 0, cgen));
        squirrel(1 + argumentCount + 1 /* receiver, args, new value */, cgen);
    } else {
        /* get __index__ { */
        dupN(argumentCount + 1, cgen);
        emitOpcode(cgen, "movl", "$%lu, %%ecx", (unsigned long)argumentCount);
        switch (expr->left->oper) {
        case Spk_OPER_APPLY:
            emitOpcode(cgen, "call", "SpikeCall%s", (isSuper ? "Super" : ""));
            break;
        case Spk_OPER_INDEX:
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
    
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static SpkUnknown *emitBranchForExpr(Expr *expr, int cond,
                                     Label *label, Label *fallThroughLabel,
                                     int dup, OpcodeGen *cgen)
{
    for ( ; expr->next; expr = expr->next) {
        _(emitCodeForOneExpr(expr, 0, cgen));
        /* XXX: We could elide this 'pop' if 'expr' is a conditional expr. */
        pop(cgen);
    }
    _(emitBranchForOneExpr(expr, cond, label, fallThroughLabel, dup, cgen));
    
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static SpkUnknown *emitBranchForOneExpr(Expr *expr, int cond,
                                        Label *label, Label *fallThroughLabel,
                                        int dup, OpcodeGen *cgen)
{
    SpkOpcode pushOpcode;
    
    switch (expr->kind) {
    case Spk_EXPR_NAME:
        pushOpcode = expr->u.ref.def->u.def.pushOpcode;
        if (pushOpcode == Spk_OPCODE_PUSH_FALSE ||
            pushOpcode == Spk_OPCODE_PUSH_TRUE) {
            int killCode = pushOpcode == Spk_OPCODE_PUSH_TRUE ? cond : !cond;
            maybeEmitLabel(&expr->label, cgen);
            if (killCode) {
                if (dup) {
                    emitOpcode(cgen, "pushl", pushOpcode == Spk_OPCODE_PUSH_TRUE ? "$true" : "$false");
                }
                _(emitBranch(Spk_OPCODE_BRANCH_ALWAYS, label, cgen));
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
                     ? Spk_OPCODE_BRANCH_IF_TRUE
                     : Spk_OPCODE_BRANCH_IF_FALSE,
                     label, cgen));
        if (dup) {
            pop(cgen);
        }
        break;
    case Spk_EXPR_AND:
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
    case Spk_EXPR_OR:
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
    case Spk_EXPR_COND:
        maybeEmitLabel(&expr->label, cgen);
        _(emitBranchForExpr(expr->cond, 0, &expr->right->label,
                            &expr->left->label, 0, cgen));
        _(emitBranchForExpr(expr->left, cond, label, fallThroughLabel,
                            dup, cgen));
        _(emitBranch(Spk_OPCODE_BRANCH_ALWAYS, fallThroughLabel, cgen));
        _(emitBranchForExpr(expr->right, cond, label, fallThroughLabel,
                            dup, cgen));
        maybeEmitLabel(&expr->endLabel, cgen);
        break;
    }
    
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static SpkUnknown *emitCodeForBlockBody(Stmt *body, Expr *valueExpr,
                                        OpcodeGen *cgen)
{
    Stmt *s;
    Label start;
    
    defineLabel(&start, cgen);
    maybeEmitLabel(&start, cgen);
    for (s = body; s; s = s->next) {
        _(emitCodeForStmt(s, &valueExpr->label, 0, 0, cgen));
    }
    _(emitCodeForExpr(valueExpr, 0, cgen));
    
    /* epilogue */
    /* XXX: Returns to block's caller; has the strange property of
       being resumable. */
    emitOpcode(cgen, "call", "__spike_xxx_block_epilogue"); /* XXX */
    _(emitBranch(Spk_OPCODE_BRANCH_ALWAYS, &start, cgen));
    
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static SpkUnknown *emitCodeForBlock(Expr *expr, CodeGen *outer) {
    CodeGen gcgen;
    OpcodeGen *cgen; BlockCodeGen *bcg;
    Stmt *body;
    Expr *valueExpr, voidDef, voidExpr;
    CodeGen *home;

    memset(&voidDef, 0, sizeof(voidDef));
    memset(&voidExpr, 0, sizeof(voidExpr));
    voidDef.kind = Spk_EXPR_NAME;
    voidDef.sym = 0 /*SpkSymbolNode_FromCString(st, "void")*/ ;
    voidDef.u.def.pushOpcode = Spk_OPCODE_PUSH_VOID;
    voidExpr.kind = Spk_EXPR_NAME;
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
    cgen->epilogueLabel = 0;
    
    switch (outer->kind) {
    case CODE_GEN_BLOCK:
    case CODE_GEN_METHOD:
        break;
    default:
        ASSERT(0, "block not allowed here");
    }
    
    body = expr->aux.block.stmtList;

    _(emitCodeForBlockBody(body, valueExpr, cgen));
    
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static SpkUnknown *emitCodeForInitializer(Expr *expr, OpcodeGen *cgen) {
    Expr *def;
    
    maybeEmitLabel(&expr->label, cgen);
    
    /*
     * XXX: Could this be merged with our goofy prologue code?  Just
     * evaluate the initializer and leave it on the stack. OTOH,
     * out-of-order var refs could yield undefined results, as they
     * access slots below %esp.
     */
    _(emitCodeForExpr(expr->right, 0, cgen));
    /* similar to store(), but with a definition instead of a
       reference */
    def = expr->left;
    ASSERT(def->kind == Spk_EXPR_NAME, "name expected");
    ASSERT(def->u.def.storeOpcode == Spk_OPCODE_STORE_LOCAL, "local variable expected");
    pop(cgen);
    emitOpcode(cgen, "movl", "%%eax, %d(%%ebp)", localVarOffset(def, cgen));
    
    maybeEmitLabel(&expr->endLabel, cgen);
    
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static SpkUnknown *emitCodeForVarDefList(Expr *defList, OpcodeGen *cgen) {
    Expr *expr;
    
    for (expr = defList; expr; expr = expr->next) {
        if (expr->kind == Spk_EXPR_ASSIGN) {
            _(emitCodeForInitializer(expr, cgen));
        }
    }
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    return 0;
}

/****************************************************************************/
/* statements */

static SpkUnknown *emitCodeForMethod(Stmt *stmt, int meta, CodeGen *cgen);
static SpkUnknown *emitCodeForClass(Stmt *stmt, CodeGen *cgen);

static SpkUnknown *emitCodeForStmt(Stmt *stmt,
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
    case Spk_STMT_BREAK:
        ASSERT(breakLabel, "break not allowed here");
        _(emitBranch(Spk_OPCODE_BRANCH_ALWAYS, breakLabel, cgen));
        break;
    case Spk_STMT_COMPOUND:
        for (s = stmt->top; s; s = s->next) {
            _(emitCodeForStmt(s, nextLabel, breakLabel, continueLabel, cgen));
        }
        break;
    case Spk_STMT_CONTINUE:
        ASSERT(continueLabel, "continue not allowed here");
        _(emitBranch(Spk_OPCODE_BRANCH_ALWAYS, continueLabel, cgen));
        break;
    case Spk_STMT_DEF_METHOD:
    case Spk_STMT_DEF_CLASS:
        break;
    case Spk_STMT_DEF_VAR:
        _(emitCodeForVarDefList(stmt->expr, cgen));
        break;
    case Spk_STMT_DEF_MODULE:
        ASSERT(0, "unexpected module node");
        break;
    case Spk_STMT_DEF_SPEC:
        ASSERT(0, "unexpected spec node");
        break;
    case Spk_STMT_DO_WHILE:
        childNextLabel = &stmt->expr->label;
        defineLabel(&stmt->top->label, cgen);
        _(emitCodeForStmt(stmt->top, childNextLabel, nextLabel,
                          childNextLabel, cgen));
        _(emitBranchForExpr(stmt->expr, 1, &stmt->top->label, nextLabel,
                            0, cgen));
        break;
    case Spk_STMT_EXPR:
        if (stmt->expr) {
            _(emitCodeForExpr(stmt->expr, 0, cgen));
            pop(cgen);
        }
        break;
    case Spk_STMT_FOR:
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
            _(emitBranch(Spk_OPCODE_BRANCH_ALWAYS, &stmt->expr->label,
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
            _(emitBranch(Spk_OPCODE_BRANCH_ALWAYS, &stmt->top->label,
                         cgen));
        }
        break;
    case Spk_STMT_IF_ELSE:
        elseLabel = stmt->bottom ? &stmt->bottom->label : nextLabel;
        _(emitBranchForExpr(stmt->expr, 0, elseLabel, &stmt->top->label,
                            0, cgen));
        _(emitCodeForStmt(stmt->top, nextLabel, breakLabel, continueLabel,
                          cgen));
        if (stmt->bottom) {
            _(emitBranch(Spk_OPCODE_BRANCH_ALWAYS, nextLabel, cgen));
            _(emitCodeForStmt(stmt->bottom, nextLabel, breakLabel,
                              continueLabel, cgen));
        }
        break;
    case Spk_STMT_PRAGMA_SOURCE:
        break;
    case Spk_STMT_RETURN:
        if (stmt->expr) {
            _(emitCodeForExpr(stmt->expr, 0, cgen));
            pop(cgen);
            emitOpcode(cgen, "movl", "%%eax, %d(%%ebp)", selfRetOffset(cgen));
        } else {
            emitOpcode(cgen, "movl", "$void, %d(%%ebp)", selfRetOffset(cgen));
        }
        _(emitBranch(Spk_OPCODE_BRANCH_ALWAYS, &cgen->epilogueLabel, cgen));
        break;
    case Spk_STMT_WHILE:
        childNextLabel = &stmt->expr->label;
        _(emitBranch(Spk_OPCODE_BRANCH_ALWAYS, &stmt->expr->label, cgen));
        defineLabel(&stmt->top->label, cgen);
        _(emitCodeForStmt(stmt->top, childNextLabel, nextLabel,
                          childNextLabel, cgen));
        _(emitBranchForExpr(stmt->expr, 1, &stmt->top->label, nextLabel,
                            0, cgen));
        break;
    case Spk_STMT_YIELD:
        if (stmt->expr) {
            _(emitCodeForExpr(stmt->expr, 0, cgen));
        } else {
            emitOpcode(cgen, "pushl", "$void");
        }
        emitOpcode(cgen, "call", "__spike_xxx_yield"); /* XXX */
        break;
    }
    
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    return 0;
}


/****************************************************************************/
/* methods */

static SpkUnknown *emitCodeForArgList(Stmt *stmt, OpcodeGen *cgen) {
    ASSERT(!cgen->varArgList, "XXX var args");
    
    if (cgen->minArgumentCount < cgen->maxArgumentCount) {
        ASSERT(0, "XXX default argument initializers");
    }
    
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static SpkUnknown *emitCodeForMethod(Stmt *stmt, int meta, CodeGen *outer) {
    CodeGen gcgen;
    OpcodeGen *cgen; MethodCodeGen *mcg;
    Stmt *body, *s;
    Stmt sentinel;
    SpkUnknown *selector;
    FILE *out;
    const char *codeObjectClass;
    const char *className, *suffix, *ns, *functionName, *obj, *code;
    
    selector = stmt->u.method.name->sym;
    memset(&sentinel, 0, sizeof(sentinel));
    sentinel.kind = Spk_STMT_RETURN;
    
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
    cgen->localCount = stmt->u.method.localCount;
    cgen->epilogueLabel = 0;
    
    body = stmt->top;
    ASSERT(body->kind == Spk_STMT_COMPOUND,
           "compound statement expected");
    
    out = cgen->generic->out;
    
    obj = "";
    code = ".code";
    
    if (outer->kind == CODE_GEN_CLASS && outer->level != 1) {
        codeObjectClass = "Method";
        className = SpkHost_SymbolAsCString(outer->u.klass.classDef->expr->sym->sym);
        suffix = meta ? ".class" : "";
        switch (stmt->u.method.ns) {
        case Spk_METHOD_NAMESPACE_RVALUE: ns = ".0."; break;
        case Spk_METHOD_NAMESPACE_LVALUE: ns = ".1."; break;
        }
    } else if (selector == Spk_main) {
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
    functionName = SpkHost_SymbolAsCString(selector);
    
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
            (unsigned long)cgen->maxArgumentCount,
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
        case Spk_STMT_DEF_CLASS:
        case Spk_STMT_DEF_METHOD:
            ASSERT(0, "nested class/method not supported by x86 backend");
            break;
        default:
            break;
        }
    }
    
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    return 0;
}


static SpkUnknown *emitCThunk(Stmt *stmt, CodeGen *cgen) {
    const char *sym, *suffix;
    FILE *out;
    
    ASSERT(cgen->level == 1, "C functions must be global");
    
    out = cgen->out;
    
    /* XXX: comdat */
    sym = SpkHost_SymbolAsCString(stmt->u.method.name->sym);
    suffix = ".thunk";
    
    fprintf(out,
            "\t.data\n"
            "\t.align\t4\n"
            "%s%s:\n"
            "\t.type\t%s%s, @object\n",
            sym, suffix, sym, suffix);
    
    fprintf(out,
            "\t.long\tCThunk\n" /* klass */
            "\t.long\tInteger\n" /* signature */
            "\t.long\t%s\n", sym /* pointer */
        );
    
    fprintf(out,
            "\t.size\t%s%s, .-%s%s\n",
            sym, suffix, sym, suffix);
    fprintf(out, "\n");
    
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    return 0;
}


/****************************************************************************/
/* classes */

static SpkUnknown *emitCodeForClassBody(Stmt *body, int meta, CodeGen *cgen) {
    Stmt *s;
    Expr *expr;
    
    ASSERT(body->kind == Spk_STMT_COMPOUND,
           "compound statement expected");
    
    for (s = body->top; s; s = s->next) {
        switch (s->kind) {
        case Spk_STMT_DEF_METHOD:
            if (IS_EXTERN(s->expr)) {
                if (IS_CDECL(s->expr))
                    _(emitCThunk(s, cgen));
            } else {
                _(emitCodeForMethod(s, meta, cgen));
            }
            break;
        case Spk_STMT_DEF_VAR:
            if (cgen->level == 1) {
                /* global variable definition */
                FILE *out;
                const char *sym;
                
                out = cgen->out;
                
                fprintf(out, "\t.bss\n");
                fprintf(out, "\t.align\t4\n");
                
                for (expr = s->expr; expr; expr = expr->next) {
                    ASSERT(expr->kind == Spk_EXPR_NAME,
                           "initializers not allowed here");
                    if (IS_EXTERN(expr))
                        continue;
                    sym = SpkHost_SymbolAsCString(expr->sym->sym);
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
                    ASSERT(expr->kind == Spk_EXPR_NAME,
                           "initializers not allowed here");
                }
            }
            break;
        case Spk_STMT_DEF_CLASS:
            _(emitCodeForClass(s, cgen));
            break;
        case Spk_STMT_PRAGMA_SOURCE:
            Spk_INCREF(s->u.source);
            Spk_XDECREF(cgen->u.klass.source);
            cgen->u.klass.source = s->u.source;
            break;
        default:
            ASSERT(0, "executable code not allowed here");
        }
    }
    
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
unwind:
    return 0;
}

static SpkUnknown *emitCodeForBehaviorObject(Stmt *classDef, int meta, CodeGen *cgen) {
    Stmt *body = meta ? classDef->bottom : classDef->top;
    FILE *out;
    const char *sym, *superSym, *suffix;
    SpkMethodNamespace ns;
    int i;
    int methodTally[Spk_NUM_METHOD_NAMESPACES];
    
    out = cgen->out;
    sym = SpkHost_SymbolAsCString(classDef->expr->sym->sym);
    superSym = classDef->u.klass.superclassName->u.ref.def->u.def.pushOpcode == Spk_OPCODE_PUSH_NULL
               ? 0 /* Class 'Object' has no superclass. */
               : SpkHost_SymbolAsCString(classDef->u.klass.superclassName->sym->sym);
    suffix = meta ? ".class" : "";
    
    for (ns = 0; ns < Spk_NUM_METHOD_NAMESPACES; ++ns) {
        Stmt *s;
            
        methodTally[ns] = 0;
        for (s = body ? body->top : 0; s; s = s->next) {
            if (s->kind == Spk_STMT_DEF_METHOD &&
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
    for (ns = 0; ns < Spk_NUM_METHOD_NAMESPACES; ++ns) {
        fprintf(out, "\t.long\t0\t/* methodDict[%d] */\n", ns); /* methodDict[ns] */
    }
    for (i = 0; i < Spk_NUM_OPER; ++i) {
        fprintf(out, "\t.long\t0\t/* operTable[%d] */\n", i); /* operTable[i] */
    }
    for (i = 0; i < Spk_NUM_CALL_OPER; ++i) {
        fprintf(out, "\t.long\t0\t/* operCallTable[%d] */\n", i); /* operCallTable[i] */
    }
    fprintf(out, "\t.long\t0\t/* assignInd */\n"); /* assignInd */
    fprintf(out, "\t.long\t0\t/* assignIndex */\n"); /* assignIndex */
#else
    for (ns = 0; ns < Spk_NUM_METHOD_NAMESPACES; ++ns) {
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
        willEmitSym((SpkSymbol *)classDef->expr->sym->sym, cgen);
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
    
    for (ns = 0; ns < Spk_NUM_METHOD_NAMESPACES; ++ns) {
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
            if (s->kind == Spk_STMT_DEF_METHOD &&
                s->u.method.ns == ns)
            {
                const char *selector = SpkHost_SymbolAsCString(s->u.method.name->sym);
                willEmitSym((SpkSymbol *)s->u.method.name->sym, cgen);
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
    
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
unwind:
    return 0;
}

static SpkUnknown *emitCodeForClass(Stmt *stmt, CodeGen *outer) {
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
    
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    return 0;
}

/****************************************************************************/

static SpkUnknown *emitCodeForModule(Stmt *stmt, ModuleCodeGen *moduleCodeGen) {
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
    
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    return 0;
}

/****************************************************************************/

SpkUnknown *SpkX86CodeGen_GenerateCode(Stmt *tree, FILE *out) {
    Stmt *rootClassList;
    Stmt *s;
    CodeGen gcgen;
    ModuleCodeGen *cgen;
    
    ASSERT(tree->kind == Spk_STMT_DEF_MODULE, "module node expected");
    ASSERT(tree->top->kind == Spk_STMT_COMPOUND, "compound statement expected");
    
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
    
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    return 0;
}
