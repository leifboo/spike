
#include "scheck.h"

#ifndef MALTIPY
#include "array.h"
#endif /* !MALTIPY */
#include "behavior.h"
#include "boot.h"
#include "class.h"
#include "heart.h"
#include "host.h"
#include "interp.h"
#include "native.h"
#include "rodata.h"
#include "st.h"
#include "tree.h"

#include <stdio.h>
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


typedef struct StaticChecker {
    SpkSymbolTable *st;
    StmtList *rootClassList;
    SpkUnknown *requestor;
} StaticChecker;


static SpkUnknown *badExpr(Expr *expr, const char *desc, StaticChecker *checker) {
    SpkUnknown *descStr, *tmp;
    
    descStr = SpkHost_StringFromCString(desc);
    if (!descStr)
        return 0;
    tmp = Spk_Keyword(Spk_GLOBAL(theInterpreter),
                      checker->requestor,
                      Spk_badExpr,
                      expr,
                      descStr,
                      0);
    Spk_DECREF(descStr);
    if (!tmp)
        return 0;
    Spk_DECREF(tmp);
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
}


static SpkUnknown *checkStmt(Stmt *, Stmt *, StaticChecker *, unsigned int);
static SpkUnknown *checkExpr(Expr *, Stmt *, StaticChecker *, unsigned int);
static SpkUnknown *checkOneExpr(Expr *, Stmt *, StaticChecker *, unsigned int);


static SpkUnknown *checkVarDefList(Expr *defList,
                                   Stmt *stmt,
                                   StaticChecker *checker,
                                   unsigned int pass)
{
    Expr *expr, *def;
    
    for (expr = defList; expr; expr = expr->next) {
        if (expr->kind == Spk_EXPR_ASSIGN) {
            def = expr->left;
            _(checkExpr(expr->right, stmt, checker, pass));
        } else {
            def = expr;
        }
        if (def->kind != Spk_EXPR_NAME) {
            if (pass == 1)
                _(badExpr(def, "invalid variable definition", checker));
            continue;
        }
        if (pass == 1)
            _(SpkSymbolTable_Insert(checker->st, def, checker->requestor));
    }
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static SpkUnknown *checkExpr(Expr *expr, Stmt *stmt, StaticChecker *checker,
                             unsigned int pass)
{
    for ( ; expr; expr = expr->next) {
        _(checkOneExpr(expr, stmt, checker, pass));
    }
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static SpkUnknown *checkBlock(Expr *expr, Stmt *stmt, StaticChecker *checker,
                              unsigned int outerPass)
{
    Expr *arg;
    Stmt *firstStmt, *s;
    unsigned int innerPass;
    
    if (outerPass != 2) {
        goto leave;
    }
    
    SpkSymbolTable_EnterScope(checker->st, 0);
    
    firstStmt = expr->aux.block.stmtList;
    expr->u.def.index = 0;
    
    if (firstStmt && firstStmt->kind == Spk_STMT_DEF_ARG) {
        /* declare block arguments */
        for (arg = firstStmt->expr; arg; arg = arg->next) {
            if (arg->kind != Spk_EXPR_NAME) {
                _(badExpr(arg, "invalid argument definition", checker));
                continue;
            }
            _(SpkSymbolTable_Insert(checker->st, arg, checker->requestor));
            ++expr->aux.block.argumentCount;
        }
        
        /* The 'index' of the block itself is the index of the first
         * argument.  The interpreter uses this index to find the
         * destination the block arguments; see Spk_OPCODE_CALL_BLOCK.
         */
        expr->u.def.index = firstStmt->expr->u.def.index;
        
        firstStmt = firstStmt->next;
    }
    
    for (innerPass = 1; innerPass <= 3; ++innerPass) {
        if (expr->aux.block.stmtList) {
            for (s = firstStmt; s; s = s->next) {
                _(checkStmt(s, stmt, checker, innerPass));
            }
        }
        if (expr->right) {
            _(checkExpr(expr->right, stmt, checker, innerPass));
        }
    }
    
    /* XXX: this is only needed for arbitrary nesting */
    expr->aux.block.localCount = checker->st->currentScope->context->nDefs -
                                 expr->aux.block.argumentCount;
    
    SpkSymbolTable_ExitScope(checker->st);
    
 leave:
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static SpkUnknown *checkOneExpr(Expr *expr, Stmt *stmt, StaticChecker *checker,
                                unsigned int pass)
{
    Expr *arg;
    
    switch (expr->kind) {
    case Spk_EXPR_LITERAL:
    case Spk_EXPR_SYMBOL:
        break;
    case Spk_EXPR_NAME:
        if (pass == 2) {
            _(SpkSymbolTable_Bind(checker->st, expr, checker->requestor));
        }
        break;
    case Spk_EXPR_BLOCK:
        _(checkBlock(expr, stmt, checker, pass));
        break;
    case Spk_EXPR_COMPOUND:
        for (arg = expr->right; arg; arg = arg->nextArg) {
            _(checkExpr(arg, stmt, checker, pass));
        }
        if (expr->var) {
            _(checkExpr(expr->var, stmt, checker, pass));
        }
        break;
    case Spk_EXPR_CALL:
    case Spk_EXPR_KEYWORD:
        _(checkExpr(expr->left, stmt, checker, pass));
        for (arg = expr->right; arg; arg = arg->nextArg) {
            _(checkExpr(arg, stmt, checker, pass));
        }
        if (expr->var) {
            _(checkExpr(expr->var, stmt, checker, pass));
        }
        break;
    case Spk_EXPR_ATTR:
    case Spk_EXPR_POSTOP:
    case Spk_EXPR_PREOP:
    case Spk_EXPR_UNARY:
        _(checkExpr(expr->left, stmt, checker, pass));
        break;
    case Spk_EXPR_AND:
    case Spk_EXPR_ASSIGN:
    case Spk_EXPR_ATTR_VAR:
    case Spk_EXPR_BINARY:
    case Spk_EXPR_ID:
    case Spk_EXPR_NI:
    case Spk_EXPR_OR:
        _(checkExpr(expr->left, stmt, checker, pass));
        _(checkExpr(expr->right, stmt, checker, pass));
        break;
    case Spk_EXPR_COND:
        _(checkExpr(expr->cond, stmt, checker, pass));
        _(checkExpr(expr->left, stmt, checker, pass));
        _(checkExpr(expr->right, stmt, checker, pass));
        break;
    }
    
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static SpkUnknown *checkMethodDef(Stmt *stmt,
                                  Stmt *outer,
                                  StaticChecker *checker,
                                  unsigned int outerPass)
{
    Stmt *body, *s;
    Expr *expr, *arg;
    SpkMethodNamespace ns;
    SpkSymbolNode *name;
    SpkOper oper;
    unsigned int innerPass;
    
    expr = stmt->expr;
    body = stmt->top;
    ASSERT(body->kind == Spk_STMT_COMPOUND,
           "compound statement expected");
    
    switch (outerPass) {
    case 1:
        ns = Spk_METHOD_NAMESPACE_RVALUE;
        name = 0;
        switch (expr->kind) {
        case Spk_EXPR_NAME:
            name = expr->sym;  Spk_INCREF(name);
            break;
        case Spk_EXPR_ASSIGN:
            if (expr->right->kind != Spk_EXPR_NAME) {
                _(badExpr(expr, "invalid method declarator", checker));
                break;
            }
            ns = Spk_METHOD_NAMESPACE_LVALUE;
            switch (expr->left->kind) {
            case Spk_EXPR_NAME:
                name = expr->left->sym;  Spk_INCREF(name);
                stmt->u.method.argList.fixed = expr->right;
                break;
            case Spk_EXPR_CALL:
                if (expr->left->oper != Spk_OPER_INDEX ||
                    expr->left->left->sym->sym != Spk_self) {
                    _(badExpr(expr, "invalid method declarator", checker));
                    break;
                }
                name = SpkSymbolNode_FromSymbol(
                    checker->st,
                    Spk___index__
                    );
                stmt->u.method.argList.fixed = expr->left->right;
                /* chain-on the new value arg */
                expr->left->right->nextArg = expr->right;  Spk_INCREF(expr->right);
                break;
            default:
                _(badExpr(expr, "invalid method declarator", checker));
                break;
            }
            break;
        case Spk_EXPR_CALL:
            if (expr->left->kind != Spk_EXPR_NAME) {
                _(badExpr(expr, "invalid method declarator", checker));
                break;
            }
            if (expr->left->sym->sym == Spk_self) {
                name = SpkSymbolNode_FromSymbol(
                    checker->st,
                    *Spk_operCallSelectors[expr->oper].selector
                    );
            } else {
                /*
                 * XXX: Should we allow "foo[...] {}" as a method
                 * definition?  More generally, could the method
                 * declarator be seen as the application of an inverse
                 * thingy?
                 */
                if (expr->oper != Spk_OPER_APPLY) {
                    _(badExpr(expr, "invalid method declarator", checker));
                    break;
                }
                name = expr->left->sym;  Spk_INCREF(name);
            }
            if (!outer || outer->kind != Spk_STMT_DEF_CLASS) {
                /* declare naked functions */
                _(SpkSymbolTable_Insert(checker->st, expr->left, checker->requestor));
            }
            stmt->u.method.argList.fixed = expr->right;
            stmt->u.method.argList.var = expr->var;
            break;
        case Spk_EXPR_UNARY:
        case Spk_EXPR_BINARY:
            if (expr->left->kind != Spk_EXPR_NAME ||
                expr->left->sym->sym != Spk_self) {
                _(badExpr(expr, "invalid method declarator", checker));
                break;
            }
            oper = expr->oper;
            if (expr->right) {
                switch (expr->right->kind) {
                case Spk_EXPR_NAME:
                    stmt->u.method.argList.fixed = expr->right;
                    break;
                case Spk_EXPR_LITERAL:
                    if (SpkHost_IsInteger(expr->right->aux.literalValue) &&
                        SpkHost_IntegerAsCLong(expr->right->aux.literalValue) == 1) {
                        if (oper == Spk_OPER_ADD) {
                            oper = Spk_OPER_SUCC;
                            break;
                        } else if (oper == Spk_OPER_SUB) {
                            oper = Spk_OPER_PRED;
                            break;
                        }
                    }
                    /* fall through */
                default:
                    _(badExpr(expr, "invalid method declarator", checker));
                    /* XXX: 'name' is defined in this error path (harmless) */
                    break;
                }
            }
            name = SpkSymbolNode_FromSymbol(
                checker->st,
                *Spk_operSelectors[oper].selector
                );
            break;
        case Spk_EXPR_KEYWORD:
            if (expr->left->kind != Spk_EXPR_NAME ||
                expr->left->sym->sym != Spk_self) {
                _(badExpr(expr, "invalid method declarator", checker));
                break;
            }
            name = SpkSymbolNode_FromSymbol(checker->st, expr->aux.keywords);
            stmt->u.method.argList.fixed = expr->right;
            break;
        default:
            _(badExpr(expr, "invalid method declarator", checker));
            break;
        }
        stmt->u.method.ns = ns;
        stmt->u.method.name = name;
        break;
    
    case 2:
        if (!outer || outer->kind != Spk_STMT_DEF_CLASS) {
            /* naked (global) function -- enter module instance context */
            SpkSymbolTable_EnterScope(checker->st, 1);
        }
        SpkSymbolTable_EnterScope(checker->st, 1);
        
        /* declare function arguments */
        for (arg = stmt->u.method.argList.fixed; arg; arg = arg->nextArg) {
            if (arg->kind != Spk_EXPR_NAME) {
                _(badExpr(arg, "invalid argument definition", checker));
                continue;
            }
            _(SpkSymbolTable_Insert(checker->st, arg, checker->requestor));
            ++stmt->u.method.argumentCount;
        }
        arg = stmt->u.method.argList.var;
        if (arg) {
            if (arg->kind != Spk_EXPR_NAME) {
                _(badExpr(arg, "invalid argument definition", checker));
            } else {
                _(SpkSymbolTable_Insert(checker->st, arg, checker->requestor));
            }
        }

        for (innerPass = 1; innerPass <= 3; ++innerPass) {
            for (s = body->top; s; s = s->next) {
                _(checkStmt(s, stmt, checker, innerPass));
            }
        }
        
        stmt->u.method.localCount = checker->st->currentScope->context->nDefs -
                                    stmt->u.method.argumentCount -
                                    (stmt->u.method.argList.var ? 1 : 0);
        
        SpkSymbolTable_ExitScope(checker->st);
        if (!outer || outer->kind != Spk_STMT_DEF_CLASS) {
            SpkSymbolTable_ExitScope(checker->st);
        }
        
        break;
    }
    
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static SpkUnknown *registerSubclass(Stmt *subclassDef, StaticChecker *checker) {
    Stmt *superclassDef;
    
    /* insert this class onto the superclass's subclass chain */
    superclassDef = subclassDef->u.klass.superclassDef;
    if (!superclassDef->u.klass.firstSubclassDef) {
        superclassDef->u.klass.firstSubclassDef = subclassDef;
    } else {
        superclassDef->u.klass.lastSubclassDef->u.klass.nextSubclassDef
            = subclassDef;
    }
    superclassDef->u.klass.lastSubclassDef = subclassDef;
    
    if (superclassDef->u.klass.predefined) {
        /* this is a 'root' class */
        if (!checker->rootClassList->first) {
            checker->rootClassList->first = subclassDef;
        } else {
            checker->rootClassList->last->u.klass.nextRootClassDef
                = subclassDef;
        }
        checker->rootClassList->last = subclassDef;
    }
    
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
}

static SpkUnknown *checkForSuperclassCycle(Stmt *aClassDef,
                                           StaticChecker *checker)
{
    /* Floyd's cycle-finding algorithm */
    Stmt *tortoise, *hare;
    
#define SUPERCLASS(stmt) (stmt)->u.klass.superclassDef
    tortoise = hare = SUPERCLASS(aClassDef);
    if (!tortoise)
        goto leave;
    hare = SUPERCLASS(hare);
    while (hare && tortoise != hare) {
        tortoise = SUPERCLASS(tortoise);
        hare = SUPERCLASS(hare);
        if (!hare)
            goto leave;
        hare = SUPERCLASS(hare);
    }
#undef SUPERCLASS
    
    if (hare) {
        _(badExpr(aClassDef->u.klass.superclassName,
                  "cycle in superclass chain",
                  checker));
    }
    
 leave:
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static SpkUnknown *checkClassBody(Stmt *body, Stmt *stmt, Stmt *outer,
                                  StaticChecker *checker, unsigned int outerPass,
                                  size_t *varCount)
{
    Stmt *s;
    unsigned int innerPass;
    
    ASSERT(body->kind == Spk_STMT_COMPOUND,
           "compound statement expected");
    SpkSymbolTable_EnterScope(checker->st, 1);
    for (innerPass = 1; innerPass <= 3; ++innerPass) {
        for (s = body->top; s; s = s->next) {
            _(checkStmt(s, stmt, checker, innerPass));
        }
    }
    *varCount = checker->st->currentScope->context->nDefs;
    SpkSymbolTable_ExitScope(checker->st);
    
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static SpkUnknown *checkClassDef(Stmt *stmt, Stmt *outer, StaticChecker *checker,
                                 unsigned int outerPass)
{
    Expr *expr;
        
    switch (outerPass) {
    case 1:
        ASSERT(stmt->expr->kind == Spk_EXPR_NAME,
               "identifier expected");
        _(SpkSymbolTable_Insert(checker->st, stmt->expr, checker->requestor));
        stmt->expr->u.def.stmt = stmt;
        
        _(checkExpr(stmt->u.klass.superclassName, stmt, checker, outerPass));
        
        break;
        
    case 2:
        _(checkExpr(stmt->u.klass.superclassName, stmt, checker, outerPass));
        
        expr = stmt->u.klass.superclassName->u.ref.def;
        if (expr) {
            if (expr->kind != Spk_EXPR_NAME || !expr->u.def.stmt) {
                _(badExpr(stmt->u.klass.superclassName,
                          "invalid superclass specification",
                          checker));
                break;
            }
            /* for convenience, cache a direct link to the superclass def */
            stmt->u.klass.superclassDef = expr->u.def.stmt;
            _(registerSubclass(stmt, checker));
        } /* else undefined */
        
        _(checkClassBody(stmt->top, stmt, outer, checker, outerPass,
                         &stmt->u.klass.instVarCount));
        if (stmt->bottom) {
            _(checkClassBody(stmt->bottom, stmt, outer, checker, outerPass,
                             &stmt->u.klass.classVarCount));
        }
        break;
        
    case 3:
        _(checkExpr(stmt->u.klass.superclassName, stmt, checker, outerPass));
        _(checkForSuperclassCycle(stmt, checker));
        break;
    }
    
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    return 0;
}

#ifdef MALTIPY
static SpkUnknown *checkImport(Stmt *stmt, StaticChecker *checker,
                               unsigned int pass)
{
    Expr *expr, *e, *def;
    
    if (pass != 1) {
        goto leave;
    }
    
    if (stmt->init) {
        /* "import a, b, c from spam.ham;" */
        for (def = stmt->init; def; def = def->next) {
            if (def->kind != Spk_EXPR_NAME) {
                _(badExpr(def, "invalid import statement", checker));
                goto leave;
            }
            def->u.def.weak = 1;
            _(SpkSymbolTable_Insert(checker->st, def, checker->requestor));
        }
        switch (stmt->expr->kind) {
        case Spk_EXPR_NAME:
        case Spk_EXPR_ATTR:
            break;
        default:
            _(badExpr(stmt->expr, "invalid import statement", checker));
            goto leave;
        }
        goto leave;
    }
    
    for (expr = stmt->expr; expr; expr = expr->next) {
        switch (expr->kind) {
        case Spk_EXPR_NAME:
            /* "import spam;" */
            def = expr;
            def->u.def.weak = 1;
            _(SpkSymbolTable_Insert(checker->st, def, checker->requestor));
            break;
        case Spk_EXPR_ATTR:
            /* "import spam.ham;" */
            def = expr->left;
            while (def->kind == Spk_EXPR_ATTR) {
                def = def->left;
            }
            if (def->kind != Spk_EXPR_NAME) {
                _(badExpr(def, "invalid import statement", checker));
                goto leave;
            }
            def->u.def.weak = 1;
            _(SpkSymbolTable_Insert(checker->st, def, checker->requestor));
            break;
        case Spk_EXPR_ASSIGN:
            /* "import ham = spam.ham;" */
            e = expr;
            do {
                def = e->left;
                if (def->kind != Spk_EXPR_NAME) {
                    _(badExpr(def, "invalid import statement", checker));
                    goto leave;
                }
                def->u.def.weak = 1;
                _(SpkSymbolTable_Insert(checker->st, def, checker->requestor));
                e = e->right;
            } while (e->kind == Spk_EXPR_ASSIGN);
            switch (e->kind) {
            case Spk_EXPR_NAME:
            case Spk_EXPR_ATTR:
                break;
            default:
                _(badExpr(e, "invalid import statement", checker));
                goto leave;
            }
            break;
        default:
            _(badExpr(expr, "invalid import statement", checker));
            goto leave;
        }
    }
    
 leave:
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    return 0;
}
#endif /* MALTIPY */

static SpkUnknown *checkStmt(Stmt *stmt, Stmt *outer, StaticChecker *checker,
                             unsigned int outerPass)
{
    switch (stmt->kind) {
    case Spk_STMT_BREAK:
    case Spk_STMT_CONTINUE:
        break;
    case Spk_STMT_COMPOUND:
        if (outerPass == 2) {
            Stmt *s;
            unsigned int innerPass;
            
            SpkSymbolTable_EnterScope(checker->st, 0);
            for (innerPass = 1; innerPass <= 3; ++innerPass) {
                for (s = stmt->top; s; s = s->next) {
                    _(checkStmt(s, stmt, checker, innerPass));
                }
            }
            SpkSymbolTable_ExitScope(checker->st);
        }
        break;
    case Spk_STMT_DEF_ARG:
        if (outerPass == 1) {
            _(badExpr(stmt->expr,
                      "'arg' can only be used as the first statement of a block",
                      checker
                  ));
        }
        break;
    case Spk_STMT_DEF_VAR:
        _(checkVarDefList(stmt->expr, stmt, checker, outerPass));
        break;
    case Spk_STMT_DEF_METHOD:
        _(checkMethodDef(stmt, outer, checker, outerPass));
        break;
    case Spk_STMT_DEF_CLASS:
        _(checkClassDef(stmt, outer, checker, outerPass));
        break;
    case Spk_STMT_DEF_MODULE:
        ASSERT(0, "unexpected module node");
        break;
    case Spk_STMT_DO_WHILE:
        _(checkExpr(stmt->expr, stmt, checker, outerPass));
        _(checkStmt(stmt->top, stmt, checker, outerPass));
        break;
    case Spk_STMT_EXPR:
        if (stmt->expr) {
            _(checkExpr(stmt->expr, stmt, checker, outerPass));
        }
        break;
    case Spk_STMT_FOR:
        if (stmt->init) {
            _(checkExpr(stmt->init, stmt, checker, outerPass));
        }
        if (stmt->expr) {
            _(checkExpr(stmt->expr, stmt, checker, outerPass));
        }
        if (stmt->incr) {
            _(checkExpr(stmt->incr, stmt, checker, outerPass));
        }
        _(checkStmt(stmt->top, stmt, checker, outerPass));
        break;
    case Spk_STMT_IF_ELSE:
        _(checkExpr(stmt->expr, stmt, checker, outerPass));
        _(checkStmt(stmt->top, stmt, checker, outerPass));
        if (stmt->bottom) {
            _(checkStmt(stmt->bottom, stmt, checker, outerPass));
        }
        break;
    case Spk_STMT_IMPORT:
#ifdef MALTIPY
        _(checkImport(stmt, checker, outerPass));
#else
        if (outerPass == 1) {
            _(badExpr(stmt->expr, "'import' not implemented", checker));
        }
#endif /* MALTIPY */
        break;
    case Spk_STMT_RAISE:
#ifdef MALTIPY
        _(checkExpr(stmt->expr, stmt, checker, outerPass));
#else
        if (outerPass == 1) {
            _(badExpr(stmt->expr, "'raise' not implemented", checker));
        }
#endif /* MALTIPY */
        break;
    case Spk_STMT_PRAGMA_SOURCE:
        _(Spk_SetAttr(Spk_GLOBAL(theInterpreter), checker->requestor, Spk_source, stmt->u.source));
        break;
    case Spk_STMT_RETURN:
    case Spk_STMT_YIELD:
        if (stmt->expr) {
            _(checkExpr(stmt->expr, stmt, checker, outerPass));
        }
        break;
    case Spk_STMT_WHILE:
        _(checkExpr(stmt->expr, stmt, checker, outerPass));
        _(checkStmt(stmt->top, stmt, checker, outerPass));
        break;
    }
    
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static struct PseudoVariable {
    const char *name;
    SpkOpcode pushOpcode;
} pseudoVariables[] = {
    { "self",        Spk_OPCODE_PUSH_SELF },
    { "super",       Spk_OPCODE_PUSH_SUPER },
    { "false",       Spk_OPCODE_PUSH_FALSE },
    { "true",        Spk_OPCODE_PUSH_TRUE },
    { "null",        Spk_OPCODE_PUSH_NULL },
    { "thisContext", Spk_OPCODE_PUSH_CONTEXT },
    { 0 }
};

static Expr *newNameExpr(void) {
    Expr *newExpr;
    
    newExpr = (Expr *)SpkObject_New(Spk_CLASS(Expr));        
    newExpr->kind = Spk_EXPR_NAME;
    return newExpr;
}

static Expr *newPseudoVariable(struct PseudoVariable *pv, StaticChecker *checker) {
    Expr *newExpr;
    
    newExpr = newNameExpr();
    newExpr->sym = SpkSymbolNode_FromCString(checker->st, pv->name);
    newExpr->u.def.pushOpcode = pv->pushOpcode;
    return newExpr;
}

static Stmt *predefinedClassDef(SpkClass *predefClass, StaticChecker *checker) {
    Stmt *newStmt;
    
    newStmt = (Stmt *)SpkObject_New(Spk_CLASS(Stmt));
    newStmt->kind = Spk_STMT_DEF_CLASS;
    newStmt->expr = newNameExpr();
    newStmt->expr->sym = SpkSymbolNode_FromSymbol(checker->st, predefClass->name);
    newStmt->expr->u.def.stmt = newStmt;
    newStmt->u.klass.predefined = 1;
    return newStmt;
}

static Stmt *globalVarDef(const char *name, StaticChecker *checker) {
    Stmt *newStmt;
    
    newStmt = (Stmt *)SpkObject_New(Spk_CLASS(Stmt));
    newStmt->kind = Spk_STMT_DEF_VAR;
    newStmt->expr = newNameExpr();
    newStmt->expr->sym = SpkSymbolNode_FromCString(checker->st, name);
    newStmt->expr->u.def.stmt = newStmt;
    return newStmt;
}

static SpkUnknown *addPredef(StmtList *predefList, Stmt *def) {
    if (!predefList->first) {
        predefList->first = def;
    } else {
        predefList->last->next = def;
    }
    predefList->last = def;
    
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
}

static SpkUnknown *declareClass(SpkBehavior *aClass,
                                StmtList *predefList,
                                StaticChecker *checker)
{
    Stmt *classDef = predefinedClassDef((SpkClass *)aClass, checker);
    _(SpkSymbolTable_Insert(checker->st, classDef->expr, checker->requestor));
    classDef->expr->u.def.initValue = (SpkUnknown *)aClass;
    _(addPredef(predefList, classDef));
    
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    return 0;
}

SpkUnknown *SpkStaticChecker_Check(Stmt *tree,
                                   SpkSymbolTable *st,
                                   SpkUnknown *requestor)
{
    StmtList *predefList;
    StmtList *rootClassList;
    Stmt *s;
    StaticChecker checker;
    struct PseudoVariable *pv;
    unsigned int pass;
    SpkClassBootRec *classBootRec; SpkBehavior **classVar;
    SpkVarBootRec *varBootRec; SpkUnknown **var;
    
    ASSERT(tree->kind == Spk_STMT_DEF_MODULE, "module node expected");
    ASSERT(tree->top->kind == Spk_STMT_COMPOUND, "compound statement expected");
    predefList = &tree->u.module.predefList;
    rootClassList = &tree->u.module.rootClassList;
    
    checker.st = st;
    checker.requestor = requestor;
    SpkSymbolTable_EnterScope(checker.st, 1); /* built-in scope */
    for (pv = pseudoVariables; pv->name; ++pv) {
        Expr *pvDef = newPseudoVariable(pv, &checker);
        _(SpkSymbolTable_Insert(checker.st, pvDef, checker.requestor));
        Spk_DECREF(pvDef);
    }
    SpkSymbolTable_EnterScope(checker.st, 1); /* global scope */
    
    predefList->first = predefList->last = 0;
    /* XXX: What about the other core classes? */
    _(declareClass(Spk_CLASS(Object), predefList, &checker));
#ifndef MALTIPY
    _(declareClass(Spk_CLASS(Array), predefList, &checker));
#endif /* !MALTIPY */
    for (classBootRec = Spk_classBootRec; *classBootRec; ++classBootRec) {
        classVar = (SpkBehavior **)((char *)Spk_heart + (*classBootRec)->classVarOffset);
        _(declareClass(*classVar, predefList, &checker));
    }
    for (varBootRec = Spk_globalVarBootRec; varBootRec->varOffset; ++varBootRec) {
        Stmt *varDef = globalVarDef(varBootRec->name, &checker);
        _(SpkSymbolTable_Insert(checker.st, varDef->expr, checker.requestor));
        var = (SpkUnknown **)((char *)Spk_heart + varBootRec->varOffset);
        varDef->expr->u.def.initValue = (SpkUnknown *)*var;
        _(addPredef(predefList, varDef));
    }
    
    checker.rootClassList = rootClassList;
    rootClassList->first = rootClassList->last = 0;

    for (pass = 1; pass <= 3; ++pass) {
        for (s = tree->top->top; s; s = s->next) {
            _(checkStmt(s, 0, &checker, pass));
        }
    }
    
    tree->u.module.dataSize = checker.st->currentScope->context->nDefs;
    
    SpkSymbolTable_ExitScope(checker.st); /* global */
    SpkSymbolTable_ExitScope(checker.st); /* built-in */
    
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    return 0;
}
