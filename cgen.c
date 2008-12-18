
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


typedef struct CodeGen {
    Behavior *firstClass, *lastClass, *currentClass;
    Method *currentMethod;
    Stmt *currentMethodDef;
    size_t currentOffset;
    opcode_t *opcodesBegin, *opcodesEnd;
    size_t stackPointer, stackSize;
    unsigned int nMessageSends;
    unsigned int nContextRefs;
    int inLeaf;
    Object **data;
    unsigned int dataSize;
    Object **rodata;
    unsigned int rodataSize, rodataAllocSize;
    IdentityDictionary *globals;
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

static void encodeUnsignedInt(unsigned long value, CodeGen *cgen) {
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

static void encodeSignedInt(long value, CodeGen *cgen) {
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

static void fixUpBranch(size_t target, CodeGen *cgen) {
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

static void emitBranch(opcode_t opcode, size_t target, CodeGen *cgen) {
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

static void tallyPush(CodeGen *cgen) {
    ++cgen->stackPointer;
    if (cgen->stackPointer > cgen->stackSize) {
        cgen->stackSize = cgen->stackPointer;
    }
    CHECK_STACKP();
}

static void dupN(unsigned long n, CodeGen *cgen) {
    EMIT_OPCODE(OPCODE_DUP_N);
    encodeUnsignedInt(n, cgen);
    cgen->stackPointer += n;
    if (cgen->stackPointer > cgen->stackSize) {
        cgen->stackSize = cgen->stackPointer;
    }
    CHECK_STACKP();
}

static unsigned int indexOfVar(Expr *def, opcode_t opcode, CodeGen *cgen) {
    /* In leaf methods, the arguments are reversed. */
    return
        (cgen->inLeaf &&
         (opcode == OPCODE_PUSH_LOCAL || opcode == OPCODE_STORE_LOCAL))
        ? cgen->currentMethodDef->u.method.argumentCount - def->u.def.index - 1
        : def->u.def.index;
}

static void push(Expr *expr, int *super, CodeGen *cgen) {
    opcode_t opcode;
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
    
    if (cgen->currentClass) {
        switch (def->u.def.level) {
        case 1: opcode = OPCODE_PUSH_GLOBAL;   break;
        case 2: opcode = OPCODE_PUSH_INST_VAR; break;
        case 3: opcode = OPCODE_PUSH_LOCAL;    break;
        default: assert(0);
        }
    } else {
        /* global function */
        switch (def->u.def.level) {
        case 1: opcode = OPCODE_PUSH_GLOBAL;   break;
        case 2: opcode = OPCODE_PUSH_LOCAL;    break;
        default: assert(0);
        }
    }
    EMIT_OPCODE(opcode);
    encodeUnsignedInt(indexOfVar(def, opcode, cgen), cgen);
    tallyPush(cgen);
}

static void store(Expr *var, CodeGen *cgen) {
    opcode_t opcode;
    Expr *def;
    
    def = var->u.ref.def;
    assert(def);
    if (cgen->currentClass) {
        switch (def->u.def.level) {
        case 1: opcode = OPCODE_STORE_GLOBAL;   break;
        case 2: opcode = OPCODE_STORE_INST_VAR; break;
        case 3: opcode = OPCODE_STORE_LOCAL;    break;
        default: assert(0);
        }
    } else {
        /* global function */
        switch (def->u.def.level) {
        case 1: opcode = OPCODE_STORE_GLOBAL;   break;
        case 2: opcode = OPCODE_STORE_LOCAL;    break;
        default: assert(0);
        }
    }
    EMIT_OPCODE(opcode);
    encodeUnsignedInt(indexOfVar(def, opcode, cgen), cgen);
}

static void save(Stmt *stmt, CodeGen *cgen) {
    EMIT_OPCODE(stmt->u.method.argList.var ? OPCODE_SAVE_VAR : OPCODE_SAVE);
    encodeUnsignedInt(stmt->u.method.argumentCount, cgen);
    encodeUnsignedInt(stmt->u.method.localCount, cgen);
    encodeUnsignedInt(cgen->stackSize, cgen);
}

static void leaf(Stmt *stmt, CodeGen *cgen) {
    EMIT_OPCODE(OPCODE_LEAF);
    encodeUnsignedInt(stmt->u.method.argumentCount, cgen);
}


/****************************************************************************/
/* rodata */

static unsigned int getLiteralIndex(Object *literal, CodeGen *cgen) {
    unsigned int index;
    
    for (index = 0; index < cgen->rodataSize; ++index) {
        /* XXX: This only works properly for Symbols. */
        if (cgen->rodata[index] == literal) {
            return cgen->dataSize + index;
        }
    }
    
    index = cgen->rodataSize++;
    
    if (cgen->rodataSize > cgen->rodataAllocSize) {
        cgen->rodataAllocSize = cgen->rodataAllocSize ? cgen->rodataAllocSize * 2 : 2;
        cgen->rodata = (Object **)realloc(cgen->rodata, cgen->rodataAllocSize * sizeof(Object *));
    }
    
    cgen->rodata[index] = literal;
    return cgen->dataSize + index;
}

#define LITERAL_INDEX(literal, cgen) ((cgen)->opcodesEnd ? getLiteralIndex(literal, cgen) : 0)


/****************************************************************************/
/* expressions */

static void emitCodeForOneExpr(Expr *, int *, CodeGen *);
static void emitBranchForExpr(Expr *expr, int, size_t, size_t, int, CodeGen *);
static void emitBranchForOneExpr(Expr *, int, size_t, size_t, int, CodeGen *);
static void inPlaceIndexOp(Expr *, CodeGen *);

static void emitCodeForExpr(Expr *expr, int *super, CodeGen *cgen) {
    for ( ; expr->next; expr = expr->next) {
        emitCodeForOneExpr(expr, super, cgen);
        EMIT_OPCODE(OPCODE_POP); --cgen->stackPointer;
    }
    emitCodeForOneExpr(expr, super, cgen);
}

static void emitCodeForOneExpr(Expr *expr, int *super, CodeGen *cgen) {
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
        encodeSignedInt(expr->lit.intValue, cgen);
        tallyPush(cgen);
        break;
    case EXPR_FLOAT:
        EMIT_OPCODE(OPCODE_PUSH_GLOBAL);
        index = LITERAL_INDEX((Object *)expr->lit.floatValue, cgen);
        encodeUnsignedInt(index, cgen);
        tallyPush(cgen);
        break;
    case EXPR_CHAR:
        EMIT_OPCODE(OPCODE_PUSH_GLOBAL);
        index = LITERAL_INDEX((Object *)expr->lit.charValue, cgen);
        encodeUnsignedInt(index, cgen);
        tallyPush(cgen);
        break;
    case EXPR_STR:
        EMIT_OPCODE(OPCODE_PUSH_GLOBAL);
        index = LITERAL_INDEX((Object *)expr->lit.strValue, cgen);
        encodeUnsignedInt(index, cgen);
        tallyPush(cgen);
        break;
    case EXPR_NAME:
        push(expr, super, cgen);
        break;
    case EXPR_SYMBOL:
        EMIT_OPCODE(OPCODE_PUSH_GLOBAL);
        index = LITERAL_INDEX((Object *)expr->sym->sym, cgen);
        encodeUnsignedInt(index, cgen);
        tallyPush(cgen);
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
        encodeUnsignedInt((unsigned int)expr->oper, cgen);
        encodeUnsignedInt(argumentCount, cgen);
        cgen->stackPointer -= (expr->var ? 1 : 0) + argumentCount + 1;
        tallyPush(cgen); /* result */
        CHECK_STACKP();
        break;
    case EXPR_ATTR:
        emitCodeForExpr(expr->left, &isSuper, cgen);
        ++cgen->nMessageSends;
        EMIT_OPCODE(isSuper ? OPCODE_ATTR_SUPER : OPCODE_ATTR);
        index = LITERAL_INDEX((Object *)expr->sym->sym, cgen);
        encodeUnsignedInt(index, cgen);
        CHECK_STACKP();
        break;
    case EXPR_ATTR_VAR:
        emitCodeForExpr(expr->left, &isSuper, cgen);
        emitCodeForExpr(expr->right, 0, cgen);
        ++cgen->nMessageSends;
        EMIT_OPCODE(isSuper ? OPCODE_ATTR_VAR_SUPER : OPCODE_ATTR_VAR);
        --cgen->stackPointer;
        CHECK_STACKP();
        break;
    case EXPR_PREOP:
        switch (expr->left->kind) {
        case EXPR_NAME:
            emitCodeForExpr(expr->left, 0, cgen);
            ++cgen->nMessageSends;
            EMIT_OPCODE(OPCODE_OPER);
            encodeUnsignedInt((unsigned int)expr->oper, cgen);
            store(expr->left, cgen);
            CHECK_STACKP();
            break;
        case EXPR_CALL:
            inPlaceIndexOp(expr, cgen);
            break;
        default:
            assert(0 && "invalid lvalue");
        }
        break;
    case EXPR_POSTOP:
        switch (expr->left->kind) {
        case EXPR_NAME:
            emitCodeForExpr(expr->left, 0, cgen);
            EMIT_OPCODE(OPCODE_DUP); tallyPush(cgen);
            ++cgen->nMessageSends;
            EMIT_OPCODE(OPCODE_OPER);
            encodeUnsignedInt((unsigned int)expr->oper, cgen);
            store(expr->left, cgen);
            EMIT_OPCODE(OPCODE_POP); --cgen->stackPointer;
            CHECK_STACKP();
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

static void inPlaceIndexOp(Expr *expr, CodeGen *cgen) {
    Expr *arg;
    size_t argumentCount;
    int isSuper;
    
    assert(expr->left->oper == OPER_GET_ITEM && "invalid lvalue");
    /* __item__/__setItem__ common receiver */
    emitCodeForExpr(expr->left->left, &isSuper, cgen);
    /* __item__/__setItem__ common arguments */
    for (arg = expr->left->right, argumentCount = 0;
         arg;
         arg = arg->nextArg, ++argumentCount) {
        emitCodeForExpr(arg, 0, cgen);
    }
    /* rhs */
    if (expr->oper == OPER_EQ) {
        emitCodeForExpr(expr->right, 0, cgen);
    } else {
        /* __item__ { */
        dupN(argumentCount + 1, cgen);
        ++cgen->nMessageSends;
        EMIT_OPCODE(isSuper ? OPCODE_CALL_SUPER : OPCODE_CALL);
        encodeUnsignedInt((unsigned int)expr->left->oper, cgen);
        encodeUnsignedInt(argumentCount, cgen);
        cgen->stackPointer -= argumentCount + 1;
        tallyPush(cgen); /* result */
        /* } __item__ */
        
        if (expr->right) {
            emitCodeForExpr(expr->right, 0, cgen);
        } else if (expr->kind == EXPR_POSTOP) {
            EMIT_OPCODE(OPCODE_DUP); tallyPush(cgen);
            EMIT_OPCODE(OPCODE_ROT);
            encodeUnsignedInt(argumentCount + 3, cgen);
        }
        
        ++cgen->nMessageSends;
        EMIT_OPCODE(OPCODE_OPER);
        encodeUnsignedInt((unsigned int)expr->oper, cgen);
        if (expr->right) {
            --cgen->stackPointer;
        }
    }
    ++argumentCount; /* new item value */
    ++cgen->nMessageSends;
    EMIT_OPCODE(isSuper ? OPCODE_CALL_SUPER : OPCODE_CALL);
    encodeUnsignedInt((unsigned int)OPER_SET_ITEM, cgen);
    encodeUnsignedInt(argumentCount, cgen);
    cgen->stackPointer -= argumentCount + 1;
    tallyPush(cgen); /* result */
    CHECK_STACKP();
    if (expr->kind == EXPR_POSTOP) {
        EMIT_OPCODE(OPCODE_POP); --cgen->stackPointer;
    }
}

static void emitBranchForExpr(Expr *expr, int cond, size_t label, size_t fallThroughLabel, int dup, CodeGen *cgen) {
    for ( ; expr->next; expr = expr->next) {
        emitCodeForOneExpr(expr, 0, cgen);
        /* XXX: We could elide this 'pop' if 'expr' is a conditional expr. */
        EMIT_OPCODE(OPCODE_POP); --cgen->stackPointer;
    }
    emitBranchForOneExpr(expr, cond, label, fallThroughLabel, dup, cgen);
}

static void emitBranchForOneExpr(Expr *expr, int cond, size_t label, size_t fallThroughLabel, int dup, CodeGen *cgen) {
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

/****************************************************************************/
/* statements */

static void emitCodeForMethod(Stmt *stmt, CodeGen *cgen);
static void emitCodeForClass(Stmt *stmt, CodeGen *cgen);

static void emitCodeForStmt(Stmt *stmt, size_t parentNextLabel, size_t breakLabel, size_t continueLabel, CodeGen *cgen) {
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
        break;
    case STMT_DEF_METHOD:
        emitCodeForMethod(stmt, cgen);
        break;
    case STMT_DEF_CLASS:
        emitCodeForClass(stmt, cgen);
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

static void rewind(CodeGen *cgen) {
    if (cgen->currentMethod) {
        cgen->opcodesBegin = SpkMethod_OPCODES(cgen->currentMethod);
        cgen->opcodesEnd = cgen->opcodesBegin;
        cgen->currentOffset = 0;
        cgen->stackPointer = cgen->stackSize = 0;
        cgen->nMessageSends = 0;
        cgen->nContextRefs = 0;
    } else {
        cgen->currentOffset = 0;
        cgen->opcodesBegin = cgen->opcodesEnd = 0;
        cgen->stackPointer = cgen->stackSize = 0;
        cgen->nMessageSends = 0;
        cgen->nContextRefs = 0;
        cgen->inLeaf = 0;
    }
}

static int isLeafMethod(Stmt *stmt, CodeGen *cgen) {
    return
        cgen->nMessageSends == 0 &&
        cgen->nContextRefs == 0 &&
        cgen->stackSize <= LEAF_STACK_SPACE &&
        stmt->u.method.localCount == 0 &&
        !stmt->u.method.argList.var;
}

static void emitCodeForMethod(Stmt *stmt, CodeGen *cgen) {
    Stmt sentinel;
    Behavior *methodClass;
    Symbol *messageSelector;
    Object *function = 0;
    size_t stackSize;
    
    assert(!cgen->currentMethodDef && "method definition not allowed here");
    
    cgen->currentMethodDef = stmt;

    if (cgen->currentClass) {
        methodClass = cgen->currentClass;
        messageSelector = stmt->u.method.name->sym;
    } else {
        /* global function */
        assert(stmt->expr->kind == EXPR_CALL &&
               stmt->expr->oper == OPER_APPLY); /* XXX */
        assert(stmt->expr->left->kind == EXPR_NAME); /* XXX */
        function = cgen->data[stmt->expr->left->u.def.index];
        methodClass = function->klass;
        messageSelector = SpkSymbol_get("__apply__");
    }
    
    memset(&sentinel, 0, sizeof(sentinel));
    sentinel.kind = STMT_RETURN;
    
    rewind(cgen);
    
    /* dry run to compute offsets */
    if (!function) EMIT_OPCODE(OPCODE_THUNK);
    save(stmt, cgen); /* XXX: 'stackSize' is zero here */
    
    emitCodeForStmt(stmt->top, sentinel.codeOffset, 0, 0, cgen);
    emitCodeForStmt(&sentinel, 0, 0, 0, cgen);
    
    if (isLeafMethod(stmt, cgen)) {
        stackSize = cgen->stackSize;
        
        /* re-compute offsets w/o save & restore */
        rewind(cgen);
        cgen->inLeaf = 1;
        if (!function) EMIT_OPCODE(OPCODE_THUNK);
        leaf(stmt, cgen);
        emitCodeForStmt(stmt->top, sentinel.codeOffset, 0, 0, cgen);
        emitCodeForStmt(&sentinel, 0, 0, 0, cgen);
        
        /* now generate code for real */
        cgen->currentMethod = SpkMethod_new(cgen->currentOffset);
        rewind(cgen);
        if (!function) EMIT_OPCODE(OPCODE_THUNK);
        leaf(stmt, cgen);
        emitCodeForStmt(stmt->top, sentinel.codeOffset, 0, 0, cgen);
        emitCodeForStmt(&sentinel, 0, 0, 0, cgen);

    } else {
        stackSize = cgen->stackSize + LEAF_STACK_SPACE;

        /* now generate code for real */
        cgen->currentMethod = SpkMethod_new(cgen->currentOffset);
        rewind(cgen);
    
        if (!function) EMIT_OPCODE(OPCODE_THUNK);
        cgen->stackSize = stackSize;
        save(stmt, cgen);
        cgen->stackSize = 0;
        
        emitCodeForStmt(stmt->top, sentinel.codeOffset, 0, 0, cgen);
        emitCodeForStmt(&sentinel, 0, 0, 0, cgen);
        cgen->stackSize += LEAF_STACK_SPACE;
    }

    assert(cgen->currentOffset == cgen->currentMethod->base.size);
    assert(cgen->stackSize == stackSize);
    
    SpkBehavior_insertMethod(methodClass, messageSelector, cgen->currentMethod);
    cgen->currentMethod = 0;
    cgen->currentMethodDef = 0;
    rewind(cgen);
}

static void emitCodeForClass(Stmt *stmt, CodeGen *cgen) {
    Behavior *theClass;
    
    assert(!cgen->currentClass && !cgen->currentMethodDef && "class definition not allowed here");
    
    theClass = (Behavior *)cgen->data[stmt->expr->u.def.index];
    cgen->currentClass = theClass;
    emitCodeForStmt(stmt->top, 0, 0, 0, cgen);
    cgen->currentClass = 0;
}

/****************************************************************************/

static void initClassVar(Expr *expr, CodeGen *cgen) {
    Object *theClass = expr->u.def.initValue;
    cgen->data[expr->u.def.index] = theClass;
    SpkIdentityDictionary_atPut(cgen->globals,
                                (Object *)expr->sym->sym,
                                theClass);
}

static Behavior *getSuperclass(Stmt *classDef, CodeGen *cgen) {
    Object *data;
    Behavior *superclass;
    
    data = cgen->data[classDef->u.klass.superclassName->u.ref.def->u.def.index];
    assert(data && (superclass = (Behavior *)Spk_CAST(Class, data)));
    return superclass;
}

static void createClass(Stmt *classDef, CodeGen *cgen) {
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
    initClassVar(classDef->expr, cgen);
}

static void createClassTree(Stmt *classDef, CodeGen *cgen) {
    /* create class with subclasses in preorder */
    Stmt *subclassDef;
    
    createClass(classDef, cgen);
    for (subclassDef = classDef->u.klass.firstSubclassDef;
         subclassDef;
         subclassDef = subclassDef->u.klass.nextSubclassDef) {
        createClassTree(subclassDef, cgen);
    }
}

/****************************************************************************/

static void createFunction(Stmt *stmt, CodeGen *cgen) {
    Behavior *theClass;
    Object *theFunction;
    Symbol *functionName;
    
    functionName = stmt->u.method.name->sym;
    /* XXX: This creates a class with an ordinary identifier as a class name. */
    theClass = (Behavior *)SpkClass_new(functionName);
    theClass->base.klass = (Behavior *)Spk_ClassClass;
    SpkBehavior_init(theClass, 0, 0, 0);
    
    if (!cgen->firstClass) {
        cgen->firstClass = theClass;
    } else {
        cgen->lastClass->next = theClass;
    }
    cgen->lastClass = theClass;
    
    theFunction = (Object *)malloc(theClass->instanceSize);
    theFunction->klass = theClass;

    /* initialize global variable */
    assert(stmt->expr->kind == EXPR_CALL &&
           stmt->expr->oper == OPER_APPLY); /* XXX */
    assert(stmt->expr->left->kind == EXPR_NAME); /* XXX */
    cgen->data[stmt->expr->left->u.def.index] = theFunction;
    SpkIdentityDictionary_atPut(cgen->globals,
                                (Object *)functionName,
                                theFunction);
}

/****************************************************************************/

Module *SpkCodeGen_generateCode(Stmt *tree, unsigned int dataSize,
                                Stmt *predefList, Stmt *rootClassList) {
    Stmt *s;
    CodeGen cgen;
    Module *module;
    Object **globals;
    unsigned int index;
    Behavior *aClass;
    
    memset(&cgen, 0, sizeof(cgen));
    cgen.data = (Object **)malloc(dataSize * sizeof(Object *));
    for (index = 0; index < dataSize; ++index) {
        cgen.data[index] = Spk_uninit; /* XXX: null? */
    }
    cgen.dataSize = dataSize;
    cgen.globals = SpkIdentityDictionary_new();
    for (s = predefList; s; s = s->next) {
        switch (s->kind) {
        case STMT_DEF_CLASS: initClassVar(s->expr, &cgen); break;
        default: assert(0);
        }
    }
    
    /* Create all classes. */
    for (s = rootClassList; s; s = s->u.klass.nextRootClassDef) {
        createClassTree(s, &cgen);
    }
    for (s = tree; s; s = s->next) {
        if (s->kind == STMT_DEF_METHOD) {
            createFunction(s, &cgen);
        }
    }

    /* Generate code. */
    for (s = tree; s; s = s->next) {
        emitCodeForStmt(s, 0, 0, 0, &cgen);
    }
    
    /* Create and initialize the module. */
    module = SpkModule_new(dataSize + cgen.rodataSize, cgen.globals);
    module->firstClass = cgen.firstClass;
    globals = SpkModule_VARIABLES((Object *)module);
    for (index = 0; index < dataSize; ++index) {
        globals[index] = cgen.data[index];
    }
    for (index = 0; index < cgen.rodataSize; ++index) {
        globals[dataSize + index] = cgen.rodata[index];
    }
    
    /* Patch 'module' attribute of classes. */
    for (aClass = cgen.firstClass; aClass; aClass = aClass->next) {
        aClass->module = module;
    }
    
    return module;
}
