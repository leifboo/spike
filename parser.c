
#include "parser.h"

#include "lexer.h"
#include "st.h"
#include "sym.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void SpkParser_Parse(void *yyp, int yymajor, Token yyminor, ParserState *);
void *SpkParser_ParseAlloc(void *(*mallocProc)(size_t));
void SpkParser_ParseFree(void *p, void (*freeProc)(void*));

/****************************************************************************/
/* grammar actions */

Stmt *SpkParser_NewClassDef(struct SymbolNode *name, struct SymbolNode *super, Stmt *stmt) {
    Stmt *newStmt;
    
    newStmt = (Stmt *)malloc(sizeof(Stmt));
    memset(newStmt, 0, sizeof(Stmt));
    newStmt->kind = STMT_DEF_CLASS;
    newStmt->top = stmt;
    newStmt->expr = SpkParser_NewExpr(EXPR_NAME, 0, 0, 0, 0); newStmt->expr->sym = name;
    if (!super)
        super = SpkSymbolNode_Get(SpkSymbol_get("Object"));
    newStmt->u.klass.superclassName = SpkParser_NewExpr(EXPR_NAME, 0, 0, 0, 0);
    newStmt->u.klass.superclassName->sym = super;
    return newStmt;
}

Expr *SpkParser_NewClassAttrExpr(struct SymbolNode *className, struct SymbolNode *attrName) {
    Expr *obj, *newExpr;
    
    obj = SpkParser_NewExpr(EXPR_NAME, 0, 0, 0, 0);
    obj->sym = className;
    newExpr = SpkParser_NewExpr(EXPR_ATTR, 0, 0, obj, 0);
    newExpr->sym = attrName;
    return newExpr;
}

Stmt *SpkParser_NewStmt(StmtKind kind, Expr *expr, Stmt *top, Stmt *bottom) {
    Stmt *newStmt;
    
    newStmt = (Stmt *)malloc(sizeof(Stmt));
    memset(newStmt, 0, sizeof(Stmt));
    newStmt->kind = kind;
    newStmt->top = top;
    newStmt->bottom = bottom;
    newStmt->expr = expr;
    return newStmt;
}

Stmt *SpkParser_NewForStmt(Expr *expr1, Expr *expr2, Expr *expr3, Stmt *body) {
    Stmt *newStmt;
    
    newStmt = (Stmt *)malloc(sizeof(Stmt));
    memset(newStmt, 0, sizeof(Stmt));
    newStmt->kind = STMT_FOR;
    newStmt->top = body;
    newStmt->init = expr1;
    newStmt->expr = expr2;
    newStmt->incr = expr3;
    return newStmt;
}

Expr *SpkParser_NewExpr(ExprKind kind, Oper oper, Expr *cond,
                        Expr *left, Expr *right) {
    Expr *newExpr;
    
    newExpr = (Expr *)malloc(sizeof(Expr));
    memset(newExpr, 0, sizeof(Expr));
    newExpr->kind = kind;
    newExpr->oper = oper;
    newExpr->cond = cond;
    newExpr->left = left;
    newExpr->right = right;
    return newExpr;
}

/****************************************************************************/

Stmt *SpkParser_ParseFile(const char *filename) {
    yyscan_t lexer;
    void *parser;
    Token token;
    ParserState parserState;
    FILE *yyin;
    char buffer[1024];
    
    yyin = fopen(filename, "r");
    if (!yyin) {
        fprintf(stderr, "cannot open '%s'\n", filename);
        return 0;
    }
    SpkLexer_lex_init(&lexer);
    SpkLexer_restart(yyin, lexer);
    fgets(buffer, sizeof(buffer), yyin); /* skip #! line */
    SpkLexer_set_lineno(2, lexer);
    
    parserState.root = 0;
    parserState.error = 0;
    
    parser = SpkParser_ParseAlloc(&malloc);
    while (SpkLexer_GetNextToken(&token, lexer)) {
        SpkParser_Parse(parser, token.id, token, &parserState);
    }
    if (token.id == -1) {
        SpkParser_ParseFree(parser, &free);
        SpkLexer_lex_destroy(lexer);
        fclose(yyin);
        return 0;
    }
    SpkParser_Parse(parser, 0, token, &parserState);
    SpkParser_ParseFree(parser, &free);
    
    SpkLexer_lex_destroy(lexer);
    fclose(yyin);
    
    return parserState.error ? 0 : parserState.root;
}
