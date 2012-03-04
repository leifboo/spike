
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
do if (!(c)) { Halt(HALT_ASSERTION_ERROR, (msg)); goto unwind; } \
while (0)

#define _(c) do { \
Unknown *_tmp = (c); \
if (!_tmp) goto unwind; \
DECREF(_tmp); } while (0)


typedef struct CxxCodeGen {
    FILE *out;
    int indent;
    Unknown *source;
    unsigned int currentLineNo;
} CxxCodeGen;


static Unknown *emitCxxCodeForStmt(Stmt *, Stmt *, CxxCodeGen *, unsigned int);
static Unknown *emitCxxCodeForExpr(Expr *, Stmt *, CxxCodeGen *, unsigned int);
static Unknown *emitCxxCodeForOneExpr(Expr *, Stmt *, CxxCodeGen *, unsigned int);
static Unknown *emitCxxCodeForMethodDef(Stmt *, Stmt *, CxxCodeGen *, unsigned int);

static void indent(CxxCodeGen *cgen) {
    int i;
    
    for (i = 0; i < cgen->indent; ++i)
        fputs("    ", cgen->out);
}

static void exprLine(Expr *expr, CxxCodeGen *cgen) {
    if (cgen->source &&
        expr->lineNo != cgen->currentLineNo)
    {
        if (expr->lineNo > cgen->currentLineNo &&
            expr->lineNo - cgen->currentLineNo <= 3) {
            for ( ; cgen->currentLineNo < expr->lineNo; ++cgen->currentLineNo)
                fprintf(cgen->out, "\n");
        } else {
            fprintf(cgen->out,
                    "\n"
                    "#line %u \"%s\"\n",
                    expr->lineNo, Host_StringAsCString(cgen->source));
            cgen->currentLineNo = expr->lineNo;
        }
        indent(cgen);
    }
}

static void stmtLine(Stmt *stmt, CxxCodeGen *cgen) {
    Expr *expr = 0;
    
    switch (stmt->kind) {
    case STMT_DO_WHILE:
        indent(cgen);
    case STMT_COMPOUND:
    case STMT_DEF_CLASS:
        return;
    }
    
    /**/ if (stmt->init) expr = stmt->init;
    else if (stmt->expr) expr = stmt->expr;
    else if (stmt->incr) expr = stmt->incr;
    
    if (expr) exprLine(expr, cgen);
    else indent(cgen);
}

static int emitDeclSpecs(Expr *def, CxxCodeGen *cgen) {
    Expr *declSpec;
    int ptr;
    
    ptr = 0;
    for (declSpec = def->declSpecs; declSpec; declSpec = declSpec->next) {
        fprintf(cgen->out, "%s ", Host_SymbolAsCString(declSpec->sym->sym));
        ptr = ptr ||
              (declSpec->u.ref.def &&
               declSpec->u.ref.def->kind == EXPR_NAME &&
               declSpec->u.ref.def->u.def.stmt &&
               declSpec->u.ref.def->u.def.stmt->kind == STMT_DEF_CLASS);
    }
    return ptr;
}

static Unknown *emitCxxCodeForVarDefList(Expr *defList,
                                            Stmt *stmt,
                                            CxxCodeGen *cgen,
                                            unsigned int pass)
{
    Expr *expr, *def;
    int objPtr, ptr, i;
    
    objPtr = emitDeclSpecs(defList, cgen);
    for (expr = defList; expr; expr = expr->next) {
        if (expr->kind == EXPR_ASSIGN) {
            def = expr->left;
            ASSERT(0, "XXX: initializers");
            _(emitCxxCodeForExpr(expr->right, stmt, cgen, pass));
        } else {
            def = expr;
        }
        ptr = objPtr; /* XXX: "obj *" is illegal, right? */
        while (def->kind == EXPR_UNARY && def->oper == OPER_IND) {
            def = def->left;
            ++ptr;
        }
        ASSERT(def->kind == EXPR_NAME, "invalid variable definition");
        for (i = 0; i < ptr; ++i)
            fputs("*", cgen->out);
        fprintf(cgen->out, "%s%s",
                Host_SymbolAsCString(def->sym->sym),
                expr->next ? ", " : "");
    }
    INCREF(GLOBAL(xvoid));
    return GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static Unknown *emitCxxCodeForExpr(Expr *expr, Stmt *stmt, CxxCodeGen *cgen,
                                      unsigned int pass)
{
    for ( ; expr; expr = expr->next) {
        _(emitCxxCodeForOneExpr(expr, stmt, cgen, pass));
    }
    INCREF(GLOBAL(xvoid));
    return GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static Unknown *emitCxxCodeForBlock(Expr *expr, Stmt *stmt, CxxCodeGen *cgen,
                                       unsigned int outerPass)
{
    Expr *arg;
    Stmt *firstStmt, *s;
    unsigned int innerPass;
    
    if (outerPass != 2) {
        goto leave;
    }
    
    firstStmt = expr->aux.block.stmtList;
    
    if (firstStmt && firstStmt->kind == STMT_DEF_VAR) {
        /* block arguments */
        for (arg = firstStmt->expr; arg; arg = arg->next) {
            ASSERT(arg->kind == EXPR_NAME, "invalid argument definition");
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
    INCREF(GLOBAL(xvoid));
    return GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static Unknown *emitCxxCodeForOneExpr(Expr *expr, Stmt *stmt, CxxCodeGen *cgen,
                                         unsigned int pass)
{
    Expr *arg;
    const char *token;
    
    exprLine(expr, cgen);
    
    /* account for precedence */
    fputs("(", cgen->out);
    
    switch (expr->kind) {
    case EXPR_LITERAL:
        /* XXX: kludge */
        Host_PrintObject(expr->aux.literalValue, cgen->out);
        break;
    case EXPR_NAME:
        if (expr->u.ref.def->u.def.pushOpcode == OPCODE_PUSH_SELF)
            fputs("this", cgen->out);
        else
            fputs(Host_SelectorAsCString(expr->u.ref.def->sym->sym), cgen->out);
        break;
    case EXPR_BLOCK:
        _(emitCxxCodeForBlock(expr, stmt, cgen, pass));
        break;
    case EXPR_COMPOUND:
        for (arg = expr->right; arg; arg = arg->nextArg) {
            _(emitCxxCodeForExpr(arg, stmt, cgen, pass));
        }
        if (expr->var) {
            _(emitCxxCodeForExpr(expr->var, stmt, cgen, pass));
        }
        break;
    case EXPR_CALL:
    case EXPR_KEYWORD:
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
    case EXPR_ATTR:
        _(emitCxxCodeForExpr(expr->left, stmt, cgen, pass));
        fputs("->", cgen->out);
        fputs(Host_SelectorAsCString(expr->sym->sym), cgen->out);
        break;
    case EXPR_POSTOP:
    case EXPR_PREOP:
    case EXPR_UNARY:
        switch (expr->oper) {
        case OPER_SUCC:  token = "++";  break;
        case OPER_PRED:  token = "--";  break;
        case OPER_ADDR:  token = "&";   break;
        case OPER_IND:   token = "*";   break;
        case OPER_POS:   token = "+";   break;
        case OPER_NEG:   token = "-";   break;
        case OPER_BNEG:  token = "~";   break;
        case OPER_LNEG:  token = "!";   break;
        }
        if (expr->kind == EXPR_POSTOP) {
            _(emitCxxCodeForExpr(expr->left, stmt, cgen, pass));
            fputs(token, cgen->out);
        } else {
            fputs(token, cgen->out);
            _(emitCxxCodeForExpr(expr->left, stmt, cgen, pass));
        }
        break;
    case EXPR_AND:
    case EXPR_OR:
    case EXPR_ID:
    case EXPR_NI:
        _(emitCxxCodeForExpr(expr->left, stmt, cgen, pass));
        switch (expr->kind) {
        default:
        case EXPR_AND:  token = "&&";  break;
        case EXPR_OR:   token = "||";  break;
        case EXPR_ID:   token = "==";  break;
        case EXPR_NI:   token = "!=";  break;
        }
        fprintf(cgen->out, " %s ", token);
        _(emitCxxCodeForExpr(expr->right, stmt, cgen, pass));
        break;
    case EXPR_ASSIGN:
        switch (expr->left->kind) {
        case EXPR_NAME:
            fputs(Host_SymbolAsCString(expr->left->u.ref.def->sym->sym), cgen->out);
            switch (expr->oper) {
            case OPER_EQ:      token = "=";    break;
            case OPER_MUL:     token = "*=";   break;
            case OPER_DIV:     token = "/=";   break;
            case OPER_MOD:     token = "%=";   break;
            case OPER_ADD:     token = "+=";   break;
            case OPER_SUB:     token = "-=";   break;
            case OPER_LSHIFT:  token = "<<=";  break;
            case OPER_RSHIFT:  token = ">>=";  break;
            case OPER_BAND:    token = "&=";   break;
            case OPER_BXOR:    token = "^=";   break;
            case OPER_BOR:     token = "|=";   break;
            }
            fprintf(cgen->out, " %s ", token);
            _(emitCxxCodeForExpr(expr->right, stmt, cgen, pass));
            break;
        default:
            ASSERT(0, "XXX");
            break;
        }
        break;
    case EXPR_ATTR_VAR:
        ASSERT(0, "XXX");
        break;
    case EXPR_BINARY:
        _(emitCxxCodeForExpr(expr->left, stmt, cgen, pass));
        switch (expr->oper) {
        case OPER_MUL:     token = "*";   break;
        case OPER_DIV:     token = "/";   break;
        case OPER_MOD:     token = "%";   break;
        case OPER_ADD:     token = "+";   break;
        case OPER_SUB:     token = "-";   break;
        case OPER_LSHIFT:  token = "<<";  break;
        case OPER_RSHIFT:  token = ">>";  break;
        case OPER_LT:      token = "<";   break;
        case OPER_GT:      token = ">";   break;
        case OPER_LE:      token = "<=";  break;
        case OPER_GE:      token = ">=";  break;
        case OPER_EQ:      token = "==";  break;
        case OPER_NE:      token = "!=";  break;
        case OPER_BAND:    token = "&";   break;
        case OPER_BXOR:    token = "^";   break;
        case OPER_BOR:     token = "|";   break;
        }
        fputs(token, cgen->out);
        _(emitCxxCodeForExpr(expr->right, stmt, cgen, pass));
        break;
    case EXPR_COND:
        _(emitCxxCodeForExpr(expr->cond, stmt, cgen, pass));
        fputs(" ? ", cgen->out);
        _(emitCxxCodeForExpr(expr->left, stmt, cgen, pass));
        fputs(" : ", cgen->out);
        _(emitCxxCodeForExpr(expr->right, stmt, cgen, pass));
        break;
    }
    
    fputs(")", cgen->out);
        
    INCREF(GLOBAL(xvoid));
    return GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static Unknown *emitCxxCodeForStmt(Stmt *stmt, Stmt *outer, CxxCodeGen *cgen,
                                      unsigned int outerPass)
{
    unsigned int varDefPass;
    
    if (outerPass == 2)
        stmtLine(stmt, cgen);
    
    switch (stmt->kind) {
    case STMT_BREAK:
        if (outerPass == 2) {
            fputs("break;", cgen->out);
        }
        break;
    case STMT_CONTINUE:
        if (outerPass == 2) {
            fputs("continue;", cgen->out);
        }
        break;
    case STMT_COMPOUND:
        if (outerPass == 2) {
            Stmt *s;
            unsigned int innerPass;
            
            fputs("{", cgen->out);
            ++cgen->indent;
            for (innerPass = 1; innerPass <= 2; ++innerPass) {
                for (s = stmt->top; s; s = s->next) {
                    _(emitCxxCodeForStmt(s, stmt, cgen, innerPass));
                }
            }
            --cgen->indent;
            fputs("}", cgen->out);
        }
        break;
    case STMT_DEF_VAR:
        switch (outer->kind) {
        default:
        case STMT_DEF_MODULE: varDefPass = 1; break;
        case STMT_DEF_CLASS:  varDefPass = 1; break;
        case STMT_DEF_METHOD: varDefPass = 2; break;
        }
        if (outerPass == varDefPass) {
            if (outerPass != 2)
                indent(cgen);
            _(emitCxxCodeForVarDefList(stmt->expr, stmt, cgen, outerPass));
            fputs(";", cgen->out);
        }
        break;
    case STMT_DEF_METHOD:
        _(emitCxxCodeForMethodDef(stmt, outer, cgen, outerPass));
        break;
    case STMT_DEF_CLASS:
        break;
    case STMT_DEF_MODULE:
        ASSERT(0, "unexpected module node");
        break;
    case STMT_DEF_SPEC:
        ASSERT(0, "unexpected spec node");
        break;
    case STMT_DO_WHILE:
        if (outerPass == 2) {
            fputs("do ", cgen->out);
            ++cgen->indent;
            _(emitCxxCodeForStmt(stmt->top, stmt, cgen, outerPass));
            --cgen->indent;
            indent(cgen);
            fputs("while (", cgen->out);
            _(emitCxxCodeForExpr(stmt->expr, stmt, cgen, outerPass));
            fputs(");", cgen->out);
        }
        break;
    case STMT_EXPR:
        if (outerPass == 2) {
            if (stmt->expr) {
                _(emitCxxCodeForExpr(stmt->expr, stmt, cgen, outerPass));
            }
            fputs(";", cgen->out);
        }
        break;
    case STMT_FOR:
        if (outerPass == 2) {
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
            fputs(")", cgen->out);
            ++cgen->indent;
            _(emitCxxCodeForStmt(stmt->top, stmt, cgen, outerPass));
            --cgen->indent;
        }
        break;
    case STMT_IF_ELSE:
        if (outerPass == 2) {
            fputs("if (", cgen->out);
            _(emitCxxCodeForExpr(stmt->expr, stmt, cgen, outerPass));
            fputs(")", cgen->out);
            ++cgen->indent;
            _(emitCxxCodeForStmt(stmt->top, stmt, cgen, outerPass));
            --cgen->indent;
            if (stmt->bottom) {
                indent(cgen);
                fputs("else ", cgen->out);
                ++cgen->indent;
                _(emitCxxCodeForStmt(stmt->bottom, stmt, cgen, outerPass));
                --cgen->indent;
            }
        }
        break;
    case STMT_PRAGMA_SOURCE:
        INCREF(stmt->u.source);
        XDECREF(cgen->source);
        cgen->source = stmt->u.source;
        cgen->currentLineNo = 0;
        break;
    case STMT_RETURN:
        if (outerPass == 2) {
            fputs("return", cgen->out);
            if (stmt->expr) {
                fputs(" ", cgen->out);
                _(emitCxxCodeForExpr(stmt->expr, stmt, cgen, outerPass));
            }
            fputs(";", cgen->out);
        }
        break;
    case STMT_YIELD:
        break;
    case STMT_WHILE:
        if (outerPass == 2) {
            fputs("while (", cgen->out);
            _(emitCxxCodeForExpr(stmt->expr, stmt, cgen, outerPass));
            fputs(")", cgen->out);
            ++cgen->indent;
            _(emitCxxCodeForStmt(stmt->top, stmt, cgen, outerPass));
            --cgen->indent;
        }
        break;
    }
    
    INCREF(GLOBAL(xvoid));
    return GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static Unknown *emitCxxCodeForMethodDef(Stmt *stmt,
                                           Stmt *outer,
                                           CxxCodeGen *cgen,
                                           unsigned int outerPass)
{
    Stmt *body, *s;
    Expr *expr, *arg, *def;
    unsigned int innerPass;
    int objPtr, ptr, i;
    
    expr = stmt->expr;
    body = stmt->top;
    ASSERT(body->kind == STMT_COMPOUND,
           "compound statement expected");
    
    indent(cgen);
    objPtr = emitDeclSpecs(expr, cgen);
    ptr = objPtr; /* XXX: "obj *" is illegal, right? */
    while (expr->kind == EXPR_UNARY && expr->oper == OPER_IND) {
        expr = expr->left;
        ++ptr;
    }
    for (i = 0; i < ptr; ++i)
        fputs("*", cgen->out);
    switch (expr->kind) {
    case EXPR_CALL:
        ASSERT(expr->left->kind == EXPR_NAME, "invalid method declarator");
        switch (outer->kind) {
        default:
            fprintf(cgen->out, "%s(", Host_SelectorAsCString(stmt->u.method.name->sym));
            break;
        case STMT_DEF_CLASS:
            switch (outerPass) {
            case 1:
                fprintf(cgen->out, "%s(", Host_SelectorAsCString(stmt->u.method.name->sym));
                break;
            case 2:
                fprintf(cgen->out, "%s::%s(",
                        Host_SymbolAsCString(outer->expr->sym->sym),
                        Host_SelectorAsCString(stmt->u.method.name->sym));
                break;
            }
            break;
        }
        for (arg = stmt->u.method.argList.fixed; arg; arg = arg->nextArg) {
            objPtr = emitDeclSpecs(arg, cgen);
            if (arg->kind == EXPR_ASSIGN) {
                def = arg->left;
            } else {
                def = arg;
            }
            ptr = objPtr; /* XXX: "obj *" is illegal, right? */
            while (def->kind == EXPR_UNARY && def->oper == OPER_IND) {
                def = def->left;
                ++ptr;
            }
            ASSERT(def->kind == EXPR_NAME, "invalid argument definition");
            for (i = 0; i < ptr; ++i)
                fputs("*", cgen->out);
            fprintf(cgen->out, "%s%s",
                    Host_SelectorAsCString(def->sym->sym),
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
        fputs(" {", cgen->out);
        ++cgen->indent;
        for (innerPass = 1; innerPass <= 2; ++innerPass) {
            for (arg = stmt->u.method.argList.fixed; arg; arg = arg->nextArg) {
                if (arg->kind == EXPR_ASSIGN) {
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
        fputs("}", cgen->out);
        break;
    }
    
    INCREF(GLOBAL(xvoid));
    return GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static Unknown *emitCxxCodeForClassBody(Stmt *body, Stmt *stmt, Stmt *outer,
                                           CxxCodeGen *cgen, unsigned int innerPass)
{
    Stmt *s;
    
    /* XXX: public vs. private */
    ASSERT(body->kind == STMT_COMPOUND,
           "compound statement expected");
    for (s = body->top; s; s = s->next) {
        _(emitCxxCodeForStmt(s, stmt, cgen, innerPass));
    }
    
    INCREF(GLOBAL(xvoid));
    return GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static Unknown *emitCxxCodeForClassDef(Stmt *stmt, Stmt *outer, CxxCodeGen *cgen,
                                          unsigned int outerPass)
{
    switch (outerPass) {
    case 0:
        ASSERT(stmt->expr->kind == EXPR_NAME,
               "identifier expected");
        indent(cgen);
        fprintf(cgen->out, "struct %s;\n", Host_SymbolAsCString(stmt->expr->sym->sym));
        break;
        
    case 1:
        indent(cgen);
        fprintf(cgen->out, "struct %s", Host_SymbolAsCString(stmt->expr->sym->sym));
        if (stmt->u.klass.superclassName) {
            Expr *sc = stmt->u.klass.superclassName;
            fputs(" : ", cgen->out);
            ASSERT(sc->kind == EXPR_NAME, "identifier expected");
            fputs(Host_SelectorAsCString(sc->u.ref.def->sym->sym), cgen->out);
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
    
    INCREF(GLOBAL(xvoid));
    return GLOBAL(xvoid);
    
 unwind:
    return 0;
}

static Unknown *emitCxxCodeForClassTree(Stmt *classDef, Stmt *outer, CxxCodeGen *cgen,
                                           unsigned int outerPass) {
    /* create class with subclasses in preorder */
    Stmt *subclassDef;
    
    _(emitCxxCodeForClassDef(classDef, outer, cgen, outerPass));
    for (subclassDef = classDef->u.klass.firstSubclassDef;
         subclassDef;
         subclassDef = subclassDef->u.klass.nextSubclassDef) {
        _(emitCxxCodeForClassTree(subclassDef, outer, cgen, outerPass));
    }
    
    INCREF(GLOBAL(xvoid));
    return GLOBAL(xvoid);
    
 unwind:
    return 0;
}


Unknown *CxxCodeGen_GenerateCode(Stmt *tree, FILE *out)
{
    CxxCodeGen cgen;
    unsigned int pass;
    Stmt *s;
    
    ASSERT(tree->kind == STMT_DEF_MODULE, "module node expected");
    ASSERT(tree->top->kind == STMT_COMPOUND, "compound statement expected");
    
    cgen.out = out;
    cgen.indent = 0;
    cgen.source = 0;
    cgen.currentLineNo = 0;
    
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
    
    INCREF(GLOBAL(xvoid));
    return GLOBAL(xvoid);
    
 unwind:
    return 0;
}
