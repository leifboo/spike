
#include "cgen.h"

#include "behavior.h"
#include "interp.h"
#include "module.h"
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
    unsigned int dataSize;
    Object **rodata;
    unsigned int rodataSize, rodataAllocSize;
} CodeGen;


/****************************************************************************/
/* opcodes */

#define EMIT_OPCODE(opcode) \
if (cgen->opcodesEnd) *cgen->opcodesEnd++ = (opcode); \
cgen->currentOffset++

#define SET_OFFSET(n) \
if (cgen->opcodesEnd) assert((n)->codeOffset == cgen->currentOffset); \
else (n)->codeOffset = cgen->currentOffset;

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

static void fixUpBranch(Stmt *target, CodeGen *cgen) {
    ptrdiff_t base, displacement, filler;
    
    /* offset of branch instruction */
    base = (ptrdiff_t)(cgen->currentOffset - 1);
    
    displacement = (ptrdiff_t)target->codeOffset - base;
    encodeSignedInt(displacement, cgen);
    
    /* XXX: don't do this */
    filler = 3 - (cgen->currentOffset - base);
    assert(0 <= filler && filler <= 1 && "invalid jump");
    for ( ; filler > 0; --filler) {
        EMIT_OPCODE(OPCODE_NOP);
    }
}

static void emitBranch(opcode_t opcode, Stmt *target, CodeGen *cgen) {
    EMIT_OPCODE(opcode);
    if (cgen->opcodesEnd) {
        fixUpBranch(target, cgen);
    } else {
        EMIT_OPCODE(OPCODE_NOP);
        EMIT_OPCODE(OPCODE_NOP);
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
    switch (def->u.def.level) {
    case 0: opcode = OPCODE_PUSH_GLOBAL;   break;
    case 1: opcode = OPCODE_PUSH_INST_VAR; break;
    case 2: opcode = OPCODE_PUSH_LOCAL;    break;
    }
    EMIT_OPCODE(opcode);
    encodeUnsignedInt(def->u.def.index, cgen);
    tallyPush(cgen);
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

static void emitCodeForExpr(Expr *expr, int *super, CodeGen *cgen) {
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
    case EXPR_POSTFIX:
        switch (expr->oper) {
        case OPER_CALL:
            for (arg = expr->right, argumentCount = 0;
                 arg;
                 arg = arg->next, ++argumentCount) {
                emitCodeForExpr(arg, 0, cgen);
            }
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
    }
    return;
}

/****************************************************************************/
/* statements */

static void emitCodeForMethod(Stmt *stmt, CodeGen *cgen);
static void emitCodeForClass(Stmt *stmt, CodeGen *cgen);

static void emitCodeForStmt(Stmt *stmt, Stmt *sentinel, CodeGen *cgen) {
    Stmt *s, *exitStmt;
    
    exitStmt = stmt->next ? stmt->next : sentinel;
    
    SET_OFFSET(stmt);
    
    switch (stmt->kind) {
    case STMT_COMPOUND:
        for (s = stmt->top; s; s = s->next) {
            emitCodeForStmt(s, exitStmt, cgen);
        }
        break;
    case STMT_DEF:
        emitCodeForMethod(stmt, cgen);
        break;
    case STMT_DEF_CLASS:
        emitCodeForClass(stmt, cgen);
        break;
    case STMT_EXPR:
        emitCodeForExpr(stmt->expr, 0, cgen);
        EMIT_OPCODE(OPCODE_POP);
        --cgen->stackPointer;
        break;
    case STMT_IF_ELSE:
        emitCodeForExpr(stmt->expr, 0, cgen);
        emitBranch(OPCODE_BRANCH_IF_FALSE, stmt->bottom ? stmt->bottom : exitStmt, cgen);
        --cgen->stackPointer;
        emitCodeForStmt(stmt->top, exitStmt, cgen);
        if (stmt->bottom) {
            emitBranch(OPCODE_BRANCH_ALWAYS, exitStmt, cgen);
            emitCodeForStmt(stmt->bottom, exitStmt, cgen);
        }
        break;
    case STMT_RETURN:
        if (stmt->expr) {
            emitCodeForExpr(stmt->expr, 0, cgen);
        } else {
            EMIT_OPCODE(OPCODE_PUSH_NULL); /* void? */
            tallyPush(cgen);
        }
        EMIT_OPCODE(OPCODE_RESTORE_SENDER);
        EMIT_OPCODE(OPCODE_RET);
        --cgen->stackPointer;
        break;
    }
    assert(cgen->stackPointer == 0);
    CHECK_STACKP();    
}

static void emitCodeForMethod(Stmt *stmt, CodeGen *cgen) {
    Stmt sentinel;

    assert(cgen->currentClass && !cgen->currentMethod && "method definition not allowed here");
    
    memset(&sentinel, 0, sizeof(sentinel));
    sentinel.kind = STMT_RETURN;

    cgen->currentOffset = 0;
    cgen->opcodesBegin = cgen->opcodesEnd = 0;
    cgen->stackPointer = cgen->stackSize = 0;
    
    /* dry run to compute offsets */
    EMIT_OPCODE(OPCODE_NEW_THUNK);
    /*EMIT_OPCODE(OPCODE_RET);*/
    EMIT_OPCODE(OPCODE_SAVE);
    emitCodeForStmt(stmt->top, &sentinel, cgen);
    emitCodeForStmt(&sentinel, 0, cgen);

    cgen->currentMethod = SpkMethod_new(stmt->u.method.argumentCount,
                                        stmt->u.method.localCount,
                                        cgen->stackSize,
                                        cgen->currentOffset);
    cgen->opcodesBegin = &cgen->currentMethod->opcodes[0];
    cgen->opcodesEnd = cgen->opcodesBegin;
    cgen->currentOffset = 0;
    cgen->stackPointer = cgen->stackSize = 0;
    
    EMIT_OPCODE(OPCODE_NEW_THUNK);
    /*EMIT_OPCODE(OPCODE_RET);*/
    EMIT_OPCODE(OPCODE_SAVE);
    emitCodeForStmt(stmt->top, &sentinel, cgen);
    emitCodeForStmt(&sentinel, 0, cgen);

    assert(cgen->currentOffset == cgen->currentMethod->size);
    assert(cgen->stackSize ==  cgen->currentMethod->stackSize);
    
    SpkBehavior_insertMethod(cgen->currentClass, stmt->u.method.name->sym, cgen->currentMethod);
    cgen->currentMethod = 0;
    cgen->currentOffset = 0;
    cgen->opcodesBegin = cgen->opcodesEnd = 0;
}

static void emitCodeForClass(Stmt *stmt, CodeGen *cgen) {
    Behavior *theClass;
    
    assert(!cgen->currentClass && !cgen->currentMethod && "class definition not allowed here");
    
    theClass = SpkBehavior_new(0, 0);
    
    if (!cgen->firstClass) {
        cgen->firstClass = theClass;
    } else {
        cgen->lastClass->next = theClass;
    }
    cgen->lastClass = theClass;
    
    cgen->currentClass = theClass;
    emitCodeForStmt(stmt->top, 0, cgen);
    cgen->currentClass = 0;
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
    cgen.dataSize = dataSize;
    
    for (s = tree; s; s = s->next) {
        emitCodeForStmt(s, 0, &cgen);
    }
    
    module = SpkModule_new(dataSize + cgen.rodataSize);
    module->firstClass = cgen.firstClass;
    globals = SpkInterpreter_instanceVars((Object *)module);
    /* XXX: properly initialize data */
    for (index = 0; index < dataSize; ++index) {
        globals[index] = 0;
    }
    for (index = 0; index < cgen.rodataSize; ++index) {
        globals[dataSize + index] = cgen.rodata[index];
    }
    for (aClass = cgen.firstClass; aClass; aClass = aClass->next) {
        aClass->module = module;
    }
    
    return module;
}
