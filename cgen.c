
#include "cgen.h"

#include "behavior.h"
#include "class.h"
#include "dict.h"
#include "interp.h"
#include "module.h"
#include "obj.h"
#include "st.h"
#include "tree.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>


#if DEBUG_STACKP
/* Emit debug opcodes which check to see whether the stack depth at
 * runtime is what the compiler thought it should be.
 */
#define CHECK_STACKP() \
    EMIT_OPCODE(OPCODE_CHECK_STACKP); \
    encodeUnsignedInt(cgen->stackPointer, cgen)
#else
#define CHECK_STACKP()
#endif


typedef struct CodeGen {
    Behavior *firstClass, *lastClass, *currentClass;
    Method *currentMethod;
    size_t currentOffset;
    opcode_t *opcodesBegin, *opcodesEnd;
    size_t stackPointer, stackSize;
    Object **data;
    unsigned int dataSize;
    Object **rodata;
    unsigned int rodataSize, rodataAllocSize;
    IdentityDictionary *globals;
} CodeGen;


/****************************************************************************/
/* opcodes */

#define EMIT_OPCODE(opcode) \
if (cgen->opcodesEnd) *cgen->opcodesEnd++ = (opcode); \
cgen->currentOffset++

#define SET_OFFSET(n) \
if (cgen->opcodesEnd) assert((n)->codeOffset == cgen->currentOffset); \
else (n)->codeOffset = cgen->currentOffset;

#define SET_END(n) \
if (cgen->opcodesEnd) assert((n)->endLabel == cgen->currentOffset); \
else (n)->endLabel = cgen->currentOffset;

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

static void push(Expr *expr, CodeGen *cgen) {
    opcode_t opcode;
    Expr *def;
    
    def = expr->u.ref.def;
    assert(def);
    if (cgen->currentClass) {
        switch (def->u.def.level) {
        case 0: opcode = OPCODE_PUSH_GLOBAL;   break;
        case 1: opcode = OPCODE_PUSH_INST_VAR; break;
        case 2: opcode = OPCODE_PUSH_LOCAL;    break;
        }
    } else {
        /* global function */
        switch (def->u.def.level) {
        case 0: opcode = OPCODE_PUSH_GLOBAL;   break;
        case 1: opcode = OPCODE_PUSH_LOCAL;    break;
        default: assert(0);
        }
    }
    EMIT_OPCODE(opcode);
    encodeUnsignedInt(def->u.def.index, cgen);
    tallyPush(cgen);
}

static void store(Expr *var, CodeGen *cgen) {
    opcode_t opcode;
    Expr *def;
    
    def = var->u.ref.def;
    assert(def);
    if (cgen->currentClass) {
        switch (def->u.def.level) {
        case 0: opcode = OPCODE_STORE_GLOBAL;   break;
        case 1: opcode = OPCODE_STORE_INST_VAR; break;
        case 2: opcode = OPCODE_STORE_LOCAL;    break;
        }
    } else {
        /* global function */
        switch (def->u.def.level) {
        case 0: opcode = OPCODE_STORE_GLOBAL;   break;
        case 1: opcode = OPCODE_STORE_LOCAL;    break;
        default: assert(0);
        }
    }
    EMIT_OPCODE(opcode);
    encodeUnsignedInt(def->u.def.index, cgen);
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

static void emitCodeForExpr(Expr *expr, int *super, CodeGen *cgen) {
    for ( ; expr->next; expr = expr->next) {
        emitCodeForOneExpr(expr, super, cgen);
        EMIT_OPCODE(OPCODE_POP); --cgen->stackPointer;
    }
    emitCodeForOneExpr(expr, super, cgen);
}

static void emitCodeForOneExpr(Expr *expr, int *super, CodeGen *cgen) {
    opcode_t opcode;
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
        encodeSignedInt(expr->intValue, cgen);
        tallyPush(cgen);
        break;
    case EXPR_CHAR:
        EMIT_OPCODE(OPCODE_PUSH_GLOBAL);
        index = LITERAL_INDEX((Object *)expr->charValue, cgen);
        encodeUnsignedInt(index, cgen);
        tallyPush(cgen);
        break;
    case EXPR_STR:
        EMIT_OPCODE(OPCODE_PUSH_GLOBAL);
        index = LITERAL_INDEX((Object *)expr->strValue, cgen);
        encodeUnsignedInt(index, cgen);
        tallyPush(cgen);
        break;
    case EXPR_NAME:
        push(expr, cgen);
        break;
    case EXPR_SELF:
        EMIT_OPCODE(OPCODE_PUSH_SELF);
        tallyPush(cgen);
        break;
    case EXPR_SUPER:
        assert(super && "invalid use of 'super'");
        *super = 1;
        break;
    case EXPR_NULL:
        EMIT_OPCODE(OPCODE_PUSH_NULL);
        tallyPush(cgen);
        break;
    case EXPR_FALSE:
        EMIT_OPCODE(OPCODE_PUSH_FALSE);
        tallyPush(cgen);
        break;
    case EXPR_TRUE:
        EMIT_OPCODE(OPCODE_PUSH_TRUE);
        tallyPush(cgen);
        break;
    case EXPR_CONTEXT:
        EMIT_OPCODE(OPCODE_PUSH_CONTEXT);
        tallyPush(cgen);
        break;
    case EXPR_POSTFIX:
        switch (expr->oper) {
        case OPER_CALL:
            for (arg = expr->right, argumentCount = 0;
                 arg;
                 arg = arg->next, ++argumentCount) {
                emitCodeForOneExpr(arg, 0, cgen);
            }
            /* XXX: In the Green Book (Ch. 2), Dan Ingalls writes that
             * some find this evaluation order to be "surprising" as
             * it differs from the left-to-right order in the source
             * code. The Spike interpreter could go either way: we
             * must encode 'argumentCount' in any case, since --
             * unlike in Smalltalk -- it is not implied by the message
             * selector. But note that Spike operators have
             * precedence, so the evaluation order is not
             * left-to-right anyway.
             *
             * XXX: It is possible that we want the intepreter to be
             * "ambidextrous". To conserve stack space, we should
             * evaluate the deepest subexpression first. In order to
             * do this, the intepreter must be able to handle both
             * cases: receiver at the top of the stack, and receiver
             * beneath the arguments. There would need to be two
             * versions of the 'send' opcodes.
             */
            emitCodeForExpr(expr->left, &isSuper, cgen);
            if (isSuper) {
                opcode = OPCODE_CALL_SUPER;
                cgen->stackPointer -= argumentCount - 1;
            } else {
                opcode = OPCODE_CALL;
                cgen->stackPointer -= argumentCount;
            }
            EMIT_OPCODE(opcode);
            encodeUnsignedInt(argumentCount, cgen);
            CHECK_STACKP();
            break;
        }
        break;
    case EXPR_ATTR:
        emitCodeForExpr(expr->left, &isSuper, cgen);
        if (isSuper) {
            opcode = OPCODE_ATTR_SUPER;
            ++cgen->stackPointer;
        } else {
            opcode = OPCODE_ATTR;
        }
        EMIT_OPCODE(opcode);
        index = LITERAL_INDEX((Object *)expr->sym->sym, cgen);
        encodeUnsignedInt(index, cgen);
        CHECK_STACKP();
        break;
    case EXPR_PREOP:
        assert(expr->left->kind == EXPR_NAME && "invalid lvalue");
        emitCodeForExpr(expr->left, 0, cgen);
        EMIT_OPCODE(OPCODE_OPER);
        encodeUnsignedInt((unsigned int)expr->oper, cgen);
        store(expr->left, cgen);
        CHECK_STACKP();
        break;
    case EXPR_POSTOP:
        assert(expr->left->kind == EXPR_NAME && "invalid lvalue");
        emitCodeForExpr(expr->left, 0, cgen);
        EMIT_OPCODE(OPCODE_DUP); ++cgen->stackPointer;
        EMIT_OPCODE(OPCODE_OPER);
        encodeUnsignedInt((unsigned int)expr->oper, cgen);
        store(expr->left, cgen);
        EMIT_OPCODE(OPCODE_POP); --cgen->stackPointer;
        CHECK_STACKP();
        break;
    case EXPR_UNARY:
        emitCodeForExpr(expr->left, &isSuper, cgen);
        if (isSuper) {
            opcode = OPCODE_OPER_SUPER;
            ++cgen->stackPointer;
        } else {
            opcode = OPCODE_OPER;
        }
        EMIT_OPCODE(opcode);
        encodeUnsignedInt((unsigned int)expr->oper, cgen);
        CHECK_STACKP();
        break;
    case EXPR_BINARY:
        emitCodeForExpr(expr->right, 0, cgen);
        emitCodeForExpr(expr->left, &isSuper, cgen);
        if (isSuper) {
            opcode = OPCODE_OPER_SUPER;
        } else {
            opcode = OPCODE_OPER;
            --cgen->stackPointer;
        }
        EMIT_OPCODE(opcode);
        encodeUnsignedInt((unsigned int)expr->oper, cgen);
        CHECK_STACKP();
        break;
    case EXPR_ID:
    case EXPR_NI:
        emitCodeForExpr(expr->right, 0, cgen);
        emitCodeForExpr(expr->left, 0, cgen);
        EMIT_OPCODE(OPCODE_ID);
        --cgen->stackPointer;
        if (expr->kind == EXPR_NI) {
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
        assert(expr->left->kind == EXPR_NAME && "invalid lvalue");
        emitCodeForExpr(expr->right, 0, cgen);
        if (expr->oper != OPER_EQ) {
            emitCodeForExpr(expr->left, 0, cgen);
            EMIT_OPCODE(OPCODE_OPER);
            encodeUnsignedInt((unsigned int)expr->oper, cgen);
            --cgen->stackPointer;
        }
        store(expr->left, cgen);
        CHECK_STACKP();
        break;
    }
    
    SET_END(expr);
    
    return;
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
    switch (expr->kind) {
    default:
        emitCodeForExpr(expr, 0, cgen);
        /*
         * XXX: This sequence could be replaced by a special set of
         * branch-or-pop opcodes.
         */
        if (dup) {
            EMIT_OPCODE(OPCODE_DUP); ++cgen->stackPointer;
        }
        emitBranch(cond ? OPCODE_BRANCH_IF_TRUE : OPCODE_BRANCH_IF_FALSE, label, cgen);
        if (dup) {
            EMIT_OPCODE(OPCODE_POP); --cgen->stackPointer;
        }
        break;
    case EXPR_FALSE:
        SET_OFFSET(expr);
        if (!cond) {
            if (dup) {
                EMIT_OPCODE(OPCODE_PUSH_FALSE);
                tallyPush(cgen);
                --cgen->stackPointer;
            }
            emitBranch(OPCODE_BRANCH_ALWAYS, label, cgen);
        }
        SET_END(expr);
        break;
    case EXPR_TRUE:
        SET_OFFSET(expr);
        if (cond) {
            if (dup) {
                EMIT_OPCODE(OPCODE_PUSH_TRUE);
                tallyPush(cgen);
                --cgen->stackPointer;
            }
            emitBranch(OPCODE_BRANCH_ALWAYS, label, cgen);
        }
        SET_END(expr);
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
        EMIT_OPCODE(OPCODE_RESTORE_SENDER);
        EMIT_OPCODE(OPCODE_RET);
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

static void emitCodeForMethod(Stmt *stmt, CodeGen *cgen) {
    Stmt sentinel;
    Behavior *methodClass;
    Symbol *messageSelector;
    Object *function = 0;

    assert(!cgen->currentMethod && "method definition not allowed here");
    
    if (cgen->currentClass) {
        methodClass = cgen->currentClass;
        messageSelector = stmt->u.method.name->sym;
    } else {
        /* global function */
        assert(stmt->expr->kind == EXPR_POSTFIX &&
               stmt->expr->oper == OPER_CALL); /* XXX */
        assert(stmt->expr->left->kind == EXPR_NAME); /* XXX */
        function = cgen->data[stmt->expr->left->u.def.index];
        methodClass = function->klass;
        messageSelector = SpkSymbol_get("__call__");
    }
    
    memset(&sentinel, 0, sizeof(sentinel));
    sentinel.kind = STMT_RETURN;

    cgen->currentOffset = 0;
    cgen->opcodesBegin = cgen->opcodesEnd = 0;
    cgen->stackPointer = cgen->stackSize = 0;
    
    /* dry run to compute offsets */
    if (!function) {
        EMIT_OPCODE(OPCODE_THUNK);
    }
    EMIT_OPCODE(OPCODE_SAVE);
    emitCodeForStmt(stmt->top, sentinel.codeOffset, 0, 0, cgen);
    emitCodeForStmt(&sentinel, 0, 0, 0, cgen);

    cgen->currentMethod = SpkMethod_new(stmt->u.method.argumentCount,
                                        stmt->u.method.localCount,
                                        cgen->stackSize,
                                        cgen->currentOffset);
    cgen->opcodesBegin = &cgen->currentMethod->opcodes[0];
    cgen->opcodesEnd = cgen->opcodesBegin;
    cgen->currentOffset = 0;
    cgen->stackPointer = cgen->stackSize = 0;
    
    if (!function) {
        EMIT_OPCODE(OPCODE_THUNK);
    }
    EMIT_OPCODE(OPCODE_SAVE);
    emitCodeForStmt(stmt->top, sentinel.codeOffset, 0, 0, cgen);
    emitCodeForStmt(&sentinel, 0, 0, 0, cgen);

    assert(cgen->currentOffset == cgen->currentMethod->size);
    assert(cgen->stackSize ==  cgen->currentMethod->stackSize);
    
    SpkBehavior_insertMethod(methodClass, messageSelector, cgen->currentMethod);
    cgen->currentMethod = 0;
    cgen->currentOffset = 0;
    cgen->opcodesBegin = cgen->opcodesEnd = 0;
}

static void emitCodeForClass(Stmt *stmt, CodeGen *cgen) {
    Behavior *theClass;
    
    assert(!cgen->currentClass && !cgen->currentMethod && "class definition not allowed here");
    
    theClass = (Behavior *)cgen->data[stmt->expr->u.def.index];
    cgen->currentClass = theClass;
    emitCodeForStmt(stmt->top, 0, 0, 0, cgen);
    cgen->currentClass = 0;
}

/****************************************************************************/

static void createClass(Stmt *stmt, CodeGen *cgen) {
    Behavior *theClass;
    Symbol *className;
    
    className = stmt->expr->sym->sym;
    theClass = (Behavior *)SpkClass_new(className);
    SpkBehavior_init(theClass, 0, 0, stmt->u.klass.instVarCount);
    
    if (!cgen->firstClass) {
        cgen->firstClass = theClass;
    } else {
        cgen->lastClass->next = theClass;
    }
    cgen->lastClass = theClass;
    
    /* initialize global variable */
    cgen->data[stmt->expr->u.def.index] = (Object *)theClass;
    SpkIdentityDictionary_atPut(cgen->globals,
                                (Object *)className,
                                (Object *)theClass);
}

static void setSuperclass(Stmt *stmt, CodeGen *cgen) {
    Behavior *theClass, *superclass;
    
    theClass = (Behavior *)cgen->data[stmt->expr->u.def.index];
    assert(theClass && theClass->base.klass == (Behavior *)ClassClass);
    superclass = stmt->u.klass.super ? (Behavior *)cgen->data[stmt->u.klass.super->u.ref.def->u.def.index] : ClassObject;
    assert(superclass && superclass->base.klass == (Behavior *)ClassClass);
    theClass->superclass = superclass;
}

static void checkForSuperclassCycle(Stmt *stmt, CodeGen *cgen) {
    /* XXX: Move/copy this check to 'scheck'. */
    Behavior *theClass, *aClass;
    
    theClass = (Behavior *)cgen->data[stmt->expr->u.def.index];
    assert(theClass && theClass->base.klass == (Behavior *)ClassClass);
    for (aClass = theClass->superclass; aClass; aClass = aClass->superclass) {
        assert(aClass != theClass && "cycle in superclass chain");
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
    theClass->base.klass = (Behavior *)ClassClass;
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
    assert(stmt->expr->kind == EXPR_POSTFIX &&
           stmt->expr->oper == OPER_CALL); /* XXX */
    assert(stmt->expr->left->kind == EXPR_NAME); /* XXX */
    cgen->data[stmt->expr->left->u.def.index] = theFunction;
    SpkIdentityDictionary_atPut(cgen->globals,
                                (Object *)functionName,
                                theFunction);
}

/****************************************************************************/

Module *SpkCodeGen_generateCode(Stmt *tree, unsigned int dataSize) {
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
    
    /* Create all classes. */
    for (s = tree; s; s = s->next) {
        switch (s->kind) {
        case STMT_DEF_CLASS:
            createClass(s, &cgen);
            break;
        case STMT_DEF_METHOD:
            createFunction(s, &cgen);
            break;
        }
    }
    for (s = tree; s; s = s->next) {
        if (s->kind == STMT_DEF_CLASS) {
            setSuperclass(s, &cgen);
        }
    }
    for (s = tree; s; s = s->next) {
        if (s->kind == STMT_DEF_CLASS) {
            checkForSuperclassCycle(s, &cgen);
        }
    }

    /* Generate code. */
    for (s = tree; s; s = s->next) {
        emitCodeForStmt(s, 0, 0, 0, &cgen);
    }
    
    /* Create and initialize the module. */
    module = SpkModule_new(dataSize + cgen.rodataSize, cgen.globals);
    module->firstClass = cgen.firstClass;
    globals = SpkInterpreter_instanceVars((Object *)module);
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
