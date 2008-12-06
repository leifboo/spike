
#include "scheck.h"

#include "behavior.h"
#include "class.h"
#include "interp.h"
#include "st.h"
#include "sym.h"
#include "tree.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct StaticChecker {
    SymbolTable *st;
    Stmt *currentClass;
} StaticChecker;


static void checkVarDeclList(Expr *declList, Stmt *stmt, StaticChecker *checker, unsigned int pass) {
    Expr *decl;
    
    if (pass != 1) {
        return;
    }
    for (decl = declList; decl; decl = decl->next) {
        assert(decl->kind == EXPR_NAME);
        SpkSymbolTable_Insert(checker->st, decl);
    }
}

static void checkOneExpr(Expr *, Stmt *, StaticChecker *, unsigned int);

static void checkExpr(Expr *expr, Stmt *stmt, StaticChecker *checker, unsigned int pass) {
    for ( ; expr; expr = expr->next) {
        checkOneExpr(expr, stmt, checker, pass);
    }
}

static void checkOneExpr(Expr *expr, Stmt *stmt, StaticChecker *checker, unsigned int pass) {
    Expr *arg;
    
    switch (expr->kind) {
    case EXPR_NAME:
        switch (pass) {
        case 1:
            if (!checker->currentClass && stmt->kind == STMT_DEF_METHOD) {
                SpkSymbolTable_Insert(checker->st, expr);
            }
            break;
        case 2:
            if (stmt->kind != STMT_DEF_METHOD) {
                SpkSymbolTable_Bind(checker->st, expr);
            }
            break;
        }
        break;
    case EXPR_CALL:
        checkExpr(expr->left, stmt, checker, pass);
        if (stmt->kind == STMT_DEF_METHOD) {
            if (pass == 1) {
                assert(expr->oper == OPER_APPLY && expr->left->kind == EXPR_NAME);
                stmt->u.method.name = expr->left->sym;
                stmt->u.method.argList = expr->right;
            }
        } else {
            for (arg = expr->right; arg; arg = arg->nextArg) {
                checkExpr(arg, stmt, checker, pass);
            }
        }
        break;
    case EXPR_ATTR:
        checkExpr(expr->left, stmt, checker, pass);
        break;
    case EXPR_ATTR_VAR:
        checkExpr(expr->left, stmt, checker, pass);
        checkExpr(expr->right, stmt, checker, pass);
        break;
    case EXPR_PREOP:
    case EXPR_POSTOP:
        checkExpr(expr->left, stmt, checker, pass);
        break;
    case EXPR_UNARY:
        checkExpr(expr->left, stmt, checker, pass);
        break;
    case EXPR_ID:
    case EXPR_NI:
    case EXPR_BINARY:
        checkExpr(expr->left, stmt, checker, pass);
        checkExpr(expr->right, stmt, checker, pass);
        break;
    case EXPR_AND:
    case EXPR_OR:
        checkExpr(expr->left, stmt, checker, pass);
        checkExpr(expr->right, stmt, checker, pass);
        break;
    case EXPR_COND:
        checkExpr(expr->cond, stmt, checker, pass);
        checkExpr(expr->left, stmt, checker, pass);
        checkExpr(expr->right, stmt, checker, pass);
        break;
    case EXPR_ASSIGN:
        checkExpr(expr->left, stmt, checker, pass);
        checkExpr(expr->right, stmt, checker, pass);
        break;
    }
}

static Stmt *superclassDef(Stmt *aClassDef, StaticChecker *checker) {
    Expr *expr;
    
    if (!aClassDef->u.klass.super)
        return 0; /* XXX: Object */
    expr = SpkSymbolTable_Lookup(checker->st, aClassDef->u.klass.super->sym);
    if (!expr)
        return 0; /* undefined */
    assert(expr->kind == EXPR_NAME && expr->u.def.stmt);
    return expr->u.def.stmt;
}

static void checkForSuperclassCycle(Stmt *aClassDef, StaticChecker *checker) {
    /* Floyd's cycle-finding algorithm */
    Stmt *tortoise, *hare;
    
    tortoise = hare = superclassDef(aClassDef, checker);
    if (!tortoise)
        return;
    hare = superclassDef(hare, checker);
    while (hare && tortoise != hare) {
        tortoise = superclassDef(tortoise, checker);
        hare = superclassDef(hare, checker);
        if (!hare)
            return;
        hare = superclassDef(hare, checker);
    }
    
    assert(!hare && "cycle in superclass chain");
}

static void checkStmt(Stmt *stmt, Stmt *outer, StaticChecker *checker, unsigned int outerPass) {
    Stmt *s;
    SymbolNode *sym;
    Expr *arg;
    unsigned int innerPass;
    int enterNewContext;
    
    switch (stmt->kind) {
    case STMT_COMPOUND:
        if (outerPass == 2) {
            enterNewContext = outer &&
                              (outer->kind == STMT_DEF_METHOD ||
                               outer->kind == STMT_DEF_CLASS);
            SpkSymbolTable_EnterScope(checker->st, enterNewContext);
            if (outer && outer->kind == STMT_DEF_METHOD) {
                /* declare function arguments */
                for (arg = outer->u.method.argList; arg; arg = arg->nextArg) {
                    assert(arg->kind == EXPR_NAME);
                    SpkSymbolTable_Insert(checker->st, arg);
                    ++outer->u.method.argumentCount;
                }
            }
            for (innerPass = 1; innerPass < 3; ++innerPass) {
                for (s = stmt->top; s; s = s->next) {
                    checkStmt(s, stmt, checker, innerPass);
                }
            }
            if (outer) {
                switch (outer->kind) {
                case STMT_DEF_METHOD:
                    outer->u.method.localCount = checker->st->currentScope->context->nDefs -
                                                 outer->u.method.argumentCount;
                    break;
                case STMT_DEF_CLASS:
                    outer->u.klass.instVarCount = checker->st->currentScope->context->nDefs;
                    break;
                }
            }
            SpkSymbolTable_ExitScope(checker->st);
        }
        break;
    case STMT_DEF_VAR:
        checkVarDeclList(stmt->expr, stmt, checker, outerPass);
        break;
    case STMT_DEF_METHOD:
        checkExpr(stmt->expr, stmt, checker, outerPass);
        checkStmt(stmt->top, stmt, checker, outerPass);
        break;
    case STMT_DEF_CLASS:
        if (outerPass == 1) {
            assert(stmt->expr->kind == EXPR_NAME);
            SpkSymbolTable_Insert(checker->st, stmt->expr);
            stmt->expr->u.def.stmt = stmt;
        }
        if (stmt->u.klass.super) {
            checkExpr(stmt->u.klass.super, stmt, checker, outerPass);
        }
        if (outerPass == 2) {
            checkForSuperclassCycle(stmt, checker);
        }
        checker->currentClass = stmt;
        checkStmt(stmt->top, stmt, checker, outerPass);
        checker->currentClass = 0;
        break;
    case STMT_DO_WHILE:
        checkExpr(stmt->expr, stmt, checker, outerPass);
        checkStmt(stmt->top, stmt, checker, outerPass);
        break;
    case STMT_EXPR:
        if (stmt->expr) {
            checkExpr(stmt->expr, stmt, checker, outerPass);
        }
        break;
    case STMT_FOR:
        if (stmt->init) {
            checkExpr(stmt->init, stmt, checker, outerPass);
        }
        if (stmt->expr) {
            checkExpr(stmt->expr, stmt, checker, outerPass);
        }
        if (stmt->incr) {
            checkExpr(stmt->incr, stmt, checker, outerPass);
        }
        checkStmt(stmt->top, stmt, checker, outerPass);
        break;
    case STMT_IF_ELSE:
        checkExpr(stmt->expr, stmt, checker, outerPass);
        checkStmt(stmt->top, stmt, checker, outerPass);
        if (stmt->bottom) {
            checkStmt(stmt->bottom, stmt, checker, outerPass);
        }
        break;
    case STMT_RETURN:
        /* return */
        if (stmt->expr) {
            checkExpr(stmt->expr, stmt, checker, outerPass);
        }
        break;
    case STMT_WHILE:
        checkExpr(stmt->expr, stmt, checker, outerPass);
        checkStmt(stmt->top, stmt, checker, outerPass);
        break;
    }
}

static struct PseudoVariable {
    const char *name;
    opcode_t pushOpcode;
} pseudoVariables[] = {
    { "self",        OPCODE_PUSH_SELF },
    { "super",       OPCODE_PUSH_SUPER },
    { "false",       OPCODE_PUSH_FALSE },
    { "true",        OPCODE_PUSH_TRUE },
    { "null",        OPCODE_PUSH_NULL },
    { "thisContext", OPCODE_PUSH_CONTEXT },
    { 0, 0 }
};

static Expr *newNameExpr() {
    Expr *newExpr;
    
    newExpr = (Expr *)malloc(sizeof(Expr));        
    memset(newExpr, 0, sizeof(Expr));
    newExpr->kind = EXPR_NAME;
    return newExpr;
}

static Expr *newPseudoVariable(struct PseudoVariable *pv) {
    Expr *newExpr;
    
    newExpr = newNameExpr();
    newExpr->sym = SpkSymbolNode_Get(SpkSymbol_get(pv->name));
    newExpr->u.def.pushOpcode = pv->pushOpcode;
    return newExpr;
}

static Stmt *predefinedClassDef(Class *predefClass) {
    Stmt *newStmt;
    
    newStmt = (Stmt *)malloc(sizeof(Stmt));
    memset(newStmt, 0, sizeof(Stmt));
    newStmt->kind = STMT_DEF_CLASS;
    newStmt->expr = newNameExpr();
    newStmt->expr->sym = SpkSymbolNode_Get(predefClass->name);
    newStmt->expr->u.def.stmt = newStmt;
    return newStmt;
}

int SpkStaticChecker_Check(Stmt *tree, BootRec *bootRec,
                           unsigned int *pDataSize, StmtList *predefList) {
    Stmt *s;
    StaticChecker checker;
    struct PseudoVariable *pv;
    unsigned int pass;
    
    checker.st = SpkSymbolTable_New();
    SpkSymbolTable_EnterScope(checker.st, 1); /* built-in scope */
    for (pv = pseudoVariables; pv->name; ++pv) {
        SpkSymbolTable_Insert(checker.st, newPseudoVariable(pv));
    }
    SpkSymbolTable_EnterScope(checker.st, 1); /* global scope */
    predefList->first = predefList->last = 0;
    for ( ; bootRec->var; ++bootRec) {
        if ((*bootRec->var)->klass == (Behavior *)ClassClass) {
            Stmt *classDef = predefinedClassDef((Class *)*bootRec->var);
            SpkSymbolTable_Insert(checker.st, classDef->expr);
            classDef->expr->u.def.initValue = *bootRec->var;
            if (!predefList->first) {
                predefList->first = classDef;
            } else {
                predefList->last->next = classDef;
            }
            predefList->last = classDef;
        }
    }
    
    checker.currentClass = 0;

    for (pass = 1; pass < 3; ++pass) {
        for (s = tree; s; s = s->next) {
            checkStmt(s, 0, &checker, pass);
        }
    }
    
    *pDataSize = checker.st->currentScope->context->nDefs;
    
    SpkSymbolTable_ExitScope(checker.st);
    
    if (checker.st->errorList) {
        SymbolNode *sym;
        
        fprintf(stderr, "errors!\n");
        for (sym = checker.st->errorList; sym; sym = sym->nextError) {
            if (sym->multipleDefList) {
                if (sym->entry && sym->entry->scope->context->level == 0) {
                    fprintf(stderr, "cannot redefine built-in name '%s'\n",
                            sym->sym->str);
                } else {
                    fprintf(stderr, "symbol '%s' multiply defined\n",
                            sym->sym->str);
                }
            }
            if (sym->undefList) {
                fprintf(stderr, "symbol '%s' undefined\n",
                        sym->sym->str);
            }
        }
        return -1;
    }
    
    return 0;
}
