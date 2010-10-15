
#include "cgen.h"

#include "heart.h"
#include "host.h"
#include "module.h"
#include "native.h"
#include "obj.h"
#include "rodata.h"
#include "st.h"
#include "tree.h"

#include <stdlib.h>
#include <string.h>


#define ASSERT(c, msg) \
do if (!(c)) { Spk_Halt(Spk_HALT_ASSERTION_ERROR, (msg)); goto unwind; } \
while (0)

#define _(c) do { \
SpkUnknown *_tmp = (c); \
if (!_tmp) goto unwind; \
Spk_DECREF(_tmp); } while (0)

#if DEBUG_STACKP
/* Emit debug opcodes which check to see whether the stack depth at
 * runtime is what the compiler thought it should be.
 */
#define CHECK_STACKP() \
    do { EMIT_OPCODE(Spk_OPCODE_CHECK_STACKP); \
    encodeUnsignedInt(cgen->stackPointer, cgen); } while (0)
#else
#define CHECK_STACKP()
#endif

/* XXX: This determines how far a branch can go, and thus how big a
   method can be. */
#define BRANCH_DISPLACEMENT_SIZE (2)

/* XXX: ditto for the literal table */
#define RODATA_INDEX_SIZE (4)


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
    SpkMethodTmpl *methodTmpl;
} MethodCodeGen;

typedef struct OpcodeGen {
    struct CodeGen *generic;
    
    size_t minArgumentCount, maxArgumentCount; int varArgList;
    size_t localCount;

    size_t currentOffset;
    SpkOpcode *opcodesBegin, *opcodesEnd;
    size_t stackPointer, stackSize;
    unsigned int nMessageSends;
    unsigned int nContextRefs;
    int inLeaf;
    
    size_t currentLineOffset;
    SpkOpcode *lineCodesBegin, *lineCodesEnd;
    size_t currentLineLabel;
    unsigned int currentLineNo;
    
    union {
        BlockCodeGen block;
        MethodCodeGen method;
    } u;
} OpcodeGen;

typedef struct ClassCodeGen {
    struct CodeGen *generic;
    Stmt *classDef;
    SpkBehaviorTmpl *behaviorTmpl;
    SpkUnknown *source;
} ClassCodeGen;

typedef struct ModuleCodeGen {
    struct CodeGen *generic;
    SpkModuleTmpl *moduleTmpl;
    
    SpkUnknown **rodata;
    unsigned int rodataSize, rodataAllocSize;
    SpkUnknown *rodataMap;
} ModuleCodeGen;

typedef struct CodeGen {
    CodeGenKind kind;
    struct CodeGen *outer;
    struct ModuleCodeGen *module;
    unsigned int level;
    union {
        OpcodeGen o;
        ClassCodeGen klass;
        ModuleCodeGen module;
    } u;
} CodeGen;


/****************************************************************************/
/* opcodes */

#define EMIT_OPCODE(opcode) \
do { if (cgen->opcodesEnd) *cgen->opcodesEnd++ = (opcode); \
cgen->currentOffset++; } while (0)

#define SET_STMT_OFFSET(n) \
do { if (cgen->opcodesEnd) \
ASSERT((n)->codeOffset == cgen->currentOffset, "bad code offset"); \
else (n)->codeOffset = cgen->currentOffset; } while (0)

#define SET_EXPR_OFFSET(n) if (setExprOffset((n), cgen)) ; else goto unwind

#define SET_EXPR_END(n) \
do { if (cgen->opcodesEnd) \
ASSERT((n)->endLabel == cgen->currentOffset, "bad end label"); \
else (n)->endLabel = cgen->currentOffset; } while (0)

#define EMIT_LINE_CODE(code) \
do { if (cgen->lineCodesEnd) *cgen->lineCodesEnd++ = (code); \
cgen->currentLineOffset++; } while (0)


static unsigned int getLiteralIndex(SpkUnknown *, OpcodeGen *);

static void encodeUnsignedIntEx(unsigned long value, int which, OpcodeGen *cgen) {
    SpkOpcode byte;
    
    do {
        byte = value & 0x7F;
        value >>= 7;
        if (value) {
            byte |= 0x80;
        }
        switch (which) {
        case 0: EMIT_OPCODE(byte); break;
        case 1: EMIT_LINE_CODE(byte); break;
        }
    } while (value);
}

static void encodeSignedIntEx(long value, int which, OpcodeGen *cgen) {
    int more, negative;
    SpkOpcode byte;
    
    more = 1;
    negative = (value < 0);
    do {
        byte = value & 0x7F;
        value >>= 7;
        if (negative) {
            /* sign extend */
            value |= - (1L << (8 * sizeof(value) - 7));
        }
        if ((value == 0 && !(byte & 0x40)) ||
            ((value == -1 && (byte & 0x40)))) {
            more = 0;
        } else {
            byte |= 0x80;
        }
        switch (which) {
        case 0: EMIT_OPCODE(byte); break;
        case 1: EMIT_LINE_CODE(byte); break;
        }
    } while (more);
}

static void encodeUnsignedInt(unsigned long value, OpcodeGen *cgen) {
    encodeUnsignedIntEx(value, 0, cgen);
}

static void encodeSignedInt(long value, OpcodeGen *cgen) {
    encodeSignedIntEx(value, 0, cgen);
}

static int setExprOffset(Expr *expr, OpcodeGen *cgen) {
    if (cgen->opcodesEnd) {
        ASSERT(expr->codeOffset == cgen->currentOffset, "bad code offset");
    } else {
        expr->codeOffset = cgen->currentOffset;
    }
    
    if (expr->lineNo != cgen->currentLineNo &&
        cgen->currentLineLabel < cgen->currentOffset) {
        size_t codeDelta; long lineDelta;
        
        /* Code offsets always increase; but line numbers might jump
           backwards -- at least in theory -- depending upon the
           evaluation order. */
        codeDelta = cgen->currentOffset - cgen->currentLineLabel;
        lineDelta = (long)expr->lineNo - (long)cgen->currentLineNo;
        encodeUnsignedIntEx(codeDelta, 1, cgen);
        encodeSignedIntEx(lineDelta, 1, cgen);
        cgen->currentLineLabel = cgen->currentOffset;
        cgen->currentLineNo = expr->lineNo;
    }
    return 1;

 unwind:
    return 0;
}

static SpkUnknown *fixUpBranch(ptrdiff_t base, size_t target, OpcodeGen *cgen) {
    ptrdiff_t start, displacement, filler;
    
    start = (ptrdiff_t)cgen->currentOffset;
    displacement = (ptrdiff_t)target - base;
    encodeSignedInt(displacement, cgen);
    
    /* XXX: Don't do this.  Write a real assembler. */
    filler = BRANCH_DISPLACEMENT_SIZE - ((ptrdiff_t)cgen->currentOffset - start);
    ASSERT(0 <= filler && filler < BRANCH_DISPLACEMENT_SIZE,
           "invalid jump");
    if (filler) {
        cgen->opcodesEnd[-1] |= 0x80;
        if (cgen->opcodesEnd[-1] & 0x40) {
            for ( ; filler > 1; --filler) {
                EMIT_OPCODE(0xFF);
            }
            EMIT_OPCODE(0x7F);
        } else {
            for ( ; filler > 1; --filler) {
                EMIT_OPCODE(0x80);
            }
            EMIT_OPCODE(0x00);
        }
    }
    
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static SpkUnknown *emitBranchDisplacement(ptrdiff_t base, size_t target, OpcodeGen *cgen) {
    size_t i;
    
    if (cgen->opcodesEnd) {
        _(fixUpBranch(base, target, cgen));
    } else {
        for (i = 0; i < BRANCH_DISPLACEMENT_SIZE; ++i) {
            EMIT_OPCODE(Spk_OPCODE_NOP);
        }
    }
    
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static SpkUnknown *emitBranch(SpkOpcode opcode, size_t target, OpcodeGen *cgen) {
    ptrdiff_t base;
    
    base = (ptrdiff_t)cgen->currentOffset; /* offset of branch instruction */
    EMIT_OPCODE(opcode);
    if (opcode != Spk_OPCODE_BRANCH_ALWAYS) {
        --cgen->stackPointer;
    }
    return emitBranchDisplacement(base, target, cgen);
}

static void tallyPush(OpcodeGen *cgen) {
    ++cgen->stackPointer;
    if (cgen->stackPointer > cgen->stackSize) {
        cgen->stackSize = cgen->stackPointer;
    }
    CHECK_STACKP();
}

static void dupN(unsigned long n, OpcodeGen *cgen) {
    if (n == 1) {
        EMIT_OPCODE(Spk_OPCODE_DUP); tallyPush(cgen);
        return;
    }
    EMIT_OPCODE(Spk_OPCODE_DUP_N);
    encodeUnsignedInt(n, cgen);
    cgen->stackPointer += n;
    if (cgen->stackPointer > cgen->stackSize) {
        cgen->stackSize = cgen->stackPointer;
    }
    CHECK_STACKP();
}

static SpkUnknown *emitCodeForName(Expr *expr, int *super, OpcodeGen *cgen) {
    Expr *def;
    
    def = expr->u.ref.def;
    ASSERT(def, "missing definition");
    
    if (def->u.def.level == 0) {
        /* built-in */
        ASSERT(def->u.def.pushOpcode, "missing push opcode for built-in");
        switch (def->u.def.pushOpcode) {
        case Spk_OPCODE_PUSH_SUPER:
            ASSERT(super, "invalid use of 'super'");
            *super = 1;
            break;
        case Spk_OPCODE_PUSH_CONTEXT:
            ++cgen->nContextRefs;
            break;
        }
        EMIT_OPCODE(def->u.def.pushOpcode);
        tallyPush(cgen);
        goto leave;
    }
    
    EMIT_OPCODE(def->u.def.pushOpcode);
    encodeUnsignedInt(def->u.def.index, cgen);
    tallyPush(cgen);
    
 leave:
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static SpkUnknown *emitLiteralIndex(SpkUnknown *literal, OpcodeGen *cgen) {
    ptrdiff_t start, filler;
    size_t index;
    
    /* XXX: Unlike with branches, it would easy to figure out in
     * advance how many literals there are, and thus how big the table
     * must be.
     */
    if (cgen->opcodesEnd) {
        start = (ptrdiff_t)cgen->currentOffset;
        
        index = getLiteralIndex(literal, cgen);
        encodeUnsignedInt(index, cgen);
        
        /* XXX: Don't do this.  Write a real assembler. */
        filler = RODATA_INDEX_SIZE - ((ptrdiff_t)cgen->currentOffset - start);
        ASSERT(0 <= filler && filler < RODATA_INDEX_SIZE,
               "too many literals");
        if (filler) {
            cgen->opcodesEnd[-1] |= 0x80;
            for ( ; filler > 1; --filler) {
                EMIT_OPCODE(0x80);
            }
            EMIT_OPCODE(0x00);
        }
    } else {
        for (filler = 0; filler < RODATA_INDEX_SIZE; ++filler) {
            EMIT_OPCODE(Spk_OPCODE_NOP);
        }
    }
    
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static SpkUnknown *emitCodeForLiteral(SpkUnknown *literal, OpcodeGen *cgen) {
    EMIT_OPCODE(Spk_OPCODE_PUSH_LITERAL);
    _(emitLiteralIndex(literal, cgen));
    tallyPush(cgen);
    
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static void store(Expr *var, OpcodeGen *cgen) {
    Expr *def = var->u.ref.def;
    EMIT_OPCODE(def->u.def.storeOpcode);
    encodeUnsignedInt(def->u.def.index, cgen);
}

static void save(OpcodeGen *cgen) {
    size_t variadic = cgen->varArgList ? 1 : 0;
    size_t contextSize =
        cgen->stackSize +
        cgen->maxArgumentCount + variadic +
        cgen->localCount;
    EMIT_OPCODE(Spk_OPCODE_SAVE);
    encodeUnsignedInt(contextSize, cgen);
    encodeUnsignedInt(cgen->stackSize, cgen);
}

static void leaf(OpcodeGen *cgen) {
    EMIT_OPCODE(Spk_OPCODE_LEAF);
}

static int inLeaf(OpcodeGen *cgen) {
    return
        cgen->nMessageSends == 0 &&
        cgen->nContextRefs == 0 &&
        cgen->maxArgumentCount <= Spk_LEAF_MAX_ARGUMENT_COUNT &&
        cgen->stackSize <= Spk_LEAF_MAX_STACK_SIZE &&
        cgen->localCount == 0 &&
        !cgen->varArgList;
}

static void rewindOpcodes(OpcodeGen *cgen,
                          SpkOpcode *opcodes,
                          SpkOpcode *lineCodes) {
    cgen->currentOffset = 0;
    cgen->opcodesBegin = cgen->opcodesEnd = opcodes;
    cgen->stackPointer = cgen->stackSize = 0;
    cgen->nMessageSends = 0;
    cgen->nContextRefs = 0;
    if (!opcodes) {
        cgen->inLeaf = 0;
    }
    cgen->currentLineOffset = 0;
    cgen->lineCodesBegin = cgen->lineCodesEnd = lineCodes;
    cgen->currentLineLabel = 0;
    cgen->currentLineNo = 0;
}


/****************************************************************************/
/* rodata */

static unsigned int getLiteralIndex(SpkUnknown *literal, OpcodeGen *cgen) {
    unsigned int index;
    
    index = SpkHost_InsertLiteral(cgen->generic->module->rodataMap, literal);
    if (index < cgen->generic->module->rodataSize) {
        return index;
    }
    
    ++cgen->generic->module->rodataSize;
    
    if (cgen->generic->module->rodataSize >
        cgen->generic->module->rodataAllocSize) {
        cgen->generic->module->rodataAllocSize
            = (cgen->generic->module->rodataAllocSize
               ? cgen->generic->module->rodataAllocSize * 2
               : 2);
        cgen->generic->module->rodata
            = (SpkUnknown **)realloc(
                cgen->generic->module->rodata,
                cgen->generic->module->rodataAllocSize * sizeof(SpkUnknown *)
                );
    }
    
    cgen->generic->module->rodata[index] = literal;
    Spk_INCREF(literal);
    
    return index;
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
static SpkUnknown *emitBranchForExpr(Expr *expr, int, size_t, size_t, int,
                                     OpcodeGen *);
static SpkUnknown *emitBranchForOneExpr(Expr *, int, size_t, size_t, int,
                                        OpcodeGen *);
static SpkUnknown *inPlaceOp(Expr *, size_t, OpcodeGen *);
static SpkUnknown *inPlaceAttrOp(Expr *, OpcodeGen *);
static SpkUnknown *inPlaceIndexOp(Expr *, OpcodeGen *);
static SpkUnknown *emitCodeForBlock(Expr *, CodeGen *);
static SpkUnknown *emitCodeForStmt(Stmt *, size_t, size_t, size_t, OpcodeGen *);

static SpkUnknown *emitCodeForExpr(Expr *expr, int *super, OpcodeGen *cgen) {
    for ( ; expr->next; expr = expr->next) {
        _(emitCodeForOneExpr(expr, super, cgen));
        EMIT_OPCODE(Spk_OPCODE_POP); --cgen->stackPointer;
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
    SpkOpcode opcode;
    
    if (super) {
        *super = 0;
    }
    SET_EXPR_OFFSET(expr);
    
    switch (expr->kind) {
    case Spk_EXPR_LITERAL:
        _(emitCodeForLiteral(expr->aux.literalValue, cgen));
        break;
    case Spk_EXPR_NAME:
        _(emitCodeForName(expr, super, cgen));
        break;
    case Spk_EXPR_BLOCK:
        ++cgen->nMessageSends;
        /* thisContext.blockCopy(index, argumentCount) { */
        EMIT_OPCODE(Spk_OPCODE_PUSH_CONTEXT);
        tallyPush(cgen);
        _(emitCodeForInt(expr->u.def.index, cgen));
        _(emitCodeForInt(expr->aux.block.argumentCount, cgen));
        EMIT_OPCODE(Spk_OPCODE_SEND_MESSAGE);
        _(emitLiteralIndex(Spk_blockCopy, cgen));
        encodeUnsignedInt(2, cgen);
        cgen->stackPointer -= 2;
        /* } */
        _(emitBranch(Spk_OPCODE_BRANCH_ALWAYS, expr->endLabel, cgen));
        _(emitCodeForBlock(expr, cgen->generic));
        CHECK_STACKP();
        break;
    case Spk_EXPR_COMPOUND:
        EMIT_OPCODE(Spk_OPCODE_PUSH_CONTEXT);
        tallyPush(cgen);
        for (arg = expr->right, argumentCount = 0;
             arg;
             arg = arg->nextArg, ++argumentCount) {
            _(emitCodeForExpr(arg, 0, cgen));
        }
        if (expr->var) {
            _(emitCodeForExpr(expr->var, 0, cgen));
        }
        ++cgen->nMessageSends;
        EMIT_OPCODE(Spk_OPCODE_SEND_MESSAGE);
        _(emitLiteralIndex(Spk_compoundExpression, cgen));
        encodeUnsignedInt(argumentCount, cgen);
        cgen->stackPointer -= argumentCount + 1;
        tallyPush(cgen); /* result */
        CHECK_STACKP();
        break;
    case Spk_EXPR_CALL:
        switch (expr->left->kind) {
        case Spk_EXPR_ATTR:
            _(emitCodeForExpr(expr->left->left, &isSuper, cgen));
            opcode = isSuper
                     ? (expr->var
                        ? Spk_OPCODE_SEND_MESSAGE_SUPER_VA
                        : Spk_OPCODE_SEND_MESSAGE_SUPER)
                     : (expr->var
                        ? Spk_OPCODE_SEND_MESSAGE_VA
                        : Spk_OPCODE_SEND_MESSAGE);
            break;
        case Spk_EXPR_ATTR_VAR:
            _(emitCodeForExpr(expr->left->left, &isSuper, cgen));
            opcode = isSuper
                     ? (expr->var
                        ? Spk_OPCODE_SEND_MESSAGE_SUPER_VAR_VA
                        : Spk_OPCODE_SEND_MESSAGE_SUPER_VAR)
                     : (expr->var
                        ? Spk_OPCODE_SEND_MESSAGE_VAR_VA
                        : Spk_OPCODE_SEND_MESSAGE_VAR);
            break;
        default:
            _(emitCodeForExpr(expr->left, &isSuper, cgen));
            opcode = isSuper
                     ? (expr->var
                        ? Spk_OPCODE_CALL_SUPER_VA
                        : Spk_OPCODE_CALL_SUPER)
                     : (expr->var
                        ? Spk_OPCODE_CALL_VA
                        : Spk_OPCODE_CALL);
            break;
        }
        for (arg = expr->right, argumentCount = 0;
             arg;
             arg = arg->nextArg, ++argumentCount) {
            _(emitCodeForExpr(arg, 0, cgen));
        }
        if (expr->var) {
            _(emitCodeForExpr(expr->var, 0, cgen));
        }
        if (expr->left->kind == Spk_EXPR_ATTR_VAR) {
            _(emitCodeForExpr(expr->left->right, 0, cgen));
        }
        ++cgen->nMessageSends;
        EMIT_OPCODE(opcode);
        switch (expr->left->kind) {
        case Spk_EXPR_ATTR:
            _(emitLiteralIndex(expr->left->sym->sym, cgen));
            break;
        case Spk_EXPR_ATTR_VAR:
            --cgen->stackPointer;
            break;
        default:
            encodeUnsignedInt((unsigned int)expr->oper, cgen);
            break;
        }
        encodeUnsignedInt(argumentCount, cgen);
        cgen->stackPointer -= (expr->var ? 1 : 0) + argumentCount + 1;
        tallyPush(cgen); /* result */
        CHECK_STACKP();
        break;
    case Spk_EXPR_ATTR:
        _(emitCodeForExpr(expr->left, &isSuper, cgen));
        ++cgen->nMessageSends;
        EMIT_OPCODE(isSuper ? Spk_OPCODE_GET_ATTR_SUPER : Spk_OPCODE_GET_ATTR);
        _(emitLiteralIndex(expr->sym->sym, cgen));
        CHECK_STACKP();
        break;
    case Spk_EXPR_ATTR_VAR:
        _(emitCodeForExpr(expr->left, &isSuper, cgen));
        _(emitCodeForExpr(expr->right, 0, cgen));
        ++cgen->nMessageSends;
        EMIT_OPCODE(isSuper
                    ? Spk_OPCODE_GET_ATTR_VAR_SUPER
                    : Spk_OPCODE_GET_ATTR_VAR);
        --cgen->stackPointer;
        CHECK_STACKP();
        break;
    case Spk_EXPR_PREOP:
    case Spk_EXPR_POSTOP:
        switch (expr->left->kind) {
        case Spk_EXPR_NAME:
            _(emitCodeForExpr(expr->left, 0, cgen));
            if (expr->kind == Spk_EXPR_POSTOP) {
                EMIT_OPCODE(Spk_OPCODE_DUP); tallyPush(cgen);
            }
            ++cgen->nMessageSends;
            EMIT_OPCODE(Spk_OPCODE_OPER);
            encodeUnsignedInt((unsigned int)expr->oper, cgen);
            store(expr->left, cgen);
            if (expr->kind == Spk_EXPR_POSTOP) {
                EMIT_OPCODE(Spk_OPCODE_POP); --cgen->stackPointer;
            }
            CHECK_STACKP();
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
        ++cgen->nMessageSends;
        EMIT_OPCODE(isSuper ? Spk_OPCODE_OPER_SUPER : Spk_OPCODE_OPER);
        encodeUnsignedInt((unsigned int)expr->oper, cgen);
        CHECK_STACKP();
        break;
    case Spk_EXPR_BINARY:
        _(emitCodeForExpr(expr->left, &isSuper, cgen));
        _(emitCodeForExpr(expr->right, 0, cgen));
        ++cgen->nMessageSends;
        EMIT_OPCODE(isSuper ? Spk_OPCODE_OPER_SUPER : Spk_OPCODE_OPER);
        encodeUnsignedInt((unsigned int)expr->oper, cgen);
        --cgen->stackPointer;
        CHECK_STACKP();
        break;
    case Spk_EXPR_ID:
    case Spk_EXPR_NI:
        _(emitCodeForExpr(expr->left, 0, cgen));
        _(emitCodeForExpr(expr->right, 0, cgen));
        EMIT_OPCODE(Spk_OPCODE_ID);
        --cgen->stackPointer;
        if (expr->kind == Spk_EXPR_NI) {
            ++cgen->nMessageSends;
            EMIT_OPCODE(Spk_OPCODE_OPER);
            encodeUnsignedInt(Spk_OPER_LNEG, cgen);
        }
        CHECK_STACKP();
        break;
    case Spk_EXPR_AND:
        _(emitBranchForExpr(expr->left, 0, expr->right->endLabel,
                            expr->right->codeOffset, 1, cgen));
        _(emitCodeForExpr(expr->right, 0, cgen));
        CHECK_STACKP();
        break;
    case Spk_EXPR_OR:
        _(emitBranchForExpr(expr->left, 1, expr->right->endLabel,
                            expr->right->codeOffset, 1, cgen));
        _(emitCodeForExpr(expr->right, 0, cgen));
        CHECK_STACKP();
        break;
    case Spk_EXPR_COND:
        _(emitBranchForExpr(expr->cond, 0, expr->right->codeOffset,
                            expr->left->codeOffset, 0, cgen));
        _(emitCodeForExpr(expr->left, 0, cgen));
        _(emitBranch(Spk_OPCODE_BRANCH_ALWAYS, expr->right->endLabel, cgen));
        --cgen->stackPointer;
        _(emitCodeForExpr(expr->right, 0, cgen));
        CHECK_STACKP();
        break;
    case Spk_EXPR_KEYWORD:
        _(emitCodeForExpr(expr->left, &isSuper, cgen));
        for (arg = expr->right, argumentCount = 0;
             arg;
             arg = arg->nextArg, ++argumentCount) {
            _(emitCodeForExpr(arg, 0, cgen));
        }
        ++cgen->nMessageSends;
        EMIT_OPCODE(isSuper
                    ? Spk_OPCODE_SEND_MESSAGE_SUPER
                    : Spk_OPCODE_SEND_MESSAGE);
        _(emitLiteralIndex(expr->aux.keywords, cgen));
        encodeUnsignedInt(argumentCount, cgen);
        cgen->stackPointer -= argumentCount + 1;
        tallyPush(cgen); /* result */
        CHECK_STACKP();
        break;
    case Spk_EXPR_ASSIGN:
        switch (expr->left->kind) {
        case Spk_EXPR_NAME:
            if (expr->oper == Spk_OPER_EQ) {
                _(emitCodeForExpr(expr->right, 0, cgen));
            } else {
                _(emitCodeForExpr(expr->left, 0, cgen));
                _(emitCodeForExpr(expr->right, 0, cgen));
                ++cgen->nMessageSends;
                EMIT_OPCODE(Spk_OPCODE_OPER);
                encodeUnsignedInt((unsigned int)expr->oper, cgen);
                --cgen->stackPointer;
            }
            store(expr->left, cgen);
            CHECK_STACKP();
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
    
    SET_EXPR_END(expr);
    
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static void squirrel(size_t resultDepth, OpcodeGen *cgen) {
    /* duplicate the last result */
    EMIT_OPCODE(Spk_OPCODE_DUP); tallyPush(cgen);
    /* squirrel it away for later */
    EMIT_OPCODE(Spk_OPCODE_ROT);
    encodeUnsignedInt(resultDepth + 1, cgen);
}

static SpkUnknown *inPlaceOp(Expr *expr, size_t resultDepth, OpcodeGen *cgen) {
    if (expr->right) {
        _(emitCodeForExpr(expr->right, 0, cgen));
    } else if (expr->kind == Spk_EXPR_POSTOP) {
        /* e.g., "a[i]++" -- squirrel away the original value */
        squirrel(resultDepth, cgen);
    }
    
    ++cgen->nMessageSends;
    EMIT_OPCODE(Spk_OPCODE_OPER);
    encodeUnsignedInt((unsigned int)expr->oper, cgen);
    if (expr->right) {
        --cgen->stackPointer;
    }
    
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
    if (expr->left->kind == Spk_EXPR_ATTR_VAR) {
        /* get/set common attr */
        _(emitCodeForExpr(expr->left->right, 0, cgen));
        ++argumentCount;
    }
    /* rhs */
    if (expr->oper == Spk_OPER_EQ) {
        _(emitCodeForExpr(expr->right, 0, cgen));
        squirrel(1 + argumentCount + 1 /* receiver, args, new value */, cgen);
    } else {
        dupN(argumentCount + 1, cgen);
        ++cgen->nMessageSends;
        switch (expr->left->kind) {
        case Spk_EXPR_ATTR:
            EMIT_OPCODE(isSuper
                        ? Spk_OPCODE_GET_ATTR_SUPER
                        : Spk_OPCODE_GET_ATTR);
            _(emitLiteralIndex(expr->left->sym->sym, cgen));
            break;
        case Spk_EXPR_ATTR_VAR:
            EMIT_OPCODE(isSuper
                        ? Spk_OPCODE_GET_ATTR_VAR_SUPER
                        : Spk_OPCODE_GET_ATTR_VAR);
            --cgen->stackPointer;
            break;
        default:
            ASSERT(0, "invalid lvalue");
        }
        _(inPlaceOp(expr, 1 + argumentCount + 1, /* receiver, args, result */
                    cgen));
    }
    ++cgen->nMessageSends;
    switch (expr->left->kind) {
    case Spk_EXPR_ATTR:
        EMIT_OPCODE(isSuper
                    ? Spk_OPCODE_SET_ATTR_SUPER
                    : Spk_OPCODE_SET_ATTR);
        _(emitLiteralIndex(expr->left->sym->sym, cgen));
        cgen->stackPointer -= 2;
        break;
    case Spk_EXPR_ATTR_VAR:
        EMIT_OPCODE(isSuper
                    ? Spk_OPCODE_SET_ATTR_VAR_SUPER
                    : Spk_OPCODE_SET_ATTR_VAR);
        cgen->stackPointer -= 3;
        break;
    default:
        ASSERT(0, "invalid lvalue");
    }
    tallyPush(cgen); /* result */
    /* discard 'set' method result, exposing the value that was
       squirrelled away */
    EMIT_OPCODE(Spk_OPCODE_POP); --cgen->stackPointer;
    CHECK_STACKP();
    
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
        ++cgen->nMessageSends;
        EMIT_OPCODE(isSuper ? Spk_OPCODE_CALL_SUPER : Spk_OPCODE_CALL);
        encodeUnsignedInt((unsigned int)expr->left->oper, cgen);
        encodeUnsignedInt(argumentCount, cgen);
        cgen->stackPointer -= argumentCount + 1;
        tallyPush(cgen); /* result */
        /* } get __index__ */
        _(inPlaceOp(expr, 1 + argumentCount + 1, /* receiver, args, result */
                    cgen));
    }
    ++argumentCount; /* new item value */
    ++cgen->nMessageSends;
    EMIT_OPCODE(isSuper ? Spk_OPCODE_SET_INDEX_SUPER : Spk_OPCODE_SET_INDEX);
    encodeUnsignedInt(argumentCount, cgen);
    cgen->stackPointer -= argumentCount + 1;
    tallyPush(cgen); /* result */
    /* discard 'set' method result, exposing the value that was
       squirrelled away */
    EMIT_OPCODE(Spk_OPCODE_POP); --cgen->stackPointer;
    CHECK_STACKP();
    
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static SpkUnknown *emitBranchForExpr(Expr *expr, int cond,
                                     size_t label, size_t fallThroughLabel,
                                     int dup, OpcodeGen *cgen)
{
    for ( ; expr->next; expr = expr->next) {
        _(emitCodeForOneExpr(expr, 0, cgen));
        /* XXX: We could elide this 'pop' if 'expr' is a conditional expr. */
        EMIT_OPCODE(Spk_OPCODE_POP); --cgen->stackPointer;
    }
    _(emitBranchForOneExpr(expr, cond, label, fallThroughLabel, dup, cgen));
    
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static SpkUnknown *emitBranchForOneExpr(Expr *expr, int cond,
                                        size_t label, size_t fallThroughLabel,
                                        int dup, OpcodeGen *cgen)
{
    SpkOpcode pushOpcode;
    
    switch (expr->kind) {
    case Spk_EXPR_NAME:
        pushOpcode = expr->u.ref.def->u.def.pushOpcode;
        if (pushOpcode == Spk_OPCODE_PUSH_FALSE ||
            pushOpcode == Spk_OPCODE_PUSH_TRUE) {
            int killCode = pushOpcode == Spk_OPCODE_PUSH_TRUE ? cond : !cond;
            SET_EXPR_OFFSET(expr);
            if (killCode) {
                if (dup) {
                    EMIT_OPCODE(pushOpcode);
                    tallyPush(cgen);
                    --cgen->stackPointer;
                }
                _(emitBranch(Spk_OPCODE_BRANCH_ALWAYS, label, cgen));
            }
            SET_EXPR_END(expr);
            break;
        } /* else fall through */
    default:
        _(emitCodeForExpr(expr, 0, cgen));
        /*
         * XXX: This sequence could be replaced by a special set of
         * branch-or-pop opcodes.
         */
        if (dup) {
            EMIT_OPCODE(Spk_OPCODE_DUP); tallyPush(cgen);
        }
        _(emitBranch(cond
                     ? Spk_OPCODE_BRANCH_IF_TRUE
                     : Spk_OPCODE_BRANCH_IF_FALSE,
                     label, cgen));
        if (dup) {
            EMIT_OPCODE(Spk_OPCODE_POP); --cgen->stackPointer;
        }
        break;
    case Spk_EXPR_AND:
        SET_EXPR_OFFSET(expr);
        if (cond) {
            /* branch if true */
            _(emitBranchForExpr(expr->left, 0, fallThroughLabel,
                                expr->right->codeOffset, 0, cgen));
            _(emitBranchForExpr(expr->right, 1, label, fallThroughLabel,
                                dup, cgen));
        } else {
            /* branch if false */
            _(emitBranchForExpr(expr->left, 0, label, expr->right->codeOffset,
                                dup, cgen));
            _(emitBranchForExpr(expr->right, 0, label, fallThroughLabel,
                                dup, cgen));
        }
        SET_EXPR_END(expr);
        break;
    case Spk_EXPR_OR:
        SET_EXPR_OFFSET(expr);
        if (cond) {
            /* branch if true */
            _(emitBranchForExpr(expr->left, 1, label, expr->right->codeOffset,
                                dup, cgen));
            _(emitBranchForExpr(expr->right, 1, label, fallThroughLabel,
                                dup, cgen));
        } else {
            /* branch if false */
            _(emitBranchForExpr(expr->left, 1, fallThroughLabel,
                                expr->right->codeOffset, 0, cgen));
            _(emitBranchForExpr(expr->right, 0, label, fallThroughLabel,
                                dup, cgen));
        }
        SET_EXPR_END(expr);
        break;
    case Spk_EXPR_COND:
        SET_EXPR_OFFSET(expr);
        _(emitBranchForExpr(expr->cond, 0, expr->right->codeOffset,
                            expr->left->codeOffset, 0, cgen));
        _(emitBranchForExpr(expr->left, cond, label, fallThroughLabel,
                            dup, cgen));
        _(emitBranch(Spk_OPCODE_BRANCH_ALWAYS, fallThroughLabel, cgen));
        _(emitBranchForExpr(expr->right, cond, label, fallThroughLabel,
                            dup, cgen));
        SET_EXPR_END(expr);
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
    size_t start;
    
    start = cgen->currentOffset;
    for (s = body; s; s = s->next) {
        _(emitCodeForStmt(s, valueExpr->codeOffset, 0, 0, cgen));
    }
    _(emitCodeForExpr(valueExpr, 0, cgen));
    
    /* epilogue */
    EMIT_OPCODE(Spk_OPCODE_RESTORE_CALLER);
    EMIT_OPCODE(Spk_OPCODE_RET);
    _(emitBranch(Spk_OPCODE_BRANCH_ALWAYS, start, cgen));
    
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
    size_t endLabel, stackSize;
    SpkOpcode *opcodesBegin, *lineCodesBegin;
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
    bcg = &gcgen.u.o.u.block;
    bcg->opcodeGen = &cgen->generic->u.o;
    bcg->opcodeGen->generic = cgen->generic;
    cgen->minArgumentCount = expr->aux.block.argumentCount;
    cgen->maxArgumentCount = expr->aux.block.argumentCount;
    cgen->varArgList = 0;
    cgen->localCount = expr->aux.block.localCount;
    
    rewindOpcodes(cgen, 0, 0);
    opcodesBegin = 0;
    lineCodesBegin = 0;
    switch (outer->kind) {
    case CODE_GEN_BLOCK:
    case CODE_GEN_METHOD:
        cgen->currentOffset = outer->u.o.currentOffset;
        if (outer->u.o.opcodesEnd) {
            opcodesBegin = outer->u.o.opcodesEnd;
        }
        cgen->currentLineOffset = outer->u.o.currentLineOffset;
        if (outer->u.o.lineCodesEnd) {
            lineCodesBegin = outer->u.o.lineCodesEnd;
        }
        cgen->currentLineLabel = outer->u.o.currentLineLabel;
        cgen->currentLineNo = outer->u.o.currentLineNo;
        break;
    default:
        ASSERT(0, "block not allowed here");
    }
    
    body = expr->aux.block.stmtList;

    /* dry run to compute offsets */
    _(emitCodeForBlockBody(body, valueExpr, cgen));
    
    if (opcodesBegin) {
        /* now generate code for real */
        
        endLabel = cgen->currentOffset;
        stackSize = cgen->stackSize;
        rewindOpcodes(cgen, opcodesBegin, lineCodesBegin);
        cgen->currentOffset = outer->u.o.currentOffset;
        cgen->currentLineOffset = outer->u.o.currentLineOffset;
        cgen->currentLineLabel = outer->u.o.currentLineLabel;
        cgen->currentLineNo = outer->u.o.currentLineNo;
        
        _(emitCodeForBlockBody(body, valueExpr, cgen));
        
        ASSERT(cgen->currentOffset == endLabel, "bad code size for block");
        ASSERT(cgen->stackSize == stackSize, "bad stack size for block");
        
    }
    
    outer->u.o.currentOffset = cgen->currentOffset;
    outer->u.o.opcodesEnd = cgen->opcodesEnd;
    
    outer->u.o.currentLineOffset = cgen->currentLineOffset;
    outer->u.o.lineCodesEnd = cgen->lineCodesEnd;
    outer->u.o.currentLineLabel = cgen->currentLineLabel;
    outer->u.o.currentLineNo = cgen->currentLineNo;
    
    /* Guarantee that the home context is at least as big as the
     * biggest child block context needs to be.  See
     * Context_blockCopy().
     */
    for (home = outer; home->kind != CODE_GEN_METHOD; home = home->outer)
        ;
    if (cgen->stackPointer > home->u.o.stackSize) {
        home->u.o.stackSize = cgen->stackPointer;
    }
    
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static SpkUnknown *emitCodeForInitializer(Expr *expr, OpcodeGen *cgen) {
    Expr *def;
    
    SET_EXPR_OFFSET(expr);
    
    _(emitCodeForExpr(expr->right, 0, cgen));
    /* similar to store(), but with a definition instead of a
       reference */
    def = expr->left;
    ASSERT(def->kind == Spk_EXPR_NAME, "name expected");
    EMIT_OPCODE(def->u.def.storeOpcode);
    encodeUnsignedInt(def->u.def.index, cgen);
    EMIT_OPCODE(Spk_OPCODE_POP); --cgen->stackPointer;
    CHECK_STACKP();
    
    SET_EXPR_END(expr);
    
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

static SpkUnknown *emitCodeForMethod(Stmt *stmt, CodeGen *cgen);
static SpkUnknown *emitCodeForClass(Stmt *stmt, CodeGen *cgen);

static SpkUnknown *emitCodeForStmt(Stmt *stmt,
                                   size_t parentNextLabel,
                                   size_t breakLabel,
                                   size_t continueLabel,
                                   OpcodeGen *cgen)
{
    Stmt *s;
    size_t nextLabel, childNextLabel, elseLabel;
    
    /* for branching to the next statement in the control flow */
    nextLabel = stmt->next ? stmt->next->codeOffset : parentNextLabel;
    
    SET_STMT_OFFSET(stmt);
    
    switch (stmt->kind) {
    case Spk_STMT_BREAK:
        ASSERT((!cgen->opcodesEnd || breakLabel),
               "break not allowed here");
        _(emitBranch(Spk_OPCODE_BRANCH_ALWAYS, breakLabel, cgen));
        break;
    case Spk_STMT_COMPOUND:
        for (s = stmt->top; s; s = s->next) {
            _(emitCodeForStmt(s, nextLabel, breakLabel, continueLabel, cgen));
        }
        break;
    case Spk_STMT_CONTINUE:
        ASSERT((!cgen->opcodesEnd || continueLabel),
               "continue not allowed here");
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
    case Spk_STMT_DEF_TYPE:
        ASSERT(0, "unexpected type node");
        break;
    case Spk_STMT_DO_WHILE:
        childNextLabel = stmt->expr->codeOffset;
        _(emitCodeForStmt(stmt->top, childNextLabel, nextLabel,
                          childNextLabel, cgen));
        _(emitBranchForExpr(stmt->expr, 1, stmt->codeOffset, nextLabel,
                            0, cgen));
        break;
    case Spk_STMT_EXPR:
        if (stmt->expr) {
            _(emitCodeForExpr(stmt->expr, 0, cgen));
            EMIT_OPCODE(Spk_OPCODE_POP); --cgen->stackPointer;
        }
        break;
    case Spk_STMT_FOR:
        childNextLabel = stmt->incr
                         ? stmt->incr->codeOffset
                         : (stmt->expr
                            ? stmt->expr->codeOffset
                            : stmt->top->codeOffset);
        if (stmt->init) {
            _(emitCodeForExpr(stmt->init, 0, cgen));
            EMIT_OPCODE(Spk_OPCODE_POP); --cgen->stackPointer;
        }
        if (stmt->expr) {
            _(emitBranch(Spk_OPCODE_BRANCH_ALWAYS, stmt->expr->codeOffset,
                         cgen));
        }
        _(emitCodeForStmt(stmt->top, childNextLabel, nextLabel,
                          childNextLabel, cgen));
        if (stmt->incr) {
            _(emitCodeForExpr(stmt->incr, 0, cgen));
            EMIT_OPCODE(Spk_OPCODE_POP); --cgen->stackPointer;
        }
        if (stmt->expr) {
            _(emitBranchForExpr(stmt->expr, 1, stmt->top->codeOffset,
                                nextLabel, 0, cgen));
        } else {
            _(emitBranch(Spk_OPCODE_BRANCH_ALWAYS, stmt->top->codeOffset,
                         cgen));
        }
        break;
    case Spk_STMT_IF_ELSE:
        elseLabel = stmt->bottom ? stmt->bottom->codeOffset : nextLabel;
        _(emitBranchForExpr(stmt->expr, 0, elseLabel, stmt->top->codeOffset,
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
        } else {
            EMIT_OPCODE(Spk_OPCODE_PUSH_VOID);
            tallyPush(cgen);
        }
        EMIT_OPCODE(Spk_OPCODE_RESTORE_SENDER);
        EMIT_OPCODE(Spk_OPCODE_RET);
        --cgen->stackPointer;
        break;
    case Spk_STMT_WHILE:
        childNextLabel = stmt->expr->codeOffset;
        _(emitBranch(Spk_OPCODE_BRANCH_ALWAYS, stmt->expr->codeOffset, cgen));
        _(emitCodeForStmt(stmt->top, childNextLabel, nextLabel,
                          childNextLabel, cgen));
        _(emitBranchForExpr(stmt->expr, 1, stmt->top->codeOffset, nextLabel,
                            0, cgen));
        break;
    case Spk_STMT_YIELD:
        if (stmt->expr) {
            _(emitCodeForExpr(stmt->expr, 0, cgen));
        } else {
            EMIT_OPCODE(Spk_OPCODE_PUSH_VOID);
            tallyPush(cgen);
        }
        EMIT_OPCODE(Spk_OPCODE_RESTORE_CALLER);
        EMIT_OPCODE(Spk_OPCODE_RET);
        --cgen->stackPointer;
        break;
    }
    ASSERT(cgen->stackPointer == 0, "garbage on stack at end of statement");
    CHECK_STACKP();
    
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static SpkUnknown *emitCodeForArgList(Stmt *stmt, OpcodeGen *cgen) {
    ptrdiff_t base;
    
    base = (ptrdiff_t)cgen->currentOffset; /* offset of 'arg' instruction */
    EMIT_OPCODE(cgen->varArgList ? Spk_OPCODE_ARG_VA : Spk_OPCODE_ARG);
    encodeUnsignedInt(cgen->minArgumentCount, cgen);
    encodeUnsignedInt(cgen->maxArgumentCount, cgen);
    
    if (cgen->minArgumentCount < cgen->maxArgumentCount) {
        /* generate code for default argument initializers */
        Expr *arg, *optionalArgList;
        size_t tally, endLabel = 0;
    
        for (arg = stmt->u.method.argList.fixed;
             arg->kind != Spk_EXPR_ASSIGN;
             arg = arg->nextArg)
            ;
        optionalArgList = arg;
        
        /* table of branch displacements */
        for (arg = optionalArgList, tally = 0; arg; arg = arg->nextArg, ++tally) {
            ASSERT(arg->kind == Spk_EXPR_ASSIGN, "assignment expected");
            _(emitBranchDisplacement(base, arg->codeOffset, cgen));
            endLabel = arg->endLabel;
        }
        /* skip label */
        _(emitBranchDisplacement(base, endLabel, cgen));
        ASSERT(tally == cgen->maxArgumentCount - cgen->minArgumentCount,
               "wrong number of default arguments");
        
        /* assignments */
        for (arg = optionalArgList; arg; arg = arg->nextArg) {
            _(emitCodeForInitializer(arg, cgen));
        }
    }
    
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static void insertMethodTmpl(SpkMethodTmpl *methodTmpl,
                             SpkMethodTmplList *list)
{
    if (!list->first) {
        list->first = methodTmpl;
    } else {
        list->last->nextInScope = methodTmpl;
    }
    list->last = methodTmpl;
}

static SpkUnknown *emitCodeForMethod(Stmt *stmt, CodeGen *outer) {
    CodeGen gcgen;
    OpcodeGen *cgen; MethodCodeGen *mcg;
    Stmt *body, *s;
    Stmt sentinel;
    SpkUnknown *selector;
    size_t stackSize;
    
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
    mcg = &gcgen.u.o.u.method;
    mcg->opcodeGen = &cgen->generic->u.o;
    mcg->opcodeGen->generic = cgen->generic;
    mcg->methodTmpl = 0;
    cgen->minArgumentCount = stmt->u.method.minArgumentCount;
    cgen->maxArgumentCount = stmt->u.method.maxArgumentCount;
    cgen->varArgList = stmt->u.method.argList.var ? 1 : 0;
    cgen->localCount = stmt->u.method.localCount;
    rewindOpcodes(cgen, 0, 0);
    
    body = stmt->top;
    ASSERT(body->kind == Spk_STMT_COMPOUND,
           "compound statement expected");
    
    save(cgen); /* XXX: 'stackSize' is zero here */
    
    _(emitCodeForArgList(stmt, cgen)); /* XXX: 'stackSize' is invalid here */
    
    _(emitCodeForStmt(body, sentinel.codeOffset, 0, 0, cgen));
    _(emitCodeForStmt(&sentinel, 0, 0, 0, cgen));
    
    if (inLeaf(cgen)) {
        stackSize = cgen->stackSize;
        
        /* re-compute offsets w/o save & restore */
        rewindOpcodes(cgen, 0, 0);
        cgen->inLeaf = 1;
        leaf(cgen);
        _(emitCodeForArgList(stmt, cgen));
        _(emitCodeForStmt(body, sentinel.codeOffset, 0, 0, cgen));
        _(emitCodeForStmt(&sentinel, 0, 0, 0, cgen));
        
        /* now generate code for real */
        mcg->methodTmpl = (SpkMethodTmpl *)malloc(sizeof(SpkMethodTmpl));
        memset(mcg->methodTmpl, 0, sizeof(SpkMethodTmpl));
        mcg->methodTmpl->bytecodeSize = cgen->currentOffset;
        mcg->methodTmpl->bytecode = (SpkOpcode *)malloc(mcg->methodTmpl->bytecodeSize);
        mcg->methodTmpl->debug.lineCodeTally
            = cgen->currentLineOffset;
        mcg->methodTmpl->debug.lineCodes
            = (SpkOpcode *)malloc(mcg->methodTmpl->debug.lineCodeTally);
        rewindOpcodes(cgen,
                      mcg->methodTmpl->bytecode,
                      mcg->methodTmpl->debug.lineCodes);
        leaf(cgen);
        _(emitCodeForArgList(stmt, cgen));
        _(emitCodeForStmt(body, sentinel.codeOffset, 0, 0, cgen));
        _(emitCodeForStmt(&sentinel, 0, 0, 0, cgen));

    } else {
        stackSize = cgen->stackSize;

        /* now generate code for real */
        mcg->methodTmpl = (SpkMethodTmpl *)malloc(sizeof(SpkMethodTmpl));
        memset(mcg->methodTmpl, 0, sizeof(SpkMethodTmpl));
        mcg->methodTmpl->bytecodeSize = cgen->currentOffset;
        mcg->methodTmpl->bytecode = (SpkOpcode *)malloc(mcg->methodTmpl->bytecodeSize);
        mcg->methodTmpl->debug.lineCodeTally
            = cgen->currentLineOffset;
        mcg->methodTmpl->debug.lineCodes
            = (SpkOpcode *)malloc(mcg->methodTmpl->debug.lineCodeTally);
        rewindOpcodes(cgen,
                      mcg->methodTmpl->bytecode,
                      mcg->methodTmpl->debug.lineCodes);
    
        cgen->stackSize = stackSize;
        save(cgen);
        cgen->stackSize = 0;
        
        _(emitCodeForArgList(stmt, cgen));
        
        _(emitCodeForStmt(body, sentinel.codeOffset, 0, 0, cgen));
        _(emitCodeForStmt(&sentinel, 0, 0, 0, cgen));
    }
    
    { /* find source pathname */
        CodeGen *cg;
        
        for (cg = outer; cg; cg = cg->outer) {
            if (cg->kind == CODE_GEN_CLASS && cg->u.klass.source) {
                Spk_INCREF(cg->u.klass.source);
                mcg->methodTmpl->debug.source = cg->u.klass.source;
                break;
            }
        }
    }

    ASSERT(cgen->currentOffset == mcg->methodTmpl->bytecodeSize,
           "bad code size");
    ASSERT(cgen->stackSize == stackSize,
           "bad stack size");
    
    ASSERT(cgen->currentLineOffset == mcg->methodTmpl->debug.lineCodeTally,
           "bad line code size");
    
    for (s = body->top; s; s = s->next) {
        switch (s->kind) {
        case Spk_STMT_DEF_CLASS:
            _(emitCodeForClass(s, cgen->generic));
            break;
        case Spk_STMT_DEF_METHOD:
            _(emitCodeForMethod(s, cgen->generic));
            break;
        default:
            break;
        }
    }
    
    mcg->methodTmpl->ns = stmt->u.method.ns;
    mcg->methodTmpl->selector = selector;  Spk_INCREF(selector);
    
    switch (outer->kind) {
    case CODE_GEN_METHOD:
        insertMethodTmpl(mcg->methodTmpl,
                         &outer->u.o.u.method.methodTmpl->nestedMethodList);
        break;
    case CODE_GEN_CLASS:
        insertMethodTmpl(mcg->methodTmpl,
                         &outer->u.klass.behaviorTmpl->methodList);
        break;
    default:
        ASSERT(0, "method not allowed here");
    }
    
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static SpkUnknown *emitCodeForClassBody(Stmt *body, CodeGen *cgen) {
    Stmt *s;
    Expr *expr;
    
    ASSERT(body->kind == Spk_STMT_COMPOUND,
           "compound statement expected");
    for (s = body->top; s; s = s->next) {
        switch (s->kind) {
        case Spk_STMT_DEF_METHOD:
            _(emitCodeForMethod(s, cgen));
            break;
        case Spk_STMT_DEF_VAR:
            for (expr = s->expr; expr; expr = expr->next) {
                ASSERT(expr->kind == Spk_EXPR_NAME,
                       "initializers not allowed here");
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

static void insertClassTmpl(SpkClassTmpl *classTmpl,
                            SpkClassTmplList *list)
{
    if (!list->first) {
        list->first = classTmpl;
    } else {
        list->last->nextInScope = classTmpl;
    }
    list->last = classTmpl;
}

static SpkUnknown *emitCodeForClass(Stmt *stmt, CodeGen *outer) {
    CodeGen gcgen;
    ClassCodeGen *cgen;
    SpkClassTmpl *classTmpl;
    
    /* push class code generator */
    cgen = &gcgen.u.klass;
    cgen->generic = &gcgen;
    cgen->generic->kind = CODE_GEN_CLASS;
    cgen->generic->outer = outer;
    cgen->generic->module = outer->module;
    cgen->classDef = stmt;
    cgen->source = 0;
    cgen->generic->level = outer->level + 1;
    
    classTmpl = (SpkClassTmpl *)stmt->expr->u.def.code;
    
    if (stmt->bottom) {
        classTmpl->metaclass.instVarCount = stmt->u.klass.classVarCount;
        cgen->behaviorTmpl = &classTmpl->metaclass;
        _(emitCodeForClassBody(stmt->bottom, cgen->generic));
    }
    classTmpl->thisClass.instVarCount = stmt->u.klass.instVarCount;
    cgen->behaviorTmpl = &classTmpl->thisClass;
    _(emitCodeForClassBody(stmt->top, cgen->generic));
    
    switch (outer->kind) {
    case CODE_GEN_METHOD:
        insertClassTmpl(classTmpl,
                        &outer->u.o.u.method.methodTmpl->nestedClassList);
        break;
    case CODE_GEN_CLASS:
        insertClassTmpl(classTmpl,
                        &outer->u.klass.behaviorTmpl->nestedClassList);
        break;
    default:
        ASSERT(0, "class definition not allowed here");
    }
    
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    return 0;
}

/****************************************************************************/
/* module initialization */

static SpkUnknown *generatePredef(Stmt *stmtList, OpcodeGen *cgen) {
    Stmt *s;
    Expr *def;
    
    /* XXX: This stashes predefined objects in the literal table! */
    
    for (s = stmtList; s; s = s->next) {
        switch (s->kind) {
        case Spk_STMT_DEF_CLASS:
            def = s->expr;
            _(emitCodeForLiteral(def->u.def.initValue, cgen));
            EMIT_OPCODE(def->u.def.storeOpcode);
            encodeUnsignedInt(def->u.def.index, cgen);
            EMIT_OPCODE(Spk_OPCODE_POP); --cgen->stackPointer;
            break;
        case Spk_STMT_DEF_VAR:
            def = s->expr;
            if (def->kind == Spk_EXPR_NAME &&
                def->u.def.initValue) {
                /* predefined variable -- stdin, stdout, stderr */
                _(emitCodeForLiteral(def->u.def.initValue, cgen));
                EMIT_OPCODE(def->u.def.storeOpcode);
                encodeUnsignedInt(def->u.def.index, cgen);
                EMIT_OPCODE(Spk_OPCODE_POP); --cgen->stackPointer;
            }
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

static SpkUnknown *generateInit(Stmt *stmtList, OpcodeGen *cgen) {
    Stmt *s;
    Expr *def;
    
    /* call Module::_init() -- "super._init();" */
    EMIT_OPCODE(Spk_OPCODE_PUSH_SUPER); /* module object */
    tallyPush(cgen);
    ++cgen->nMessageSends;
    EMIT_OPCODE(Spk_OPCODE_SEND_MESSAGE_SUPER);
    _(emitLiteralIndex(Spk__init, cgen));
    encodeUnsignedInt(0, cgen);
    EMIT_OPCODE(Spk_OPCODE_POP); --cgen->stackPointer;
    
    for (s = stmtList; s; s = s->next) {
        switch (s->kind) {
        case Spk_STMT_DEF_CLASS:
            /* handled by Module::_init() */
            break;
        case Spk_STMT_DEF_METHOD:
            if (s->expr->kind == Spk_EXPR_CALL) {
                /* Create a thunk for each global function. */
                def = s->expr->left;
                EMIT_OPCODE(Spk_OPCODE_PUSH_SELF); /* module object */
                tallyPush(cgen);
                _(emitCodeForLiteral(s->u.method.name->sym, cgen));
                ++cgen->nMessageSends;
                EMIT_OPCODE(Spk_OPCODE_SEND_MESSAGE);
                _(emitLiteralIndex(Spk__thunk, cgen));
                encodeUnsignedInt(1, cgen);
                EMIT_OPCODE(def->u.def.storeOpcode);
                encodeUnsignedInt(def->u.def.index, cgen);
                EMIT_OPCODE(Spk_OPCODE_POP); --cgen->stackPointer;
            }
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

/****************************************************************************/
/* method generator */

typedef SpkUnknown *SpkCodeGenerator(Stmt *, OpcodeGen *);

static SpkUnknown *generateMethod(SpkUnknown *selector,
                                  size_t argumentCount,
                                  SpkCodeGenerator *generator,
                                  Stmt *stmtList,
                                  CodeGen *outer)
{
    CodeGen gcgen;
    OpcodeGen *cgen; MethodCodeGen *mcg;
    int run;
    size_t stackSize = 0;
    
    /* push method code generator */
    cgen = &gcgen.u.o;
    cgen->generic = &gcgen;
    cgen->generic->kind = CODE_GEN_METHOD;
    cgen->generic->outer = outer;
    cgen->generic->module = outer->module;
    cgen->generic->level = outer->level + 1;
    mcg = &gcgen.u.o.u.method;
    mcg->opcodeGen = &cgen->generic->u.o;
    mcg->opcodeGen->generic = cgen->generic;
    mcg->methodTmpl = 0;
    cgen->minArgumentCount = argumentCount;
    cgen->maxArgumentCount = argumentCount;
    cgen->varArgList = 0;
    cgen->localCount = 0;
    rewindOpcodes(cgen, 0, 0);
    
    for (run = 0; run < 2; ++run) {
        switch (run) {
        case 0:
            /* dry run to compute offsets */
            stackSize = 0;
            break;
        case 1:
            /* now generate code for real */
            stackSize = cgen->stackSize;
            mcg->methodTmpl = (SpkMethodTmpl *)malloc(sizeof(SpkMethodTmpl));
            memset(mcg->methodTmpl, 0, sizeof(SpkMethodTmpl));
            mcg->methodTmpl->bytecodeSize = cgen->currentOffset;
            mcg->methodTmpl->bytecode = (SpkOpcode *)malloc(mcg->methodTmpl->bytecodeSize);
            rewindOpcodes(cgen, mcg->methodTmpl->bytecode, 0);
            break;
        }
        
        /* save */
        cgen->stackSize = stackSize;
        save(cgen);
        cgen->stackSize = 0;

        EMIT_OPCODE(Spk_OPCODE_ARG);
        encodeUnsignedInt(argumentCount, cgen);
        encodeUnsignedInt(argumentCount, cgen);
        
        _((*generator)(stmtList, cgen));
        
        /* push void */
        EMIT_OPCODE(Spk_OPCODE_PUSH_VOID);
        tallyPush(cgen);
        
        /* restore sender */
        EMIT_OPCODE(Spk_OPCODE_RESTORE_SENDER);
        
        /* ret */
        EMIT_OPCODE(Spk_OPCODE_RET);
    }
    
    ASSERT(cgen->currentOffset == mcg->methodTmpl->bytecodeSize,
           "bad code size");
    ASSERT(cgen->stackSize == stackSize,
           "bad stack size");
    
    ASSERT(outer->kind == CODE_GEN_CLASS,
           "generated code not allowed here");

    mcg->methodTmpl->ns = Spk_METHOD_NAMESPACE_RVALUE;
    mcg->methodTmpl->selector = selector;  Spk_INCREF(selector);
    insertMethodTmpl(mcg->methodTmpl,
                     &outer->u.klass.behaviorTmpl->methodList);
    
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    return 0;
}

/****************************************************************************/

static SpkUnknown *emitCodeForModule(Stmt *stmt, ModuleCodeGen *moduleCodeGen) {
    CodeGen gcgen;
    ClassCodeGen *cgen;
    Stmt *predefList;
    
    /* push class code generator */
    cgen = &gcgen.u.klass;
    cgen->generic = &gcgen;
    cgen->generic->kind = CODE_GEN_CLASS;
    cgen->generic->outer = 0;
    cgen->generic->module = moduleCodeGen;
    cgen->classDef = stmt;
    cgen->behaviorTmpl = &moduleCodeGen->moduleTmpl->moduleClass.thisClass;
    cgen->source = 0;
    cgen->generic->level = 1;
    
    predefList = stmt->u.module.predefList.first;
    
    moduleCodeGen->moduleTmpl->moduleClass.thisClass.instVarCount
        = stmt->u.module.dataSize;
    _(emitCodeForClassBody(stmt->top, cgen->generic));
    _(generateMethod(Spk__predef, 0, &generatePredef,  predefList, cgen->generic));
    _(generateMethod(Spk__init,   0, &generateInit,    stmt->top->top, cgen->generic));
    
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    return 0;
}

/****************************************************************************/

static SpkUnknown *createClass(Stmt *classDef, ModuleCodeGen *cgen) {
    SpkClassTmpl *classTmpl;
    SpkClassTmplList *classList;
    
    classTmpl = (SpkClassTmpl *)malloc(sizeof(SpkClassTmpl));
    memset(classTmpl, 0, sizeof(SpkClassTmpl));
    
    classTmpl->symbol = classDef->expr->sym->sym;
    Spk_INCREF(classTmpl->symbol);
    classTmpl->name = SpkHost_SymbolAsCString(classTmpl->symbol);
    
    classTmpl->classVarIndex = classDef->expr->u.def.index;
    classTmpl->superclassVarIndex = classDef->u.klass.superclassName->u.ref.def->u.def.index;
    
    classTmpl->superclassName = classDef->u.klass.superclassName->sym->sym;
    Spk_INCREF(classTmpl->superclassName);
    
    classList = &cgen->moduleTmpl->classList;
    if (!classList->first) {
        classList->first = classTmpl;
    } else {
        classList->last->next = classTmpl;
    }
    classList->last = classTmpl;
    
    classDef->expr->u.def.code = (void *)classTmpl;
    
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
}

static SpkUnknown *createClassTree(Stmt *classDef, ModuleCodeGen *cgen) {
    /* create class with subclasses in preorder */
    Stmt *subclassDef;
    
    _(createClass(classDef, cgen));
    for (subclassDef = classDef->u.klass.firstSubclassDef;
         subclassDef;
         subclassDef = subclassDef->u.klass.nextSubclassDef) {
        _(createClassTree(subclassDef, cgen));
    }
    
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    return 0;
}

/****************************************************************************/

SpkModuleTmpl *SpkCodeGen_GenerateCode(Stmt *tree) {
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
    
    cgen->rodataMap = SpkHost_NewLiteralDict();
    if (!cgen->rodataMap) {
        goto unwind;
    }
    
    /* Create the module template. */
    cgen->moduleTmpl = (SpkModuleTmpl *)malloc(sizeof(SpkModuleTmpl));
    memset(cgen->moduleTmpl, 0, sizeof(SpkModuleTmpl));
    
    /* Create all class templates. */
    for (s = rootClassList; s; s = s->u.klass.nextRootClassDef) {
        _(createClassTree(s, cgen));
    }

    /* Generate code. */
    _(emitCodeForModule(tree, cgen));
    
    /* Finish initializing the module template. */
    cgen->moduleTmpl->moduleClass.name = "UserModule";
    cgen->moduleTmpl->literals = cgen->rodata;
    cgen->moduleTmpl->literalCount = cgen->rodataSize;
    
    Spk_DECREF(cgen->rodataMap);
    return cgen->moduleTmpl;
    
 unwind:
    Spk_XDECREF(cgen->rodataMap);
    free(cgen->moduleTmpl);
    return 0;
}


/****************************************************************************/

SpkMethod *SpkCodeGen_NewNativeAccessor(unsigned int kind,
                                        unsigned int type,
                                        size_t offset)
{
    CodeGen gcgen;
    OpcodeGen *cgen; MethodCodeGen *mcg;
    int run;
    size_t stackSize = 0;
    SpkMethod *method;
    
    ASSERT(kind == SpkAccessor_READ || kind == SpkAccessor_WRITE,
           "invalid kind");
    
    /* initialize method code generator */
    cgen = &gcgen.u.o;
    cgen->generic = &gcgen;
    cgen->generic->kind = CODE_GEN_METHOD;
    cgen->generic->outer = 0;
    cgen->generic->module = 0;
    cgen->generic->level = 0;
    mcg = &gcgen.u.o.u.method;
    mcg->opcodeGen = &cgen->generic->u.o;
    mcg->opcodeGen->generic = cgen->generic;
    mcg->methodTmpl = 0;
    cgen->varArgList = 0;
    cgen->localCount = 0;
    
    switch (kind) {
    case SpkAccessor_READ:
        cgen->minArgumentCount = 0;
        cgen->maxArgumentCount = 0;
        break;
    case SpkAccessor_WRITE:
        cgen->minArgumentCount = 1;
        cgen->maxArgumentCount = 1;
        break;
    }
    
    rewindOpcodes(cgen, 0, 0);
    
    for (run = 0; run < 2; ++run) {
        switch (run) {
        case 0:
            /* dry run to compute offsets */
            stackSize = 0;
            break;
        case 1:
            /* now generate code for real */
            stackSize = cgen->stackSize;
            ASSERT(stackSize <= Spk_LEAF_MAX_STACK_SIZE,
                   "stack too big for leaf");
            method = SpkMethod_New(cgen->currentOffset);
            rewindOpcodes(cgen, SpkMethod_OPCODES(method), 0);
            break;
        }
        
        /* leaf */
        EMIT_OPCODE(Spk_OPCODE_LEAF);
        
        /* arg */
        EMIT_OPCODE(Spk_OPCODE_ARG);
        encodeUnsignedInt(cgen->minArgumentCount, cgen);
        encodeUnsignedInt(cgen->maxArgumentCount, cgen);
        
        switch (kind) {
        case SpkAccessor_READ:
            /* push result */
            EMIT_OPCODE(Spk_OPCODE_NATIVE_PUSH_INST_VAR);
            encodeUnsignedInt(type, cgen);
            encodeUnsignedInt(offset, cgen);
            tallyPush(cgen);
            break;
            
        case SpkAccessor_WRITE:
            /* push argument variable */
            EMIT_OPCODE(Spk_OPCODE_PUSH_LOCAL);
            encodeUnsignedInt(0, cgen);
            tallyPush(cgen);
            /* store in native field */
            EMIT_OPCODE(Spk_OPCODE_NATIVE_STORE_INST_VAR);
            encodeUnsignedInt(type, cgen);
            encodeUnsignedInt(offset, cgen);
            /* pop */
            EMIT_OPCODE(Spk_OPCODE_POP);
            --cgen->stackPointer;
            
            /* push void result */
            EMIT_OPCODE(Spk_OPCODE_PUSH_VOID);
            tallyPush(cgen);
            
            break;
        }
        
        
        /* restore sender */
        EMIT_OPCODE(Spk_OPCODE_RESTORE_SENDER);
        
        /* ret */
        EMIT_OPCODE(Spk_OPCODE_RET);
    }
    
    ASSERT(cgen->currentOffset == method->base.size,
           "bad method size");
    ASSERT(cgen->stackSize == stackSize,
           "bad stack size");
    
    return method;
    
 unwind:
    return 0;
}
