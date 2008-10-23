
#include "parser.h"

#include "lexer.h"
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
    if (super) {
        newStmt->u.klass.super = SpkParser_NewExpr(EXPR_NAME, 0, 0, 0, 0);
        newStmt->u.klass.super->sym = super;
    }
    return newStmt;
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
    SpkParser_Parse(parser, 0, token, &parserState);
    SpkParser_ParseFree(parser, &free);
    
    SpkLexer_lex_destroy(lexer);
    fclose(yyin);
    
    return parserState.error ? 0 : parserState.root;
}
