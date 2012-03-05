
#include "scheck.h"

#include "array.h"
#include "behavior.h"
#include "boot.h"
#include "class.h"
#include "heart.h"
#include "int.h"
#include "interp.h"
#include "native.h"
#include "rodata.h"
#include "st.h"
#include "str.h"
#include "tree.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int declareBuiltIn = 1;
int declareObject = 1;


#define ASSERT(c, msg) \
do if (!(c)) { Halt(HALT_ASSERTION_ERROR, (msg)); goto unwind; } \
while (0)

#define _(c) do { \
Unknown *_tmp = (c); \
if (!_tmp) goto unwind; } while (0)


typedef struct StaticChecker {
    SymbolTable *st;
    StmtList *rootClassList;
    Unknown *requestor;
    Stmt *currentMethod;
} StaticChecker;


static Unknown *notifyBadExpr(Expr *expr, const char *desc, StaticChecker *checker) {
    Unknown *descStr, *tmp;
    
    descStr = (Unknown *)String_FromCString(desc);
    if (!descStr)
        return 0;
    tmp = Send(GLOBAL(theInterpreter),
                   checker->requestor,
                   badExpr,
                   expr,
                   descStr,
                   0);
    if (!tmp)
        return 0;
    return GLOBAL(xvoid);
}


static Unknown *checkStmt(Stmt *, Stmt *, StaticChecker *, unsigned int);
static Unknown *checkExpr(Expr *, Stmt *, StaticChecker *, unsigned int);
static Unknown *checkOneExpr(Expr *, Stmt *, StaticChecker *, unsigned int);


static Unknown *checkDeclSpecs(unsigned int *specifiers,
                                  Expr *declSpecs,
                                  StaticChecker *checker,
                                  unsigned int pass)
{
    Expr *declSpec;
    Stmt *specDef;
    
    *specifiers = 0;
    
    for (declSpec = declSpecs; declSpec; declSpec = declSpec->next) {
        ASSERT(declSpec->kind == EXPR_NAME, "identifier expected");
        _(SymbolTable_Bind(checker->st, declSpec, checker->requestor));
        if (declSpec->u.ref.def) {
            /* XXX: check for invalid combinations */
            specDef = declSpec->u.ref.def->u.def.stmt;
            if (*specifiers & specDef->u.spec.mask)
                _(notifyBadExpr(declSpec, "invalid specifier", checker));
            *specifiers = (*specifiers & ~specDef->u.spec.mask) | specDef->u.spec.value;
        }
    }
    
    return GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static Unknown *checkVarDefList(Expr *defList,
                                   Stmt *stmt,
                                   StaticChecker *checker,
                                   unsigned int pass)
{
    Expr *expr, *def;
    unsigned int specifiers;
    
    if (pass == 1)
        _(checkDeclSpecs(&specifiers, defList->declSpecs, checker, pass));
    
    for (expr = defList; expr; expr = expr->next) {
        if (expr->kind == EXPR_ASSIGN) {
            def = expr->left;
            _(checkExpr(expr->right, stmt, checker, pass));
        } else {
            def = expr;
        }
        while (def->kind == EXPR_UNARY && def->oper == OPER_IND) {
            def = def->left;
        }
        if (def->kind != EXPR_NAME) {
            if (pass == 1)
                _(notifyBadExpr(def, "invalid variable definition", checker));
            continue;
        }
        if (pass == 1) {
            _(SymbolTable_Insert(checker->st, def, checker->requestor));
            expr->u.def.stmt = stmt;
            expr->specifiers = specifiers;
        }
    }
    return GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static Unknown *checkExpr(Expr *expr, Stmt *stmt, StaticChecker *checker,
                             unsigned int pass)
{
    for ( ; expr; expr = expr->next) {
        _(checkOneExpr(expr, stmt, checker, pass));
    }
    return GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static Unknown *checkBlock(Expr *expr, Stmt *stmt, StaticChecker *checker,
                              unsigned int outerPass)
{
    Expr *arg;
    Stmt *s;
    unsigned int innerPass;
    
    if (outerPass != 2) {
        goto leave;
    }
    
    ++checker->currentMethod->u.method.blockCount;
    
    SymbolTable_EnterScope(checker->st, 0);
    
    expr->u.def.index = 0;
    
    if (expr->aux.block.argList) {
        /* declare block arguments */
        for (arg = expr->aux.block.argList; arg; arg = arg->nextArg) {
            if (arg->kind != EXPR_NAME) {
                _(notifyBadExpr(arg, "invalid argument definition", checker));
                continue;
            }
            _(SymbolTable_Insert(checker->st, arg, checker->requestor));
            ++expr->aux.block.argumentCount;
        }
        
        /* The 'index' of the block itself is the index of the first
         * argument.  The interpreter uses this index to find the
         * destination the block arguments; see OPCODE_CALL_BLOCK.
         */
        expr->u.def.index = expr->aux.block.argList->u.def.index;
    }
    
    for (innerPass = 1; innerPass <= 3; ++innerPass) {
        if (expr->aux.block.stmtList) {
            for (s = expr->aux.block.stmtList; s; s = s->next) {
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
    
    SymbolTable_ExitScope(checker->st);
    
 leave:
    return GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static Unknown *checkOneExpr(Expr *expr, Stmt *stmt, StaticChecker *checker,
                                unsigned int pass)
{
    Expr *arg;
    
    switch (expr->kind) {
    case EXPR_LITERAL:
        break;
    case EXPR_NAME:
        if (pass == 2) {
            _(SymbolTable_Bind(checker->st, expr, checker->requestor));
        }
        break;
    case EXPR_BLOCK:
        _(checkBlock(expr, stmt, checker, pass));
        break;
    case EXPR_COMPOUND:
        for (arg = expr->right; arg; arg = arg->nextArg) {
            _(checkExpr(arg, stmt, checker, pass));
        }
        if (expr->var) {
            _(checkExpr(expr->var, stmt, checker, pass));
        }
        break;
    case EXPR_CALL:
    case EXPR_KEYWORD:
        _(checkExpr(expr->left, stmt, checker, pass));
        for (arg = expr->right; arg; arg = arg->nextArg) {
            _(checkExpr(arg, stmt, checker, pass));
        }
        if (expr->var) {
            _(checkExpr(expr->var, stmt, checker, pass));
        }
        break;
    case EXPR_ATTR:
    case EXPR_POSTOP:
    case EXPR_PREOP:
    case EXPR_UNARY:
        _(checkExpr(expr->left, stmt, checker, pass));
        break;
    case EXPR_AND:
    case EXPR_ASSIGN:
    case EXPR_ATTR_VAR:
    case EXPR_BINARY:
    case EXPR_ID:
    case EXPR_NI:
    case EXPR_OR:
        _(checkExpr(expr->left, stmt, checker, pass));
        _(checkExpr(expr->right, stmt, checker, pass));
        break;
    case EXPR_COND:
        _(checkExpr(expr->cond, stmt, checker, pass));
        _(checkExpr(expr->left, stmt, checker, pass));
        _(checkExpr(expr->right, stmt, checker, pass));
        break;
    }
    
    return GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static Unknown *checkMethodDef(Stmt *stmt,
                                  Stmt *outer,
                                  StaticChecker *checker,
                                  unsigned int outerPass)
{
    Stmt *body, *s;
    Expr *expr, *arg, *def;
    MethodNamespace ns;
    SymbolNode *name;
    Oper oper;
    unsigned int innerPass;
    unsigned int specifiers;
    
    checker->currentMethod = stmt;
    
    expr = stmt->expr;
    body = stmt->top;
    ASSERT(body->kind == STMT_COMPOUND,
           "compound statement expected");
    
    switch (outerPass) {
    case 1:
        _(checkDeclSpecs(&specifiers, expr->declSpecs, checker, outerPass));
        expr->specifiers = specifiers;
        ns = METHOD_NAMESPACE_RVALUE;
        name = 0;
        /* XXX: This can only be meaningfully composed with EXPR_CALL. */
        while (expr->kind == EXPR_UNARY && expr->oper == OPER_IND) {
            expr = expr->left;
        }
        switch (expr->kind) {
        case EXPR_NAME:
            name = expr->sym;
            break;
        case EXPR_ASSIGN:
            if (expr->right->kind != EXPR_NAME) {
                _(notifyBadExpr(expr, "invalid method declarator", checker));
                break;
            }
            ns = METHOD_NAMESPACE_LVALUE;
            switch (expr->left->kind) {
            case EXPR_NAME:
                name = expr->left->sym;
                stmt->u.method.argList.fixed = expr->right;
                break;
            case EXPR_CALL:
                if (expr->left->oper != OPER_INDEX ||
                    expr->left->left->sym->sym != self) {
                    _(notifyBadExpr(expr, "invalid method declarator", checker));
                    break;
                }
                name = SymbolNode_FromSymbol(
                    checker->st,
                    __index__
                    );
                stmt->u.method.argList.fixed = expr->left->right;
                /* chain-on the new value arg */
                expr->left->right->nextArg = expr->right;
                break;
            default:
                _(notifyBadExpr(expr, "invalid method declarator", checker));
                break;
            }
            break;
        case EXPR_CALL:
            if (expr->left->kind != EXPR_NAME) {
                _(notifyBadExpr(expr, "invalid method declarator", checker));
                break;
            }
            if (expr->left->sym->sym == self) {
                name = SymbolNode_FromSymbol(
                    checker->st,
                    *operCallSelectors[expr->oper].selector
                    );
            } else {
                /*
                 * XXX: Should we allow "foo[...] {}" as a method
                 * definition?  More generally, could the method
                 * declarator be seen as the application of an inverse
                 * thingy?
                 */
                if (expr->oper != OPER_APPLY) {
                    _(notifyBadExpr(expr, "invalid method declarator", checker));
                    break;
                }
                name = expr->left->sym;
            }
            if (!outer || outer->kind != STMT_DEF_CLASS) {
                /* declare naked functions */
                _(SymbolTable_Insert(checker->st, expr->left, checker->requestor));
                expr->left->specifiers = specifiers;
            }
            stmt->u.method.argList.fixed = expr->right;
            stmt->u.method.argList.var = expr->var;
            break;
        case EXPR_UNARY:
        case EXPR_BINARY:
            if (expr->left->kind != EXPR_NAME ||
                expr->left->sym->sym != self) {
                _(notifyBadExpr(expr, "invalid method declarator", checker));
                break;
            }
            oper = expr->oper;
            if (expr->right) {
                switch (expr->right->kind) {
                case EXPR_NAME:
                    stmt->u.method.argList.fixed = expr->right;
                    break;
                case EXPR_LITERAL:
                    if (IsInteger(expr->right->aux.literalValue) &&
                        Integer_AsCLong((Integer *)expr->right->aux.literalValue) == 1) {
                        if (oper == OPER_ADD) {
                            oper = OPER_SUCC;
                            break;
                        } else if (oper == OPER_SUB) {
                            oper = OPER_PRED;
                            break;
                        }
                    }
                    /* fall through */
                default:
                    _(notifyBadExpr(expr, "invalid method declarator", checker));
                    /* XXX: 'name' is defined in this error path (harmless) */
                    break;
                }
            }
            name = SymbolNode_FromSymbol(
                checker->st,
                *operSelectors[oper].selector
                );
            break;
        case EXPR_KEYWORD:
            if (expr->left->kind != EXPR_NAME ||
                expr->left->sym->sym != self) {
                _(notifyBadExpr(expr, "invalid method declarator", checker));
                break;
            }
            name = SymbolNode_FromSymbol(checker->st, (struct Symbol *)expr->aux.keywords);
            stmt->u.method.argList.fixed = expr->right;
            break;
        default:
            _(notifyBadExpr(expr, "invalid method declarator", checker));
            break;
        }
        stmt->u.method.ns = ns;
        stmt->u.method.name = name;
        break;
    
    case 2:
        if (!outer || outer->kind != STMT_DEF_CLASS) {
            /* naked (global) function -- enter module instance context */
            SymbolTable_EnterScope(checker->st, 1);
        }
        SymbolTable_EnterScope(checker->st, 1);
        
        /* declare function arguments */
        for (arg = stmt->u.method.argList.fixed; arg; arg = arg->nextArg) {
            def = arg;
            while (def->kind == EXPR_UNARY && def->oper == OPER_IND) {
                def = def->left;
            }
            if (def->kind != EXPR_NAME)
                break;
            checkDeclSpecs(&specifiers, def->declSpecs, checker, 1);
            def->specifiers = specifiers;
            _(SymbolTable_Insert(checker->st, def, checker->requestor));
            ++stmt->u.method.minArgumentCount;
            ++stmt->u.method.maxArgumentCount;
        }
        for ( ; arg; arg = arg->nextArg) {
            if (arg->kind == EXPR_ASSIGN) {
                def = arg->left;
            } else {
                def = arg;
                if (def->kind == EXPR_NAME) {
                    _(notifyBadExpr(arg, "non-default argument follows default argument", checker));
                    /* fall though and define it, to reduce the number of errors */
                }
            }
            while (def->kind == EXPR_UNARY && def->oper == OPER_IND) {
                def = def->left;
            }
            if (def->kind != EXPR_NAME) {
                _(notifyBadExpr(arg, "invalid argument definition", checker));
                continue;
            }
            _(SymbolTable_Insert(checker->st, def, checker->requestor));
            ++stmt->u.method.maxArgumentCount;
        }
        arg = stmt->u.method.argList.var;
        if (arg) {
            if (arg->kind != EXPR_NAME) {
                _(notifyBadExpr(arg, "invalid argument definition", checker));
            } else {
                _(SymbolTable_Insert(checker->st, arg, checker->requestor));
            }
        }
        
        if (IS_EXTERN(expr) && body->top) {
            _(notifyBadExpr(expr, "'extern' method with a non-empty body", checker));
        }

        for (innerPass = 1; innerPass <= 3; ++innerPass) {
            for (arg = stmt->u.method.argList.fixed; arg; arg = arg->nextArg) {
                if (arg->kind == EXPR_ASSIGN) {
                    _(checkExpr(arg->right, stmt, checker, innerPass));
                }
            }
            for (s = body->top; s; s = s->next) {
                _(checkStmt(s, stmt, checker, innerPass));
            }
        }
        
        stmt->u.method.localCount = checker->st->currentScope->context->nDefs -
                                    stmt->u.method.maxArgumentCount -
                                    (stmt->u.method.argList.var ? 1 : 0);
        
        SymbolTable_ExitScope(checker->st);
        if (!outer || outer->kind != STMT_DEF_CLASS) {
            SymbolTable_ExitScope(checker->st);
        }
        
        break;
    }
    
    checker->currentMethod = 0;
    
    return GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static Unknown *registerSubclass(Stmt *subclassDef, StaticChecker *checker) {
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
    
    return GLOBAL(xvoid);
}

static Unknown *checkForSuperclassCycle(Stmt *aClassDef,
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
        _(notifyBadExpr(aClassDef->u.klass.superclassName,
                  "cycle in superclass chain",
                  checker));
    }
    
 leave:
    return GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static Unknown *checkClassBody(Stmt *body, Stmt *stmt, Stmt *outer,
                                  StaticChecker *checker, unsigned int outerPass,
                                  size_t *varCount)
{
    Stmt *s;
    unsigned int innerPass;
    
    ASSERT(body->kind == STMT_COMPOUND,
           "compound statement expected");
    SymbolTable_EnterScope(checker->st, 1);
    for (innerPass = 1; innerPass <= 3; ++innerPass) {
        for (s = body->top; s; s = s->next) {
            _(checkStmt(s, stmt, checker, innerPass));
        }
    }
    *varCount = checker->st->currentScope->context->nDefs;
    SymbolTable_ExitScope(checker->st);
    
    return GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static Unknown *checkClassDef(Stmt *stmt, Stmt *outer, StaticChecker *checker,
                                 unsigned int outerPass)
{
    Expr *expr;
        
    switch (outerPass) {
    case 1:
        ASSERT(stmt->expr->kind == EXPR_NAME,
               "identifier expected");
        _(SymbolTable_Insert(checker->st, stmt->expr, checker->requestor));
        stmt->expr->u.def.stmt = stmt;
        
        _(checkExpr(stmt->u.klass.superclassName, stmt, checker, outerPass));
        
        break;
        
    case 2:
        _(checkExpr(stmt->u.klass.superclassName, stmt, checker, outerPass));
        
        expr = stmt->u.klass.superclassName->u.ref.def;
        if (expr) {
            if (expr->kind == EXPR_NAME &&
                expr->u.def.stmt &&
                expr->u.def.stmt->kind == STMT_DEF_CLASS)
            {
                /* for convenience, cache a direct link to the superclass def */
                stmt->u.klass.superclassDef = expr->u.def.stmt;
                _(registerSubclass(stmt, checker));
            } else if (expr->u.def.pushOpcode == OPCODE_PUSH_NULL) {
                /* for class Object, which has no superclass */
            } else {
                _(notifyBadExpr(stmt->u.klass.superclassName,
                          "invalid superclass specification",
                          checker));
                break;
            }
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
    
    return GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static Unknown *checkStmt(Stmt *stmt, Stmt *outer, StaticChecker *checker,
                             unsigned int outerPass)
{
    switch (stmt->kind) {
    case STMT_BREAK:
    case STMT_CONTINUE:
        break;
    case STMT_COMPOUND:
        if (outerPass == 2) {
            Stmt *s;
            unsigned int innerPass;
            
            SymbolTable_EnterScope(checker->st, 0);
            for (innerPass = 1; innerPass <= 3; ++innerPass) {
                for (s = stmt->top; s; s = s->next) {
                    _(checkStmt(s, stmt, checker, innerPass));
                }
            }
            SymbolTable_ExitScope(checker->st);
        }
        break;
    case STMT_DEF_VAR:
        _(checkVarDefList(stmt->expr, stmt, checker, outerPass));
        break;
    case STMT_DEF_METHOD:
        _(checkMethodDef(stmt, outer, checker, outerPass));
        break;
    case STMT_DEF_CLASS:
        _(checkClassDef(stmt, outer, checker, outerPass));
        break;
    case STMT_DEF_MODULE:
        ASSERT(0, "unexpected module node");
        break;
    case STMT_DEF_SPEC:
        ASSERT(0, "unexpected spec node");
        break;
    case STMT_DO_WHILE:
        _(checkExpr(stmt->expr, stmt, checker, outerPass));
        _(checkStmt(stmt->top, stmt, checker, outerPass));
        break;
    case STMT_EXPR:
        if (stmt->expr) {
            _(checkExpr(stmt->expr, stmt, checker, outerPass));
        }
        break;
    case STMT_FOR:
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
    case STMT_IF_ELSE:
        _(checkExpr(stmt->expr, stmt, checker, outerPass));
        _(checkStmt(stmt->top, stmt, checker, outerPass));
        if (stmt->bottom) {
            _(checkStmt(stmt->bottom, stmt, checker, outerPass));
        }
        break;
    case STMT_PRAGMA_SOURCE:
        _(SetAttr(GLOBAL(theInterpreter), checker->requestor, source, (Unknown *)stmt->u.source));
        break;
    case STMT_RETURN:
    case STMT_YIELD:
        if (stmt->expr) {
            _(checkExpr(stmt->expr, stmt, checker, outerPass));
        }
        break;
    case STMT_WHILE:
        _(checkExpr(stmt->expr, stmt, checker, outerPass));
        _(checkStmt(stmt->top, stmt, checker, outerPass));
        break;
    }
    
    return GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static struct PseudoVariable {
    const char *name;
    Opcode pushOpcode;
} pseudoVariables[] = {
    { "self",        OPCODE_PUSH_SELF },
    { "super",       OPCODE_PUSH_SUPER },
    { "false",       OPCODE_PUSH_FALSE },
    { "true",        OPCODE_PUSH_TRUE },
    { "null",        OPCODE_PUSH_NULL },
    { "thisContext", OPCODE_PUSH_CONTEXT },
    { 0 }
};

static struct Spec {
    const char *name;
    unsigned int mask;
    unsigned int value;
} builtInSpecifiers[] = {
    { "obj",     SPEC_TYPE,       SPEC_TYPE_OBJ  },
    { "int",     SPEC_TYPE,       SPEC_TYPE_INT  },
    { "char",    SPEC_TYPE,       SPEC_TYPE_CHAR },
    { "import",  SPEC_STORAGE,    SPEC_STORAGE_IMPORT },
    { "export",  SPEC_STORAGE,    SPEC_STORAGE_EXPORT },
    { "extern",  SPEC_STORAGE,    SPEC_STORAGE_EXTERN },
    { "cdecl",   SPEC_CALL_CONV,  SPEC_CALL_CONV_C },
    { 0 }
};

static Expr *newNameExpr(void) {
    Expr *newExpr;
    
    newExpr = (Expr *)Object_New(CLASS(XExpr));        
    newExpr->kind = EXPR_NAME;
    return newExpr;
}

static void declareBuiltInSpecifier(struct Spec *bis, SymbolTable *st) {
    Expr *nameExpr;
    Stmt *specDef;
    
    nameExpr = newNameExpr();
    
    specDef = (Stmt *)Object_New(CLASS(XStmt));
    specDef->kind = STMT_DEF_SPEC;
    specDef->expr = nameExpr;
    specDef->expr->sym = SymbolNode_FromCString(st, bis->name);
    specDef->expr->u.def.stmt = specDef;
    specDef->u.spec.mask = bis->mask;
    specDef->u.spec.value = bis->value;
    
    SymbolTable_Insert(st, specDef->expr, 0);
}

static Expr *newPseudoVariable(struct PseudoVariable *pv, SymbolTable *st) {
    Expr *newExpr;
    
    newExpr = newNameExpr();
    newExpr->sym = SymbolNode_FromCString(st, pv->name);
    newExpr->u.def.pushOpcode = pv->pushOpcode;
    return newExpr;
}

static Stmt *predefinedClassDef(Class *predefClass, StaticChecker *checker) {
    Stmt *newStmt;
    
    newStmt = (Stmt *)Object_New(CLASS(XStmt));
    newStmt->kind = STMT_DEF_CLASS;
    newStmt->expr = newNameExpr();
    newStmt->expr->sym = SymbolNode_FromSymbol(checker->st, predefClass->name);
    newStmt->expr->u.def.stmt = newStmt;
    newStmt->u.klass.predefined = 1;
    return newStmt;
}

static Stmt *globalVarDef(const char *name, StaticChecker *checker) {
    Stmt *newStmt;
    
    newStmt = (Stmt *)Object_New(CLASS(XStmt));
    newStmt->kind = STMT_DEF_VAR;
    newStmt->expr = newNameExpr();
    newStmt->expr->sym = SymbolNode_FromCString(checker->st, name);
    newStmt->expr->u.def.stmt = newStmt;
    return newStmt;
}

static Unknown *addPredef(StmtList *predefList, Stmt *def) {
    if (!predefList->first) {
        predefList->first = def;
    } else {
        predefList->last->next = def;
    }
    predefList->last = def;
    
    return GLOBAL(xvoid);
}

static Unknown *declareClass(Behavior *aClass,
                                StmtList *predefList,
                                StaticChecker *checker)
{
    Stmt *classDef = predefinedClassDef((Class *)aClass, checker);
    _(SymbolTable_Insert(checker->st, classDef->expr, checker->requestor));
    classDef->expr->u.def.initValue = (Unknown *)aClass;
    _(addPredef(predefList, classDef));
    
    return GLOBAL(xvoid);
    
 unwind:
    return 0;
}

Unknown *StaticChecker_DeclareBuiltIn(SymbolTable *st,
                                            Unknown *requestor)
{
    struct PseudoVariable *pv;
    struct Spec *bis;
    
    /* XXX: where to exit this scope? */
    SymbolTable_EnterScope(st, 1); /* built-in scope */
    
    for (pv = pseudoVariables; pv->name; ++pv) {
        Expr *pvDef = newPseudoVariable(pv, st);
        _(SymbolTable_Insert(st, pvDef, requestor));
    }
    
    for (bis = builtInSpecifiers; bis->name; ++bis) {
        declareBuiltInSpecifier(bis, st);
    }
    
    return GLOBAL(xvoid);
    
 unwind:
    return 0;
}

Unknown *StaticChecker_Check(Stmt *tree,
                                   SymbolTable *st,
                                   Unknown *requestor)
{
    StmtList *predefList;
    StmtList *rootClassList;
    Stmt *s;
    StaticChecker checker;
    unsigned int pass;
    ClassBootRec *cbr; Behavior **classVar;
    VarBootRec *varBootRec; Unknown **var;
    
    ASSERT(tree->kind == STMT_DEF_MODULE, "module node expected");
    ASSERT(tree->top->kind == STMT_COMPOUND, "compound statement expected");
    predefList = &tree->u.module.predefList;
    rootClassList = &tree->u.module.rootClassList;
    
    checker.st = st;
    checker.requestor = requestor;
    checker.currentMethod = 0;
    SymbolTable_EnterScope(checker.st, 1); /* global scope */
    
    predefList->first = predefList->last = 0;
    /* 'Object' is always needed -- see Parser_NewClassDef() */
    if (declareObject) /* Well, almost always. */
        _(declareClass(CLASS(Object), predefList, &checker));
    if (declareBuiltIn) {
        /* XXX: What about the other core classes? */
        _(declareClass(CLASS(Array), predefList, &checker));
        _(declareClass(CLASS(IdentityDictionary), predefList, &checker));
        _(declareClass(CLASS(Symbol), predefList, &checker));
        for (cbr = essentialClassBootRec; *cbr; ++cbr) {
            classVar = (Behavior **)((char *)heart + (*cbr)->classVarOffset);
            _(declareClass(*classVar, predefList, &checker));
        }
        for (cbr = classBootRec; *cbr; ++cbr) {
            classVar = (Behavior **)((char *)heart + (*cbr)->classVarOffset);
            _(declareClass(*classVar, predefList, &checker));
        }
    }
    for (varBootRec = globalVarBootRec; varBootRec->varOffset; ++varBootRec) {
        Stmt *varDef = globalVarDef(varBootRec->name, &checker);
        _(SymbolTable_Insert(checker.st, varDef->expr, checker.requestor));
        var = (Unknown **)((char *)heart + varBootRec->varOffset);
        varDef->expr->u.def.initValue = (Unknown *)*var;
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
    
    SymbolTable_ExitScope(checker.st); /* global */
    
    return GLOBAL(xvoid);
    
 unwind:
    return 0;
}
