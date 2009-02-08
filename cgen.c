
#include "cgen.h"

#include "behavior.h"
#include "class.h"
#include "dict.h"
#include "interp.h"
#include "module.h"
#include "obj.h"
#include "st.h"
#include "sym.h"
#include "tree.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>


#if DEBUG_STACKP
/* Emit debug opcodes which check to see whether the stack depth at
 * runtime is what the compiler thought it should be.
 */
#define CHECK_STACKP() \
    do { EMIT_OPCODE(OPCODE_CHECK_STACKP); \
    encodeUnsignedInt(cgen->stackPointer, cgen); } while (0)
#else
#define CHECK_STACKP()
#endif

#define NESTING 0


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
    Method *methodInstance;
} MethodCodeGen;

typedef struct OpcodeGen {
    struct CodeGen *generic;
    
    size_t argumentCount; int varArgList;
    size_t localCount;

    size_t currentOffset;
    opcode_t *opcodesBegin, *opcodesEnd;
    size_t stackPointer, stackSize;
    unsigned int nMessageSends;
    unsigned int nContextRefs;
    int inLeaf;
    
    union {
        BlockCodeGen block;
        MethodCodeGen method;
    } u;
} OpcodeGen;

typedef struct ClassCodeGen {
    struct CodeGen *generic;
    Stmt *classDef;
    Behavior *classInstance;
} ClassCodeGen;

typedef struct ModuleCodeGen {
    struct CodeGen *generic;
    Behavior *moduleClassInstance;
    
    Behavior *firstClass, *lastClass;
    Object **rodata;
    unsigned int rodataSize, rodataAllocSize;
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

#define SET_OFFSET(n) \
do { if (cgen->opcodesEnd) assert((n)->codeOffset == cgen->currentOffset); \
else (n)->codeOffset = cgen->currentOffset; } while (0)

#define SET_END(n) \
do { if (cgen->opcodesEnd) assert((n)->endLabel == cgen->currentOffset); \
else (n)->endLabel = cgen->currentOffset; } while (0)

static void encodeUnsignedInt(unsigned long value, OpcodeGen *cgen) {
    opcode_t byte;
    
    do {
        byte = value & 0x7F;
        value >>= 7;
        if (value) {
            byte |= 0x80;
        }
        EMIT_OPCODE(byte);
    } while (value);
}

static void encodeSignedInt(long value, OpcodeGen *cgen) {
    int more, negative;
    opcode_t byte;
    
    more = 1;
    negative = (value < 0);
    do {
        byte = value & 0x7F;
        value >>= 7;
        if (negative) {
            /* sign extend */
            value |= - (1 << (8 * sizeof(value) - 7));
        }
        if ((value == 0 && !(byte & 0x40) ||
             (value == -1 && (byte & 0x40)))) {
            more = 0;
        } else {
            byte |= 0x80;
        }
        EMIT_OPCODE(byte);
    } while (more);
}

static void fixUpBranch(size_t target, OpcodeGen *cgen) {
    ptrdiff_t base, displacement, filler;
    
    /* offset of branch instruction */
    base = (ptrdiff_t)(cgen->currentOffset - 1);
    
    displacement = (ptrdiff_t)target - base;
    encodeSignedInt(displacement, cgen);
    
    /* XXX: don't do this */
    filler = 3 - (cgen->currentOffset - base);
    assert(0 <= filler && filler <= 1 && "invalid jump");
    for ( ; filler > 0; --filler) {
        EMIT_OPCODE(OPCODE_NOP);
    }
}

static void emitBranch(opcode_t opcode, size_t target, OpcodeGen *cgen) {
    EMIT_OPCODE(opcode);
    if (cgen->opcodesEnd) {
        fixUpBranch(target, cgen);
    } else {
        EMIT_OPCODE(OPCODE_NOP);
        EMIT_OPCODE(OPCODE_NOP);
    }
    if (opcode != OPCODE_BRANCH_ALWAYS) {
        --cgen->stackPointer;
    }
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
        EMIT_OPCODE(OPCODE_DUP); tallyPush(cgen);
        return;
    }
    EMIT_OPCODE(OPCODE_DUP_N);
    encodeUnsignedInt(n, cgen);
    cgen->stackPointer += n;
    if (cgen->stackPointer > cgen->stackSize) {
        cgen->stackSize = cgen->stackPointer;
    }
    CHECK_STACKP();
}

static void push(Expr *expr, int *super, OpcodeGen *cgen) {
    Expr *def;
    
    def = expr->u.ref.def;
    assert(def);
    
    if (def->u.def.level == 0) {
        /* built-in */
        assert(def->u.def.pushOpcode);
        switch (def->u.def.pushOpcode) {
        case OPCODE_PUSH_SUPER:
            assert(super && "invalid use of 'super'");
            *super = 1;
            break;
        case OPCODE_PUSH_CONTEXT:
            ++cgen->nContextRefs;
            break;
        }
        EMIT_OPCODE(def->u.def.pushOpcode);
        tallyPush(cgen);
        return;
    }
    
    if (NESTING) {
        /* arbitrary nesting version */
        size_t activation;
        
        assert(cgen->generic->level >= def->u.def.level);
        activation = cgen->generic->level - def->u.def.level;
        EMIT_OPCODE(OPCODE_PUSH);
        encodeUnsignedInt(activation, cgen);
        encodeUnsignedInt(def->u.def.index, cgen);
        tallyPush(cgen);
        return;
    }
    
    EMIT_OPCODE(def->u.def.pushOpcode);
    encodeUnsignedInt(def->u.def.index, cgen);
    tallyPush(cgen);
}

static void store(Expr *var, OpcodeGen *cgen) {
    Expr *def;
    
    def = var->u.ref.def;
    assert(def);
    
    if (NESTING) {
        /* arbitrary nesting version */
        size_t activation;
        
        assert(cgen->generic->level >= def->u.def.level);
        activation = cgen->generic->level - def->u.def.level;
        EMIT_OPCODE(OPCODE_STORE);
        encodeUnsignedInt(activation, cgen);
        encodeUnsignedInt(def->u.def.index, cgen);
        return;
    }
    
    EMIT_OPCODE(def->u.def.storeOpcode);
    encodeUnsignedInt(def->u.def.index, cgen);
}

static void save(OpcodeGen *cgen) {
    EMIT_OPCODE(cgen->varArgList ? OPCODE_SAVE_VAR : OPCODE_SAVE);
    encodeUnsignedInt(cgen->generic->level, cgen);
    encodeUnsignedInt(cgen->argumentCount, cgen);
    encodeUnsignedInt(cgen->localCount, cgen);
    encodeUnsignedInt(cgen->stackSize, cgen);
}

static void leaf(OpcodeGen *cgen) {
    EMIT_OPCODE(OPCODE_LEAF);
    encodeUnsignedInt(cgen->argumentCount, cgen);
}

static int inLeaf(OpcodeGen *cgen) {
    return
        cgen->nMessageSends == 0 &&
        cgen->nContextRefs == 0 &&
        cgen->argumentCount <= LEAF_MAX_ARGUMENT_COUNT &&
        cgen->stackSize <= LEAF_MAX_STACK_SIZE &&
        cgen->localCount == 0 &&
        !cgen->varArgList;
}

static void rewind(OpcodeGen *cgen, opcode_t *opcodes) {
    cgen->currentOffset = 0;
    cgen->opcodesBegin = cgen->opcodesEnd = opcodes;
    cgen->stackPointer = cgen->stackSize = 0;
    cgen->nMessageSends = 0;
    cgen->nContextRefs = 0;
    if (!opcodes) {
        cgen->inLeaf = 0;
    }
}


/****************************************************************************/
/* rodata */

static unsigned int getLiteralIndex(Object *literal, OpcodeGen *cgen) {
    unsigned int index;
    
    for (index = 0; index < cgen->generic->module->rodataSize; ++index) {
        /* XXX: This only works properly for Symbols. */
        if (cgen->generic->module->rodata[index] == literal) {
            return index;
        }
    }
    
    index = cgen->generic->module->rodataSize++;
    
    if (cgen->generic->module->rodataSize > cgen->generic->module->rodataAllocSize) {
        cgen->generic->module->rodataAllocSize = cgen->generic->module->rodataAllocSize ? cgen->generic->module->rodataAllocSize * 2 : 2;
        cgen->generic->module->rodata = (Object **)realloc(cgen->generic->module->rodata, cgen->generic->module->rodataAllocSize * sizeof(Object *));
    }
    
    cgen->generic->module->rodata[index] = literal;
    return index;
}

#define LITERAL_INDEX(literal, cgen) ((cgen)->opcodesEnd ? getLiteralIndex(literal, cgen) : 0)


/****************************************************************************/
/* expressions */

static void emitCodeForOneExpr(Expr *, int *, OpcodeGen *);
static void emitBranchForExpr(Expr *expr, int, size_t, size_t, int, OpcodeGen *);
static void emitBranchForOneExpr(Expr *, int, size_t, size_t, int, OpcodeGen *);
static void inPlaceOp(Expr *, size_t, OpcodeGen *);
static void inPlaceAttrOp(Expr *, OpcodeGen *);
static void inPlaceIndexOp(Expr *, OpcodeGen *);
static void emitCodeForBlock(Expr *, CodeGen *);
static void emitCodeForStmt(Stmt *, size_t, size_t, size_t, OpcodeGen *);

static void emitCodeForExpr(Expr *expr, int *super, OpcodeGen *cgen) {
    for ( ; expr->next; expr = expr->next) {
        emitCodeForOneExpr(expr, super, cgen);
        EMIT_OPCODE(OPCODE_POP); --cgen->stackPointer;
    }
    emitCodeForOneExpr(expr, super, cgen);
}

static void emitCodeForOneExpr(Expr *expr, int *super, OpcodeGen *cgen) {
    Expr *arg;
    size_t index, argumentCount;
    int isSuper;
    
    if (super) {
        *super = 0;
    }
    SET_OFFSET(expr);
    
    switch (expr->kind) {
    case EXPR_INT:
        EMIT_OPCODE(OPCODE_PUSH_INT);
        encodeSignedInt(expr->aux.lit.intValue, cgen);
        tallyPush(cgen);
        break;
    case EXPR_FLOAT:
        EMIT_OPCODE(OPCODE_PUSH_LITERAL);
        index = LITERAL_INDEX((Object *)expr->aux.lit.floatValue, cgen);
        encodeUnsignedInt(index, cgen);
        tallyPush(cgen);
        break;
    case EXPR_CHAR:
        EMIT_OPCODE(OPCODE_PUSH_LITERAL);
        index = LITERAL_INDEX((Object *)expr->aux.lit.charValue, cgen);
        encodeUnsignedInt(index, cgen);
        tallyPush(cgen);
        break;
    case EXPR_STR:
        EMIT_OPCODE(OPCODE_PUSH_LITERAL);
        index = LITERAL_INDEX((Object *)expr->aux.lit.strValue, cgen);
        encodeUnsignedInt(index, cgen);
        tallyPush(cgen);
        break;
    case EXPR_NAME:
        push(expr, super, cgen);
        break;
    case EXPR_SYMBOL:
        EMIT_OPCODE(OPCODE_PUSH_LITERAL);
        index = LITERAL_INDEX((Object *)expr->sym->sym, cgen);
        encodeUnsignedInt(index, cgen);
        tallyPush(cgen);
        break;
    case EXPR_BLOCK:
        ++cgen->nMessageSends;
        /* thisContext.blockCopy(index, argumentCount) { */
        EMIT_OPCODE(OPCODE_PUSH_CONTEXT);
        tallyPush(cgen);
        EMIT_OPCODE(OPCODE_GET_ATTR);
        index = LITERAL_INDEX((Object *)SpkSymbol_get("blockCopy"), cgen);
        encodeUnsignedInt(index, cgen);
        EMIT_OPCODE(OPCODE_PUSH_INT);
        encodeSignedInt(expr->u.def.index, cgen);
        tallyPush(cgen);
        EMIT_OPCODE(OPCODE_PUSH_INT);
        encodeSignedInt(expr->aux.block.argumentCount, cgen);
        tallyPush(cgen);
        EMIT_OPCODE(OPCODE_CALL);
        encodeUnsignedInt(METHOD_NAMESPACE_RVALUE, cgen);
        encodeUnsignedInt(OPER_APPLY, cgen);
        encodeUnsignedInt(2, cgen);
        cgen->stackPointer -= 2;
        /* } */
        emitBranch(OPCODE_BRANCH_ALWAYS, expr->endLabel, cgen);
        emitCodeForBlock(expr, cgen->generic);
        CHECK_STACKP();
        break;
    case EXPR_CALL:
        emitCodeForExpr(expr->left, &isSuper, cgen);
        for (arg = expr->right, argumentCount = 0;
             arg;
             arg = arg->nextArg, ++argumentCount) {
            emitCodeForExpr(arg, 0, cgen);
        }
        if (expr->var) {
            emitCodeForExpr(expr->var, 0, cgen);
        }
        ++cgen->nMessageSends;
        EMIT_OPCODE(isSuper
                    ? (expr->var
                       ? OPCODE_CALL_SUPER_VAR
                       : OPCODE_CALL_SUPER)
                    : (expr->var
                       ? OPCODE_CALL_VAR
                       : OPCODE_CALL));
        encodeUnsignedInt(METHOD_NAMESPACE_RVALUE, cgen);
        encodeUnsignedInt((unsigned int)expr->oper, cgen);
        encodeUnsignedInt(argumentCount, cgen);
        cgen->stackPointer -= (expr->var ? 1 : 0) + argumentCount + 1;
        tallyPush(cgen); /* result */
        CHECK_STACKP();
        break;
    case EXPR_ATTR:
        emitCodeForExpr(expr->left, &isSuper, cgen);
        ++cgen->nMessageSends;
        EMIT_OPCODE(isSuper ? OPCODE_GET_ATTR_SUPER : OPCODE_GET_ATTR);
        index = LITERAL_INDEX((Object *)expr->sym->sym, cgen);
        encodeUnsignedInt(index, cgen);
        CHECK_STACKP();
        break;
    case EXPR_ATTR_VAR:
        emitCodeForExpr(expr->left, &isSuper, cgen);
        emitCodeForExpr(expr->right, 0, cgen);
        ++cgen->nMessageSends;
        EMIT_OPCODE(isSuper ? OPCODE_GET_ATTR_VAR_SUPER : OPCODE_GET_ATTR_VAR);
        --cgen->stackPointer;
        CHECK_STACKP();
        break;
    case EXPR_PREOP:
    case EXPR_POSTOP:
        switch (expr->left->kind) {
        case EXPR_NAME:
            emitCodeForExpr(expr->left, 0, cgen);
            if (expr->kind == EXPR_POSTOP) {
                EMIT_OPCODE(OPCODE_DUP); tallyPush(cgen);
            }
            ++cgen->nMessageSends;
            EMIT_OPCODE(OPCODE_OPER);
            encodeUnsignedInt((unsigned int)expr->oper, cgen);
            store(expr->left, cgen);
            if (expr->kind == EXPR_POSTOP) {
                EMIT_OPCODE(OPCODE_POP); --cgen->stackPointer;
            }
            CHECK_STACKP();
            break;
        case EXPR_ATTR:
        case EXPR_ATTR_VAR:
            inPlaceAttrOp(expr, cgen);
            break;
        case EXPR_CALL:
            inPlaceIndexOp(expr, cgen);
            break;
        default:
            assert(0 && "invalid lvalue");
        }
        break;
    case EXPR_UNARY:
        emitCodeForExpr(expr->left, &isSuper, cgen);
        ++cgen->nMessageSends;
        EMIT_OPCODE(isSuper ? OPCODE_OPER_SUPER : OPCODE_OPER);
        encodeUnsignedInt((unsigned int)expr->oper, cgen);
        CHECK_STACKP();
        break;
    case EXPR_BINARY:
        emitCodeForExpr(expr->left, &isSuper, cgen);
        emitCodeForExpr(expr->right, 0, cgen);
        ++cgen->nMessageSends;
        EMIT_OPCODE(isSuper ? OPCODE_OPER_SUPER : OPCODE_OPER);
        encodeUnsignedInt((unsigned int)expr->oper, cgen);
        --cgen->stackPointer;
        CHECK_STACKP();
        break;
    case EXPR_ID:
    case EXPR_NI:
        emitCodeForExpr(expr->left, 0, cgen);
        emitCodeForExpr(expr->right, 0, cgen);
        EMIT_OPCODE(OPCODE_ID);
        --cgen->stackPointer;
        if (expr->kind == EXPR_NI) {
            ++cgen->nMessageSends;
            EMIT_OPCODE(OPCODE_OPER);
            encodeUnsignedInt(OPER_LNEG, cgen);
        }
        CHECK_STACKP();
        break;
    case EXPR_AND:
        emitBranchForExpr(expr->left, 0, expr->right->endLabel, expr->right->codeOffset, 1, cgen);
        emitCodeForExpr(expr->right, 0, cgen);
        CHECK_STACKP();
        break;
    case EXPR_OR:
        emitBranchForExpr(expr->left, 1, expr->right->endLabel, expr->right->codeOffset, 1, cgen);
        emitCodeForExpr(expr->right, 0, cgen);
        CHECK_STACKP();
        break;
    case EXPR_COND:
        emitBranchForExpr(expr->cond, 0, expr->right->codeOffset, expr->left->codeOffset, 0, cgen);
        emitCodeForExpr(expr->left, 0, cgen);
        emitBranch(OPCODE_BRANCH_ALWAYS, expr->right->endLabel, cgen);
        --cgen->stackPointer;
        emitCodeForExpr(expr->right, 0, cgen);
        CHECK_STACKP();
        break;
    case EXPR_ASSIGN:
        switch (expr->left->kind) {
        case EXPR_NAME:
            if (expr->oper == OPER_EQ) {
                emitCodeForExpr(expr->right, 0, cgen);
            } else {
                emitCodeForExpr(expr->left, 0, cgen);
                emitCodeForExpr(expr->right, 0, cgen);
                ++cgen->nMessageSends;
                EMIT_OPCODE(OPCODE_OPER);
                encodeUnsignedInt((unsigned int)expr->oper, cgen);
                --cgen->stackPointer;
            }
            store(expr->left, cgen);
            CHECK_STACKP();
            break;
        case EXPR_ATTR:
        case EXPR_ATTR_VAR:
            inPlaceAttrOp(expr, cgen);
            break;
        case EXPR_CALL:
            inPlaceIndexOp(expr, cgen);
            break;
        default:
            assert(0 && "invalid lvalue");
        }
    }
    
    SET_END(expr);
    
    return;
}

static void squirrel(size_t resultDepth, OpcodeGen *cgen) {
    /* duplicate the last result */
    EMIT_OPCODE(OPCODE_DUP); tallyPush(cgen);
    /* squirrel it away for later */
    EMIT_OPCODE(OPCODE_ROT);
    encodeUnsignedInt(resultDepth + 1, cgen);
}

static void inPlaceOp(Expr *expr, size_t resultDepth, OpcodeGen *cgen) {
    if (expr->right) {
        emitCodeForExpr(expr->right, 0, cgen);
    } else if (expr->kind == EXPR_POSTOP) {
        /* e.g., "a[i]++" -- squirrel away the original value */
        squirrel(resultDepth, cgen);
    }
    
    ++cgen->nMessageSends;
    EMIT_OPCODE(OPCODE_OPER);
    encodeUnsignedInt((unsigned int)expr->oper, cgen);
    if (expr->right) {
        --cgen->stackPointer;
    }
    
    if (expr->kind != EXPR_POSTOP) {
        /* e.g., "++a[i]" -- squirrel away the new value */
        squirrel(resultDepth, cgen);
    }
}

static void inPlaceAttrOp(Expr *expr, OpcodeGen *cgen) {
    size_t index, argumentCount;
    int isSuper;
    
    argumentCount = 0;
    /* get/set common receiver */
    emitCodeForExpr(expr->left->left, &isSuper, cgen);
    if (expr->left->kind == EXPR_ATTR_VAR) {
        /* get/set common attr */
        emitCodeForExpr(expr->left->right, 0, cgen);
        ++argumentCount;
    }
    /* rhs */
    if (expr->oper == OPER_EQ) {
        emitCodeForExpr(expr->right, 0, cgen);
        squirrel(1 + argumentCount + 1 /* receiver, args, new value */, cgen);
    } else {
        dupN(argumentCount + 1, cgen);
        ++cgen->nMessageSends;
        switch (expr->left->kind) {
        case EXPR_ATTR:
            EMIT_OPCODE(isSuper ? OPCODE_GET_ATTR_SUPER : OPCODE_GET_ATTR);
            index = LITERAL_INDEX((Object *)expr->left->sym->sym, cgen);
            encodeUnsignedInt(index, cgen);
            break;
        case EXPR_ATTR_VAR:
            EMIT_OPCODE(isSuper ? OPCODE_GET_ATTR_VAR_SUPER : OPCODE_GET_ATTR_VAR);
            --cgen->stackPointer;
            break;
        }
        inPlaceOp(expr, 1 + argumentCount + 1 /* receiver, args, result */, cgen);
    }
    ++cgen->nMessageSends;
    switch (expr->left->kind) {
    case EXPR_ATTR:
        EMIT_OPCODE(isSuper ? OPCODE_SET_ATTR_SUPER : OPCODE_SET_ATTR);
        index = LITERAL_INDEX((Object *)expr->left->sym->sym, cgen);
        encodeUnsignedInt(index, cgen);
        cgen->stackPointer -= 2;
        break;
    case EXPR_ATTR_VAR:
        EMIT_OPCODE(isSuper ? OPCODE_SET_ATTR_VAR_SUPER : OPCODE_SET_ATTR_VAR);
        cgen->stackPointer -= 3;
        break;
    }
    tallyPush(cgen); /* result */
    /* discard 'set' method result, exposing the value that was squirrelled away */
    EMIT_OPCODE(OPCODE_POP); --cgen->stackPointer;
    CHECK_STACKP();
}

static void inPlaceIndexOp(Expr *expr, OpcodeGen *cgen) {
    Expr *arg;
    size_t argumentCount;
    int isSuper;
    
    assert(expr->left->oper == OPER_INDEX && "invalid lvalue");
    /* get/set common receiver */
    emitCodeForExpr(expr->left->left, &isSuper, cgen);
    /* get/set common arguments */
    for (arg = expr->left->right, argumentCount = 0;
         arg;
         arg = arg->nextArg, ++argumentCount) {
        emitCodeForExpr(arg, 0, cgen);
    }
    /* rhs */
    if (expr->oper == OPER_EQ) {
        emitCodeForExpr(expr->right, 0, cgen);
        squirrel(1 + argumentCount + 1 /* receiver, args, new value */, cgen);
    } else {
        /* get __index__ { */
        dupN(argumentCount + 1, cgen);
        ++cgen->nMessageSends;
        EMIT_OPCODE(isSuper ? OPCODE_CALL_SUPER : OPCODE_CALL);
        encodeUnsignedInt(METHOD_NAMESPACE_RVALUE, cgen);
        encodeUnsignedInt((unsigned int)expr->left->oper, cgen);
        encodeUnsignedInt(argumentCount, cgen);
        cgen->stackPointer -= argumentCount + 1;
        tallyPush(cgen); /* result */
        /* } get __index__ */
        inPlaceOp(expr, 1 + argumentCount + 1 /* receiver, args, result */ , cgen);
    }
    ++argumentCount; /* new item value */
    ++cgen->nMessageSends;
    EMIT_OPCODE(isSuper ? OPCODE_CALL_SUPER : OPCODE_CALL);
    encodeUnsignedInt(METHOD_NAMESPACE_LVALUE, cgen);
    encodeUnsignedInt((unsigned int)OPER_INDEX, cgen);
    encodeUnsignedInt(argumentCount, cgen);
    cgen->stackPointer -= argumentCount + 1;
    tallyPush(cgen); /* result */
    /* discard 'set' method result, exposing the value that was squirrelled away */
    EMIT_OPCODE(OPCODE_POP); --cgen->stackPointer;
    CHECK_STACKP();
}

static void emitBranchForExpr(Expr *expr, int cond, size_t label, size_t fallThroughLabel, int dup, OpcodeGen *cgen) {
    for ( ; expr->next; expr = expr->next) {
        emitCodeForOneExpr(expr, 0, cgen);
        /* XXX: We could elide this 'pop' if 'expr' is a conditional expr. */
        EMIT_OPCODE(OPCODE_POP); --cgen->stackPointer;
    }
    emitBranchForOneExpr(expr, cond, label, fallThroughLabel, dup, cgen);
}

static void emitBranchForOneExpr(Expr *expr, int cond, size_t label, size_t fallThroughLabel, int dup, OpcodeGen *cgen) {
    opcode_t pushOpcode;
    
    switch (expr->kind) {
    case EXPR_NAME:
        pushOpcode = expr->u.ref.def->u.def.pushOpcode;
        if (pushOpcode == OPCODE_PUSH_FALSE ||
            pushOpcode == OPCODE_PUSH_TRUE) {
            int killCode = pushOpcode == OPCODE_PUSH_TRUE ? cond : !cond;
            SET_OFFSET(expr);
            if (killCode) {
                if (dup) {
                    EMIT_OPCODE(pushOpcode);
                    tallyPush(cgen);
                    --cgen->stackPointer;
                }
                emitBranch(OPCODE_BRANCH_ALWAYS, label, cgen);
            }
            SET_END(expr);
            break;
        } /* else fall through */
    default:
        emitCodeForExpr(expr, 0, cgen);
        /*
         * XXX: This sequence could be replaced by a special set of
         * branch-or-pop opcodes.
         */
        if (dup) {
            EMIT_OPCODE(OPCODE_DUP); tallyPush(cgen);
        }
        emitBranch(cond ? OPCODE_BRANCH_IF_TRUE : OPCODE_BRANCH_IF_FALSE, label, cgen);
        if (dup) {
            EMIT_OPCODE(OPCODE_POP); --cgen->stackPointer;
        }
        break;
    case EXPR_AND:
        SET_OFFSET(expr);
        if (cond) {
            /* branch if true */
            emitBranchForExpr(expr->left, 0, fallThroughLabel, expr->right->codeOffset, 0, cgen);
            emitBranchForExpr(expr->right, 1, label, fallThroughLabel, dup, cgen);
        } else {
            /* branch if false */
            emitBranchForExpr(expr->left, 0, label, expr->right->codeOffset, dup, cgen);
            emitBranchForExpr(expr->right, 0, label, fallThroughLabel, dup, cgen);
        }
        SET_END(expr);
        break;
    case EXPR_OR:
        SET_OFFSET(expr);
        if (cond) {
            /* branch if true */
            emitBranchForExpr(expr->left, 1, label, expr->right->codeOffset, dup, cgen);
            emitBranchForExpr(expr->right, 1, label, fallThroughLabel, dup, cgen);
        } else {
            /* branch if false */
            emitBranchForExpr(expr->left, 1, fallThroughLabel, expr->right->codeOffset, 0, cgen);
            emitBranchForExpr(expr->right, 0, label, fallThroughLabel, dup, cgen);
        }
        SET_END(expr);
        break;
    case EXPR_COND:
        SET_OFFSET(expr);
        emitBranchForExpr(expr->cond, 0, expr->right->codeOffset, expr->left->codeOffset, 0, cgen);
        emitBranchForExpr(expr->left, cond, label, fallThroughLabel, dup, cgen);
        emitBranch(OPCODE_BRANCH_ALWAYS, fallThroughLabel, cgen);
        emitBranchForExpr(expr->right, cond, label, fallThroughLabel, dup, cgen);
        SET_END(expr);
        break;
    }
}

static void emitCodeForBlockBody(Stmt *body, Expr *valueExpr, OpcodeGen *cgen) {
    Stmt *s;
    
    for (s = body; s; s = s->next) {
        emitCodeForStmt(s, valueExpr->codeOffset, 0, 0, cgen);
    }
    emitCodeForExpr(valueExpr, 0, cgen);
}

static void emitCodeForBlock(Expr *expr, CodeGen *outer) {
    CodeGen gcgen;
    OpcodeGen *cgen; BlockCodeGen *bcg;
    Stmt *body, *s;
    Expr *valueExpr, voidDef, voidExpr;
    size_t codeSize, stackSize;
    opcode_t *opcodesBegin;
    CodeGen *home;
    
    memset(&voidDef, 0, sizeof(voidDef));
    memset(&voidExpr, 0, sizeof(voidExpr));
    voidDef.kind = EXPR_NAME;
    voidDef.sym = SpkSymbolNode_Get(SpkSymbol_get("void"));
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
    bcg = &gcgen.u.o.u.block;
    bcg->opcodeGen = &cgen->generic->u.o;
    bcg->opcodeGen->generic = cgen->generic;
    cgen->argumentCount = expr->aux.block.argumentCount;
    cgen->varArgList = 0;
    cgen->localCount = expr->aux.block.localCount;
    
    rewind(cgen, 0);
    opcodesBegin = 0;
    switch (outer->kind) {
    case CODE_GEN_BLOCK:
    case CODE_GEN_METHOD:
        opcodesBegin = outer->u.o.opcodesBegin ? outer->u.o.opcodesBegin + outer->u.o.currentOffset : 0;
        break;
    default:
        assert(0);
    }
    
    body = expr->aux.block.stmtList;

    /* dry run to compute offsets */
    if (NESTING) {
        save(cgen); /* XXX: 'stackSize' is zero here */
    }
    
    emitCodeForBlockBody(body, valueExpr, cgen);
    EMIT_OPCODE(OPCODE_RESTORE_CALLER);
    EMIT_OPCODE(OPCODE_RET);
    
    if (opcodesBegin) {
        /* now generate code for real */
        
        codeSize = cgen->currentOffset;
        stackSize = cgen->stackSize;
        
        rewind(cgen, opcodesBegin);
        
        cgen->stackSize = stackSize;
        if (NESTING) {
            save(cgen);
        }
        cgen->stackSize = 0;
        
        emitCodeForBlockBody(body, valueExpr, cgen);
        EMIT_OPCODE(OPCODE_RESTORE_CALLER);
        EMIT_OPCODE(OPCODE_RET);
        
        assert(cgen->currentOffset == codeSize);
        assert(cgen->stackSize == stackSize);
    }
    
    outer->u.o.currentOffset += cgen->currentOffset;
    outer->u.o.opcodesEnd = cgen->opcodesEnd;
    
    /* Guarantee that the home context is at least as big as the
     * biggest child block context needs to be.  See
     * Context_blockCopy().
     */
    for (home = outer; home->kind != CODE_GEN_METHOD; home = home->outer)
        ;
    if (cgen->stackPointer > home->u.o.stackSize) {
        home->u.o.stackSize = cgen->stackPointer;
    }
}

/****************************************************************************/
/* statements */

static void emitCodeForMethod(Stmt *stmt, CodeGen *cgen);
static void emitCodeForClass(Stmt *stmt, CodeGen *cgen);

static void emitCodeForStmt(Stmt *stmt, size_t parentNextLabel, size_t breakLabel, size_t continueLabel, OpcodeGen *cgen) {
    Stmt *s;
    size_t nextLabel, childNextLabel, elseLabel;
    
    /* for branching to the next statement in the control flow */
    nextLabel = stmt->next ? stmt->next->codeOffset : parentNextLabel;
    
    SET_OFFSET(stmt);
    
    switch (stmt->kind) {
    case STMT_BREAK:
        assert((!cgen->opcodesEnd || breakLabel) && "break not allowed here");
        emitBranch(OPCODE_BRANCH_ALWAYS, breakLabel, cgen);
        break;
    case STMT_COMPOUND:
        for (s = stmt->top; s; s = s->next) {
            emitCodeForStmt(s, nextLabel, breakLabel, continueLabel, cgen);
        }
        break;
    case STMT_CONTINUE:
        assert((!cgen->opcodesEnd || continueLabel) && "continue not allowed here");
        emitBranch(OPCODE_BRANCH_ALWAYS, continueLabel, cgen);
        break;
    case STMT_DEF_VAR:
    case STMT_DEF_METHOD:
    case STMT_DEF_CLASS:
        break;
    case STMT_DO_WHILE:
        childNextLabel = stmt->expr->codeOffset;
        emitCodeForStmt(stmt->top, childNextLabel, nextLabel, childNextLabel, cgen);
        emitBranchForExpr(stmt->expr, 1, stmt->codeOffset, nextLabel, 0, cgen);
        break;
    case STMT_EXPR:
        if (stmt->expr) {
            emitCodeForExpr(stmt->expr, 0, cgen);
            EMIT_OPCODE(OPCODE_POP); --cgen->stackPointer;
        }
        break;
    case STMT_FOR:
        childNextLabel = stmt->incr ? stmt->incr->codeOffset : (stmt->expr ? stmt->expr->codeOffset : stmt->top->codeOffset);
        if (stmt->init) {
            emitCodeForExpr(stmt->init, 0, cgen);
            EMIT_OPCODE(OPCODE_POP); --cgen->stackPointer;
        }
        if (stmt->expr) {
            emitBranch(OPCODE_BRANCH_ALWAYS, stmt->expr->codeOffset, cgen);
        }
        emitCodeForStmt(stmt->top, childNextLabel, nextLabel, childNextLabel, cgen);
        if (stmt->incr) {
            emitCodeForExpr(stmt->incr, 0, cgen);
            EMIT_OPCODE(OPCODE_POP); --cgen->stackPointer;
        }
        if (stmt->expr) {
            emitBranchForExpr(stmt->expr, 1, stmt->top->codeOffset, nextLabel, 0, cgen);
        } else {
            emitBranch(OPCODE_BRANCH_ALWAYS, stmt->top->codeOffset, cgen);
        }
        break;
    case STMT_IF_ELSE:
        elseLabel = stmt->bottom ? stmt->bottom->codeOffset : nextLabel;
        emitBranchForExpr(stmt->expr, 0, elseLabel, stmt->top->codeOffset, 0, cgen);
        emitCodeForStmt(stmt->top, nextLabel, breakLabel, continueLabel, cgen);
        if (stmt->bottom) {
            emitBranch(OPCODE_BRANCH_ALWAYS, nextLabel, cgen);
            emitCodeForStmt(stmt->bottom, nextLabel, breakLabel, continueLabel, cgen);
        }
        break;
    case STMT_RETURN:
        if (stmt->expr) {
            emitCodeForExpr(stmt->expr, 0, cgen);
        } else {
            EMIT_OPCODE(OPCODE_PUSH_VOID);
            tallyPush(cgen);
        }
        if (cgen->inLeaf) {
            EMIT_OPCODE(OPCODE_RET_LEAF);
        } else {
            EMIT_OPCODE(OPCODE_RESTORE_SENDER);
            EMIT_OPCODE(OPCODE_RET);
        }
        --cgen->stackPointer;
        break;
    case STMT_WHILE:
        childNextLabel = stmt->expr->codeOffset;
        emitBranch(OPCODE_BRANCH_ALWAYS, stmt->expr->codeOffset, cgen);
        emitCodeForStmt(stmt->top, childNextLabel, nextLabel, childNextLabel, cgen);
        emitBranchForExpr(stmt->expr, 1, stmt->top->codeOffset, nextLabel, 0, cgen);
        break;
    }
    assert(cgen->stackPointer == 0);
    CHECK_STACKP();
}

static void emitCodeForMethod(Stmt *stmt, CodeGen *outer) {
    CodeGen gcgen;
    OpcodeGen *cgen; MethodCodeGen *mcg;
    Stmt *body, *s;
    Stmt sentinel;
    Symbol *messageSelector, *apply;
    size_t stackSize;
    int thunk;
    Method *enclosingMethod;
    
    apply = SpkSymbol_get("__apply__");
    messageSelector = stmt->u.method.name->sym;
    memset(&sentinel, 0, sizeof(sentinel));
    sentinel.kind = STMT_RETURN;
    
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
    mcg->methodInstance = 0;
    cgen->argumentCount = stmt->u.method.argumentCount;
    cgen->varArgList = stmt->u.method.argList.var ? 1 : 0;
    cgen->localCount = stmt->u.method.localCount;
    rewind(cgen, 0);
    
    body = stmt->top;
    assert(body->kind == STMT_COMPOUND);
    
    /* emit a 'thunk' opcode if this is named call-style method --
       e.g., "class X { foo(...) {} }" */
    thunk = (messageSelector != apply &&
             stmt->expr->kind == EXPR_CALL &&
             stmt->expr->oper == OPER_APPLY);
    
    /* dry run to compute offsets */
    if (thunk) EMIT_OPCODE(OPCODE_THUNK);
    save(cgen); /* XXX: 'stackSize' is zero here */
    
    emitCodeForStmt(body, sentinel.codeOffset, 0, 0, cgen);
    emitCodeForStmt(&sentinel, 0, 0, 0, cgen);
    
    if (inLeaf(cgen)) {
        stackSize = cgen->stackSize;
        
        /* re-compute offsets w/o save & restore */
        rewind(cgen, 0);
        cgen->inLeaf = 1;
        if (thunk) EMIT_OPCODE(OPCODE_THUNK);
        leaf(cgen);
        emitCodeForStmt(body, sentinel.codeOffset, 0, 0, cgen);
        emitCodeForStmt(&sentinel, 0, 0, 0, cgen);
        
        /* now generate code for real */
        mcg->methodInstance = SpkMethod_new(cgen->currentOffset);
        rewind(cgen, SpkMethod_OPCODES(mcg->methodInstance));
        if (thunk) EMIT_OPCODE(OPCODE_THUNK);
        leaf(cgen);
        emitCodeForStmt(body, sentinel.codeOffset, 0, 0, cgen);
        emitCodeForStmt(&sentinel, 0, 0, 0, cgen);

    } else {
        stackSize = cgen->stackSize;

        /* now generate code for real */
        mcg->methodInstance = SpkMethod_new(cgen->currentOffset);
        rewind(cgen, SpkMethod_OPCODES(mcg->methodInstance));
    
        if (thunk) EMIT_OPCODE(OPCODE_THUNK);
        cgen->stackSize = stackSize;
        save(cgen);
        cgen->stackSize = 0;
        
        emitCodeForStmt(body, sentinel.codeOffset, 0, 0, cgen);
        emitCodeForStmt(&sentinel, 0, 0, 0, cgen);
    }

    assert(cgen->currentOffset == mcg->methodInstance->base.size);
    assert(cgen->stackSize == stackSize);
    
    for (s = body->top; s; s = s->next) {
        switch (s->kind) {
        case STMT_DEF_CLASS:
            emitCodeForClass(s, cgen->generic);
            break;
        case STMT_DEF_METHOD:
            emitCodeForMethod(s, cgen->generic);
            break;
        default:
            break;
        }
    }
    
    switch (outer->kind) {
    case CODE_GEN_METHOD:
        enclosingMethod = outer->u.o.u.method.methodInstance;
        if (!enclosingMethod->nestedMethodList.first) {
            enclosingMethod->nestedMethodList.first = mcg->methodInstance;
        } else {
            enclosingMethod->nestedMethodList.last->nextInScope = mcg->methodInstance;
        }
        enclosingMethod->nestedMethodList.last = mcg->methodInstance;
        break;
    case CODE_GEN_CLASS:
        SpkBehavior_insertMethod(outer->u.klass.classInstance,
                                 stmt->u.method.namespace,
                                 messageSelector,
                                 mcg->methodInstance);
        break;
    default:
        assert(0);
    }
}

static void emitCodeForClassBody(Stmt *body, CodeGen *cgen) {
    Stmt *s;
    
    assert(body->kind == STMT_COMPOUND);
    for (s = body->top; s; s = s->next) {
        switch (s->kind) {
        case STMT_DEF_METHOD:
            emitCodeForMethod(s, cgen);
            break;
        case STMT_DEF_VAR:
            break;
        case STMT_DEF_CLASS:
            emitCodeForClass(s, cgen);
            break;
        default:
            assert(0 && "executable code not allowed here");
        }
    }
}

static void emitCodeForClass(Stmt *stmt, CodeGen *outer) {
    CodeGen gcgen;
    ClassCodeGen *cgen;
    Method *enclosingMethod;
    Behavior *enclosingClass;
    
    /* push class code generator */
    cgen = &gcgen.u.klass;
    cgen->generic = &gcgen;
    cgen->generic->kind = CODE_GEN_CLASS;
    cgen->generic->outer = outer;
    cgen->generic->module = outer->module;
    cgen->classDef = stmt;
    cgen->classInstance = (Behavior *)stmt->expr->u.def.initValue;
    assert(cgen->classInstance);
    cgen->generic->level = outer->level + 1;
    
    emitCodeForClassBody(stmt->top, cgen->generic);

    switch (outer->kind) {
    case CODE_GEN_METHOD:
        enclosingMethod = outer->u.o.u.method.methodInstance;
        if (!enclosingMethod->nestedClassList.first) {
            enclosingMethod->nestedClassList.first = cgen->classInstance;
        } else {
            enclosingMethod->nestedClassList.last->nextInScope = cgen->classInstance;
        }
        enclosingMethod->nestedClassList.last = cgen->classInstance;
        break;
    case CODE_GEN_CLASS:
        enclosingClass = outer->u.klass.classInstance;
        if (!enclosingClass->nestedClassList.first) {
            enclosingClass->nestedClassList.first = cgen->classInstance;
        } else {
            enclosingClass->nestedClassList.last->nextInScope = cgen->classInstance;
        }
        enclosingClass->nestedClassList.last = cgen->classInstance;
        break;
    default:
        assert(0);
    }
}

static void emitCodeForModule(Stmt *stmt, ModuleCodeGen *moduleCodeGen) {
    CodeGen gcgen;
    ClassCodeGen *cgen;
    
    /* push class code generator */
    cgen = &gcgen.u.klass;
    cgen->generic = &gcgen;
    cgen->generic->kind = CODE_GEN_CLASS;
    cgen->generic->outer = 0;
    cgen->generic->module = moduleCodeGen;
    cgen->classDef = stmt;
    cgen->classInstance = moduleCodeGen->moduleClassInstance;
    cgen->generic->level = 1;
    
    emitCodeForClassBody(stmt->top, cgen->generic);
}

/****************************************************************************/

static Behavior *getSuperclass(Stmt *classDef, ModuleCodeGen *cgen) {
    Object *initValue;
    Behavior *superclass;
    
    initValue = classDef->u.klass.superclassName->u.ref.def->u.def.initValue;
    assert(initValue && (superclass = (Behavior *)Spk_CAST(Class, initValue)));
    return superclass;
}

static void createClass(Stmt *classDef, ModuleCodeGen *cgen) {
    Behavior *theClass;
    Symbol *className;
    
    theClass = (Behavior *)SpkClass_new(classDef->expr->sym->sym);
    SpkBehavior_init(theClass, getSuperclass(classDef, cgen), 0, classDef->u.klass.instVarCount);
    
    if (!cgen->firstClass) {
        cgen->firstClass = theClass;
    } else {
        cgen->lastClass->next = theClass;
    }
    cgen->lastClass = theClass;
    
    classDef->expr->u.def.initValue = (Object *)theClass;
}

static void createClassTree(Stmt *classDef, ModuleCodeGen *cgen) {
    /* create class with subclasses in preorder */
    Stmt *subclassDef;
    
    createClass(classDef, cgen);
    for (subclassDef = classDef->u.klass.firstSubclassDef;
         subclassDef;
         subclassDef = subclassDef->u.klass.nextSubclassDef) {
        createClassTree(subclassDef, cgen);
    }
}

static void initClassTreeOperTables(Stmt *classDef, ModuleCodeGen *cgen) {
    /* create class with subclasses in preorder */
    Behavior *theClass;
    Stmt *subclassDef;
    
    theClass = Spk_CAST(Behavior, classDef->expr->u.def.initValue);
    SpkBehavior_inheritOperators(theClass);
    for (subclassDef = classDef->u.klass.firstSubclassDef;
         subclassDef;
         subclassDef = subclassDef->u.klass.nextSubclassDef) {
        initClassTreeOperTables(subclassDef, cgen);
    }
}

/****************************************************************************/

static void initGlobalVars(Object **globals, Stmt *stmtList) {
    Stmt *s;
    
    for (s = stmtList; s; s = s->next) {
        if (s->kind == STMT_DEF_CLASS || s->kind == STMT_DEF_VAR) {
            globals[s->expr->u.def.index] = s->expr->u.def.initValue;
        }
    }
}

static void initThunks(Object **globals, Module *module, Stmt *stmtList) {
    /* Create a thunk for each global function. */
    Stmt *s;
    Thunk *thunk;
    Method *method;
    Behavior *methodClass;
    
    methodClass = module->base.klass;
    
    for (s = stmtList; s; s = s->next) {
        if (s->kind == STMT_DEF_METHOD) {
            method = SpkBehavior_lookupMethod(
                methodClass,
                METHOD_NAMESPACE_RVALUE,
                s->u.method.name->sym
                );
            
            thunk = (Thunk *)malloc(sizeof(Thunk));
            thunk->base.klass = Spk_ClassThunk;
            thunk->receiver = (Object *)module;
            thunk->method = method;
            thunk->methodClass = methodClass;
            thunk->pc = SpkMethod_OPCODES(method) + 1; /* skip 'thunk' opcode */
            
            globals[s->expr->left->u.def.index] = (Object *)thunk;
        }
    }
}

Module *SpkCodeGen_generateCode(Stmt *tree, unsigned int dataSize,
                                Stmt *predefList, Stmt *rootClassList) {
    Stmt *s;
    CodeGen gcgen;
    ModuleCodeGen *cgen;
    Module *module;
    Object **globals, **literals;
    unsigned int index;
    Behavior *aClass;
    
    memset(&gcgen, 0, sizeof(gcgen));
    cgen = &gcgen.u.module;
    cgen->generic = &gcgen;
    cgen->generic->kind = CODE_GEN_MODULE;
    
    /* Create a 'Module' subclass to represent this module. */
    cgen->moduleClassInstance = (Behavior *)SpkBehavior_new();
    SpkBehavior_init(cgen->moduleClassInstance, Spk_ClassModule, 0, dataSize);
    
    /* Create all classes. */
    for (s = rootClassList; s; s = s->u.klass.nextRootClassDef) {
        createClassTree(s, cgen);
    }

    /* Generate code. */
    emitCodeForModule(tree, cgen);
    
    /* Finish initializing classes. XXX: Do class init right! */
    for (s = rootClassList; s; s = s->u.klass.nextRootClassDef) {
        initClassTreeOperTables(s, cgen);
    }
    
    /* Create and initialize the module. */
    module = SpkModule_new(cgen->moduleClassInstance);
    module->firstClass = cgen->firstClass;
    module->literals = cgen->rodata;
    globals = SpkModule_VARIABLES(module);
    for (index = 0; index < dataSize; ++index) {
        globals[index] = Spk_uninit; /* XXX: null? */
    }
    /* XXX: This stuff should happen at runtime, with binding! */
    initGlobalVars(globals, predefList);
    initGlobalVars(globals, tree->top->top);
    initThunks(globals, module, tree->top->top);

    /* Patch 'module', 'outer' attributes of classes. */
    for (aClass = cgen->firstClass; aClass; aClass = aClass->next) {
        aClass->module = module;
        aClass->outer = (Object *)module;
        aClass->outerClass = cgen->moduleClassInstance;
    }
    module->base.klass->module = module;
    
    return module;
}
