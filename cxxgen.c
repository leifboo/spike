
#include "cxxgen.h"

#include "heart.h"
#include "host.h"
#include "native.h"
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


typedef struct CxxCodeGen {
    FILE *out;
    int indent;
} CxxCodeGen;


static SpkUnknown *emitCxxCodeForStmt(Stmt *, Stmt *, CxxCodeGen *, unsigned int);
static SpkUnknown *emitCxxCodeForExpr(Expr *, Stmt *, CxxCodeGen *, unsigned int);
static SpkUnknown *emitCxxCodeForOneExpr(Expr *, Stmt *, CxxCodeGen *, unsigned int);
static SpkUnknown *emitCxxCodeForMethodDef(Stmt *, Stmt *, CxxCodeGen *, unsigned int);

static void indent(CxxCodeGen *cgen) {
    int i;
    
    for (i = 0; i < cgen->indent; ++i)
        fputs("    ", cgen->out);
}

static void emitDeclSpecs(Expr *def, CxxCodeGen *cgen) {
    int type = 0;
#if 0
    if (def->declSpecs & Spk_DECL_SPEC_INT) {
        fputs("int ", cgen->out);
        type = 1;
    }
#endif
    if (!type) {
        fputs("obj ", cgen->out);
    }
}

static SpkUnknown *emitCxxCodeForVarDefList(Expr *defList,
                                            Stmt *stmt,
                                            CxxCodeGen *cgen,
                                            unsigned int pass)
{
    Expr *expr, *def;
    
    emitDeclSpecs(defList, cgen);
    for (expr = defList; expr; expr = expr->next) {
        if (expr->kind == Spk_EXPR_ASSIGN) {
            def = expr->left;
            ASSERT(0, "XXX: initializers");
            _(emitCxxCodeForExpr(expr->right, stmt, cgen, pass));
        } else {
            def = expr;
        }
        ASSERT(def->kind == Spk_EXPR_NAME, "invalid variable definition");
        fprintf(cgen->out, "%s%s",
                SpkHost_SymbolAsCString(def->sym->sym),
                expr->next ? ", " : "");
    }
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static SpkUnknown *emitCxxCodeForExpr(Expr *expr, Stmt *stmt, CxxCodeGen *cgen,
                                      unsigned int pass)
{
    for ( ; expr; expr = expr->next) {
        _(emitCxxCodeForOneExpr(expr, stmt, cgen, pass));
    }
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static SpkUnknown *emitCxxCodeForBlock(Expr *expr, Stmt *stmt, CxxCodeGen *cgen,
                                       unsigned int outerPass)
{
    Expr *arg;
    Stmt *firstStmt, *s;
    unsigned int innerPass;
    
    if (outerPass != 2) {
        goto leave;
    }
    
    firstStmt = expr->aux.block.stmtList;
    
    if (firstStmt && firstStmt->kind == Spk_STMT_DEF_VAR) {
        /* block arguments */
        for (arg = firstStmt->expr; arg; arg = arg->next) {
            ASSERT(arg->kind == Spk_EXPR_NAME, "invalid argument definition");
        }
        
        firstStmt = firstStmt->next;
    }
    
    for (innerPass = 1; innerPass <= 2; ++innerPass) {
        if (expr->aux.block.stmtList) {
            for (s = firstStmt; s; s = s->next) {
                _(emitCxxCodeForStmt(s, stmt, cgen, innerPass));
            }
        }
        if (expr->right) {
            _(emitCxxCodeForExpr(expr->right, stmt, cgen, innerPass));
        }
    }
    
 leave:
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static SpkUnknown *emitCxxCodeForOneExpr(Expr *expr, Stmt *stmt, CxxCodeGen *cgen,
                                         unsigned int pass)
{
    Expr *arg;
    const char *token;
    
    /* account for precedence */
    fputs("(", cgen->out);
    
    switch (expr->kind) {
    case Spk_EXPR_LITERAL:
        /* XXX: kludge */
        SpkHost_PrintObject(expr->aux.literalValue, cgen->out);
        break;
    case Spk_EXPR_SYMBOL:
        ASSERT(0, "XXX");
        break;
    case Spk_EXPR_NAME:
        fputs(SpkHost_SelectorAsCString(expr->u.ref.def->sym->sym), cgen->out);
        break;
    case Spk_EXPR_BLOCK:
        _(emitCxxCodeForBlock(expr, stmt, cgen, pass));
        break;
    case Spk_EXPR_COMPOUND:
        for (arg = expr->right; arg; arg = arg->nextArg) {
            _(emitCxxCodeForExpr(arg, stmt, cgen, pass));
        }
        if (expr->var) {
            _(emitCxxCodeForExpr(expr->var, stmt, cgen, pass));
        }
        break;
    case Spk_EXPR_CALL:
    case Spk_EXPR_KEYWORD:
        _(emitCxxCodeForExpr(expr->left, stmt, cgen, pass));
        fputs("(", cgen->out);
        for (arg = expr->right; arg; arg = arg->nextArg) {
            _(emitCxxCodeForExpr(arg, stmt, cgen, pass));
            if (arg->nextArg)
                fputs(", ", cgen->out);
        }
        if (expr->var) {
            _(emitCxxCodeForExpr(expr->var, stmt, cgen, pass));
        }
        fputs(")", cgen->out);
        break;
    case Spk_EXPR_ATTR:
        _(emitCxxCodeForExpr(expr->left, stmt, cgen, pass));
        fputs(".", cgen->out);
        fputs(SpkHost_SelectorAsCString(expr->sym->sym), cgen->out);
        break;
    case Spk_EXPR_POSTOP:
    case Spk_EXPR_PREOP:
    case Spk_EXPR_UNARY:
        switch (expr->oper) {
        case Spk_OPER_SUCC:  token = "++";  break;
        case Spk_OPER_PRED:  token = "--";  break;
        case Spk_OPER_ADDR:  token = "&";   break;
        case Spk_OPER_IND:   token = "*";   break;
        case Spk_OPER_POS:   token = "+";   break;
        case Spk_OPER_NEG:   token = "-";   break;
        case Spk_OPER_BNEG:  token = "~";   break;
        case Spk_OPER_LNEG:  token = "!";   break;
        }
        if (expr->kind == Spk_EXPR_POSTOP) {
            _(emitCxxCodeForExpr(expr->left, stmt, cgen, pass));
            fputs(token, cgen->out);
        } else {
            fputs(token, cgen->out);
            _(emitCxxCodeForExpr(expr->left, stmt, cgen, pass));
        }
        break;
    case Spk_EXPR_AND:
    case Spk_EXPR_OR:
    case Spk_EXPR_ID:
    case Spk_EXPR_NI:
        _(emitCxxCodeForExpr(expr->left, stmt, cgen, pass));
        switch (expr->kind) {
        default:
        case Spk_EXPR_AND:  token = "&&";  break;
        case Spk_EXPR_OR:   token = "||";  break;
        case Spk_EXPR_ID:   token = "==";  break;
        case Spk_EXPR_NI:   token = "!=";  break;
        }
        fprintf(cgen->out, " %s ", token);
        _(emitCxxCodeForExpr(expr->right, stmt, cgen, pass));
        break;
    case Spk_EXPR_ASSIGN:
        switch (expr->left->kind) {
        case Spk_EXPR_NAME:
            fputs(SpkHost_SymbolAsCString(expr->left->u.ref.def->sym->sym), cgen->out);
            switch (expr->oper) {
            case Spk_OPER_EQ:      token = "=";    break;
            case Spk_OPER_MUL:     token = "*=";   break;
            case Spk_OPER_DIV:     token = "/=";   break;
            case Spk_OPER_MOD:     token = "%=";   break;
            case Spk_OPER_ADD:     token = "+=";   break;
            case Spk_OPER_SUB:     token = "-=";   break;
            case Spk_OPER_LSHIFT:  token = "<<=";  break;
            case Spk_OPER_RSHIFT:  token = ">>=";  break;
            case Spk_OPER_BAND:    token = "&=";   break;
            case Spk_OPER_BXOR:    token = "^=";   break;
            case Spk_OPER_BOR:     token = "|=";   break;
            }
            fprintf(cgen->out, " %s ", token);
            _(emitCxxCodeForExpr(expr->right, stmt, cgen, pass));
            break;
        default:
            ASSERT(0, "XXX");
            break;
        }
        break;
    case Spk_EXPR_ATTR_VAR:
        ASSERT(0, "XXX");
        break;
    case Spk_EXPR_BINARY:
        _(emitCxxCodeForExpr(expr->left, stmt, cgen, pass));
        switch (expr->oper) {
        case Spk_OPER_MUL:     token = "*";   break;
        case Spk_OPER_DIV:     token = "/";   break;
        case Spk_OPER_MOD:     token = "%";   break;
        case Spk_OPER_ADD:     token = "+";   break;
        case Spk_OPER_SUB:     token = "-";   break;
        case Spk_OPER_LSHIFT:  token = "<<";  break;
        case Spk_OPER_RSHIFT:  token = ">>";  break;
        case Spk_OPER_LT:      token = "<";   break;
        case Spk_OPER_GT:      token = ">";   break;
        case Spk_OPER_LE:      token = "<=";  break;
        case Spk_OPER_GE:      token = ">=";  break;
        case Spk_OPER_EQ:      token = "==";  break;
        case Spk_OPER_NE:      token = "!=";  break;
        case Spk_OPER_BAND:    token = "&";   break;
        case Spk_OPER_BXOR:    token = "^";   break;
        case Spk_OPER_BOR:     token = "|";   break;
        }
        fputs(token, cgen->out);
        _(emitCxxCodeForExpr(expr->right, stmt, cgen, pass));
        break;
    case Spk_EXPR_COND:
        _(emitCxxCodeForExpr(expr->cond, stmt, cgen, pass));
        fputs(" ? ", cgen->out);
        _(emitCxxCodeForExpr(expr->left, stmt, cgen, pass));
        fputs(" : ", cgen->out);
        _(emitCxxCodeForExpr(expr->right, stmt, cgen, pass));
        break;
    }
    
    fputs(")", cgen->out);
        
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static SpkUnknown *emitCxxCodeForStmt(Stmt *stmt, Stmt *outer, CxxCodeGen *cgen,
                                      unsigned int outerPass)
{
    unsigned int varDefPass;
    
    switch (stmt->kind) {
    case Spk_STMT_BREAK:
        if (outerPass == 2) {
            indent(cgen);
            fputs("break;\n", cgen->out);
        }
        break;
    case Spk_STMT_CONTINUE:
        if (outerPass == 2) {
            indent(cgen);
            fputs("continue;\n", cgen->out);
        }
        break;
    case Spk_STMT_COMPOUND:
        if (outerPass == 2) {
            Stmt *s;
            unsigned int innerPass;
            
            indent(cgen);
            fputs("{\n", cgen->out);
            ++cgen->indent;
            for (innerPass = 1; innerPass <= 2; ++innerPass) {
                for (s = stmt->top; s; s = s->next) {
                    _(emitCxxCodeForStmt(s, stmt, cgen, innerPass));
                }
            }
            --cgen->indent;
            indent(cgen);
            fputs("}\n", cgen->out);
        }
        break;
    case Spk_STMT_DEF_VAR:
        switch (outer->kind) {
        default:
        case Spk_STMT_DEF_MODULE: varDefPass = 1; break;
        case Spk_STMT_DEF_CLASS:  varDefPass = 1; break;
        case Spk_STMT_DEF_METHOD: varDefPass = 2; break;
        }
        if (outerPass == varDefPass) {
            indent(cgen);
            _(emitCxxCodeForVarDefList(stmt->expr, stmt, cgen, outerPass));
            fputs(";\n", cgen->out);
        }
        break;
    case Spk_STMT_DEF_METHOD:
        _(emitCxxCodeForMethodDef(stmt, outer, cgen, outerPass));
        break;
    case Spk_STMT_DEF_CLASS:
        break;
    case Spk_STMT_DEF_MODULE:
        ASSERT(0, "unexpected module node");
        break;
    case Spk_STMT_DEF_TYPE:
        ASSERT(0, "unexpected type node");
        break;
    case Spk_STMT_DO_WHILE:
        if (outerPass == 2) {
            indent(cgen);
            fputs("do\n", cgen->out);
            ++cgen->indent;
            _(emitCxxCodeForStmt(stmt->top, stmt, cgen, outerPass));
            --cgen->indent;
            indent(cgen);
            fputs("while (", cgen->out);
            _(emitCxxCodeForExpr(stmt->expr, stmt, cgen, outerPass));
            fputs(");\n", cgen->out);
        }
        break;
    case Spk_STMT_EXPR:
        if (outerPass == 2) {
            indent(cgen);
            if (stmt->expr) {
                _(emitCxxCodeForExpr(stmt->expr, stmt, cgen, outerPass));
            }
            fputs(";\n", cgen->out);
        }
        break;
    case Spk_STMT_FOR:
        if (outerPass == 2) {
            indent(cgen);
            fputs("for (", cgen->out);
            if (stmt->init) {
                _(emitCxxCodeForExpr(stmt->init, stmt, cgen, outerPass));
            }
            fputs("; ", cgen->out);
            if (stmt->expr) {
                _(emitCxxCodeForExpr(stmt->expr, stmt, cgen, outerPass));
            }
            fputs("; ", cgen->out);
            if (stmt->incr) {
                _(emitCxxCodeForExpr(stmt->incr, stmt, cgen, outerPass));
            }
            fputs(")\n", cgen->out);
            ++cgen->indent;
            _(emitCxxCodeForStmt(stmt->top, stmt, cgen, outerPass));
            --cgen->indent;
        }
        break;
    case Spk_STMT_IF_ELSE:
        if (outerPass == 2) {
            indent(cgen);
            fputs("if (", cgen->out);
            _(emitCxxCodeForExpr(stmt->expr, stmt, cgen, outerPass));
            fputs(")\n", cgen->out);
            ++cgen->indent;
            _(emitCxxCodeForStmt(stmt->top, stmt, cgen, outerPass));
            --cgen->indent;
            if (stmt->bottom) {
                indent(cgen);
                fputs("else\n", cgen->out);
                ++cgen->indent;
                _(emitCxxCodeForStmt(stmt->bottom, stmt, cgen, outerPass));
                --cgen->indent;
            }
        }
        break;
    case Spk_STMT_PRAGMA_SOURCE:
        /* XXX */
        break;
    case Spk_STMT_RETURN:
        if (outerPass == 2) {
            indent(cgen);
            fputs("return", cgen->out);
            if (stmt->expr) {
                fputs(" ", cgen->out);
                _(emitCxxCodeForExpr(stmt->expr, stmt, cgen, outerPass));
            }
            fputs(";\n", cgen->out);
        }
        break;
    case Spk_STMT_YIELD:
        break;
    case Spk_STMT_WHILE:
        if (outerPass == 2) {
            indent(cgen);
            fputs("while (", cgen->out);
            _(emitCxxCodeForExpr(stmt->expr, stmt, cgen, outerPass));
            fputs(")\n", cgen->out);
            ++cgen->indent;
            _(emitCxxCodeForStmt(stmt->top, stmt, cgen, outerPass));
            --cgen->indent;
        }
        break;
    }
    
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static SpkUnknown *emitCxxCodeForMethodDef(Stmt *stmt,
                                           Stmt *outer,
                                           CxxCodeGen *cgen,
                                           unsigned int outerPass)
{
    Stmt *body, *s;
    Expr *expr, *arg, *def;
    unsigned int innerPass;
    
    expr = stmt->expr;
    body = stmt->top;
    ASSERT(body->kind == Spk_STMT_COMPOUND,
           "compound statement expected");
    
    indent(cgen);
    switch (expr->kind) {
    case Spk_EXPR_CALL:
        ASSERT(expr->left->kind == Spk_EXPR_NAME, "invalid method declarator");
        switch (outer->kind) {
        default:
            emitDeclSpecs(expr, cgen);
            fprintf(cgen->out, "%s(", SpkHost_SelectorAsCString(stmt->u.method.name->sym));
            break;
        case Spk_STMT_DEF_CLASS:
            switch (outerPass) {
            case 1:
                emitDeclSpecs(expr, cgen);
                fprintf(cgen->out, "%s(", SpkHost_SelectorAsCString(stmt->u.method.name->sym));
                break;
            case 2:
                emitDeclSpecs(expr, cgen);
                fprintf(cgen->out, "%s::%s(",
                        SpkHost_SymbolAsCString(outer->expr->sym->sym),
                        SpkHost_SelectorAsCString(stmt->u.method.name->sym));
                break;
            }
            break;
        }
        for (arg = stmt->u.method.argList.fixed; arg; arg = arg->nextArg) {
            if (arg->kind == Spk_EXPR_ASSIGN) {
                def = arg->left;
            } else {
                def = arg;
            }
            ASSERT(def->kind == Spk_EXPR_NAME, "invalid argument definition");
            emitDeclSpecs(def, cgen);
            fprintf(cgen->out, "%s%s",
                    SpkHost_SelectorAsCString(def->sym->sym),
                    arg->nextArg ? ", " : "");
        }
        fputs(")", cgen->out);
        break;
        
    default:
        ASSERT(0, "invalid method declarator");
        break;
    }
    
    switch (outerPass) {
    case 1:
        fputs(";\n", cgen->out);
        break;
        
    case 2:
        fputs(" {\n", cgen->out);
        ++cgen->indent;
        for (innerPass = 1; innerPass <= 2; ++innerPass) {
            for (arg = stmt->u.method.argList.fixed; arg; arg = arg->nextArg) {
                if (arg->kind == Spk_EXPR_ASSIGN) {
                    ASSERT(0, "XXX: default arguments");
                    _(emitCxxCodeForExpr(arg->right, stmt, cgen, innerPass));
                }
            }
            for (s = body->top; s; s = s->next) {
                _(emitCxxCodeForStmt(s, stmt, cgen, innerPass));
            }
        }
        --cgen->indent;
        indent(cgen);
        fputs("}\n", cgen->out);
        break;
    }
    
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static SpkUnknown *emitCxxCodeForClassBody(Stmt *body, Stmt *stmt, Stmt *outer,
                                           CxxCodeGen *cgen, unsigned int innerPass)
{
    Stmt *s;
    
    /* XXX: public vs. private */
    ASSERT(body->kind == Spk_STMT_COMPOUND,
           "compound statement expected");
    for (s = body->top; s; s = s->next) {
        _(emitCxxCodeForStmt(s, stmt, cgen, innerPass));
    }
    
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static SpkUnknown *emitCxxCodeForClassDef(Stmt *stmt, Stmt *outer, CxxCodeGen *cgen,
                                          unsigned int outerPass)
{
    switch (outerPass) {
    case 0:
        ASSERT(stmt->expr->kind == Spk_EXPR_NAME,
               "identifier expected");
        indent(cgen);
        fprintf(cgen->out, "struct %s;\n", SpkHost_SymbolAsCString(stmt->expr->sym->sym));
        break;
        
    case 1:
        indent(cgen);
        fprintf(cgen->out, "struct %s", SpkHost_SymbolAsCString(stmt->expr->sym->sym));
        if (stmt->u.klass.superclassName) {
            fputs(" : ", cgen->out);
            _(emitCxxCodeForExpr(stmt->u.klass.superclassName, stmt, cgen, outerPass));
        }
        fputs(" {\n", cgen->out);
        ++cgen->indent;
        _(emitCxxCodeForClassBody(stmt->top, stmt, outer, cgen, 1));
        --cgen->indent;
        indent(cgen);
        fputs("};\n", cgen->out);
        if (0 && stmt->bottom) {
            /* XXX: metaclasses */
            _(emitCxxCodeForClassBody(stmt->bottom, stmt, outer, cgen, outerPass));
        }
        break;
        
    case 2:
        _(emitCxxCodeForClassBody(stmt->top, stmt, outer, cgen, 2));
        if (0 && stmt->bottom) {
            /* XXX: metaclasses */
            _(emitCxxCodeForClassBody(stmt->bottom, stmt, outer, cgen, outerPass));
        }
        break;
        
    }
    
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static SpkUnknown *emitCxxCodeForClassTree(Stmt *classDef, Stmt *outer, CxxCodeGen *cgen,
                                           unsigned int outerPass) {
    /* create class with subclasses in preorder */
    Stmt *subclassDef;
    
    _(emitCxxCodeForClassDef(classDef, outer, cgen, outerPass));
    for (subclassDef = classDef->u.klass.firstSubclassDef;
         subclassDef;
         subclassDef = subclassDef->u.klass.nextSubclassDef) {
        _(emitCxxCodeForClassTree(subclassDef, outer, cgen, outerPass));
    }
    
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    return 0;
}


SpkUnknown *SpkCxxCodeGen_GenerateCode(Stmt *tree, FILE *out)
{
    CxxCodeGen cgen;
    unsigned int pass;
    Stmt *s;
    
    ASSERT(tree->kind == Spk_STMT_DEF_MODULE, "module node expected");
    ASSERT(tree->top->kind == Spk_STMT_COMPOUND, "compound statement expected");
    
    cgen.out = out;
    cgen.indent = 0;
    
    for (pass = 0; pass <= 2; ++pass) {
        for (s = tree->u.module.rootClassList.first; s; s = s->u.klass.nextRootClassDef) {
            _(emitCxxCodeForClassTree(s, tree, &cgen, pass));
        }
        if (pass > 0) {
            for (s = tree->top->top; s; s = s->next) {
                _(emitCxxCodeForStmt(s, tree, &cgen, pass));
            }
        }
    }
    
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    return 0;
}
