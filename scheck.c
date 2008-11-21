#include "scheck.h"

#include "interp.h"
#include "st.h"
#include <assert.h>
#include <stdio.h>


typedef struct StaticChecker {
    SymbolTable *st;
    Stmt *currentClass;
    Symbol *self, *super, *null, *false, *true, *thisContext;
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
                if (expr->sym->sym == checker->self) {
                    expr->kind = EXPR_SELF;
                } else if (expr->sym->sym == checker->super) {
                    expr->kind = EXPR_SUPER;
                } else if (expr->sym->sym == checker->null) {
                    expr->kind = EXPR_NULL;
                } else if (expr->sym->sym == checker->false) {
                    expr->kind = EXPR_FALSE;
                } else if (expr->sym->sym == checker->true) {
                    expr->kind = EXPR_TRUE;
                } else if (expr->sym->sym == checker->thisContext) {
                    expr->kind = EXPR_CONTEXT;
                } else {
                    SpkSymbolTable_Bind(checker->st, expr);
                }
            }
            break;
        }
        break;
    case EXPR_POSTFIX:
        switch (expr->oper) {
        case OPER_CALL:
            checkExpr(expr->left, stmt, checker, pass);
            if (stmt->kind == STMT_DEF_METHOD) {
                if (pass == 1) {
                    assert(expr->left->kind == EXPR_NAME);
                    stmt->u.method.name = expr->left->sym;
                    stmt->u.method.argList = expr->right;
                }
            } else {
                for (arg = expr->right; arg; arg = arg->next) {
                    checkOneExpr(arg, stmt, checker, pass);
                }
            }
            break;
        }
        break;
    case EXPR_ATTR:
        checkExpr(expr->left, stmt, checker, pass);
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
                for (arg = outer->u.method.argList; arg; arg = arg->next) {
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
        }
        if (stmt->u.klass.super) {
            checkExpr(stmt->u.klass.super, stmt, checker, outerPass);
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

int SpkStaticChecker_Check(Stmt *tree, unsigned int *pDataSize) {
    Stmt *s;
    StaticChecker checker;
    unsigned int pass;
    
    checker.st = SpkSymbolTable_New();
    checker.currentClass = 0;
    checker.self = SpkSymbol_get("self");
    checker.super = SpkSymbol_get("super");
    checker.null = SpkSymbol_get("null");
    checker.false = SpkSymbol_get("false");
    checker.true = SpkSymbol_get("true");
    checker.thisContext = SpkSymbol_get("thisContext");
    SpkSymbolTable_EnterScope(checker.st, 1);
    
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
                fprintf(stderr, "symbol '%s' multiply defined\n",
                        sym->sym->str);
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
