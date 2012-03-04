
#ifndef __parser_h__
#define __parser_h__


#include "lexer.h"
#include "tree.h"

#include <stdio.h>


struct SymbolNode;


typedef struct Parser {
    Object base;
    Stmt *root;
    int error;
    Unknown *st;
    Unknown *tb;
} Parser;


Stmt *Parser_NewClassDef(struct Token *, struct Token *, Stmt *, Stmt *,
                               Parser *);
Expr *Parser_NewAttrExpr(Expr *, struct Token *, struct Token *,
                               Parser *);
Expr *Parser_NewClassAttrExpr(struct Token *, struct Token *, struct Token *,
                                    Parser *);
Expr *Parser_NewExpr(ExprKind, Oper, Expr *, Expr *, Expr *, struct Token *,
                           Parser *);
Expr *Parser_NewCallExpr(Oper, Expr *, ArgList *, Token *, Parser *);
Expr *Parser_NewBlock(Expr *, Stmt *, Expr *, struct Token *, Parser *);
Expr *Parser_NewKeywordExpr(struct Token *, Expr *, Parser *);
Expr *Parser_AppendKeyword(Expr *, struct Token *, Expr *, Parser *);
Expr *Parser_FreezeKeywords(Expr *, struct Token *, Parser *);
Expr *Parser_NewCompoundExpr(Expr *, struct Token *, Parser *);
Stmt *Parser_NewStmt(StmtKind, Expr *, Stmt *, Stmt *, Parser *);
Stmt *Parser_NewForStmt(Expr *, Expr *, Expr *, Stmt *, Parser *);

void Parser_SetNextExpr(Expr *, Expr *, Parser *);
void Parser_SetLeftExpr(Expr *, Expr *, Parser *);
void Parser_SetNextArg(Expr *, Expr *, Parser *);
void Parser_SetDeclSpecs(Expr *, Expr *, Parser *);
void Parser_SetNextStmt(Stmt *, Stmt *, Parser *);
void Parser_Concat(Expr *, Token *, Parser *);

Stmt *Parser_NewModuleDef(Stmt *);
Stmt *Parser_ParseFileStream(FILE *, struct SymbolTable *);
Stmt *Parser_ParseString(const char *, struct SymbolTable *);
void Parser_Source(Stmt **, Unknown *);

void Parser_Parse(void *yyp, int yymajor, struct Token yyminor, Parser *);
void *Parser_ParseAlloc(void *(*mallocProc)(size_t));
void Parser_ParseFree(void *p, void (*freeProc)(void*));
void Parser_ParseTrace(FILE *TraceFILE, char *zTracePrompt);


extern struct ClassTmpl ClassParserTmpl;


#endif /* __parser_h__ */
