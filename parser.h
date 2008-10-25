
#ifndef __parser_h__
#define __parser_h__


#include "tree.h"


typedef struct ParserState {
    Stmt *root;
    int error;
} ParserState;


Stmt *SpkParser_NewClassDef(struct SymbolNode *name, struct SymbolNode *super, Stmt *stmt);
Expr *SpkParser_NewClassAttrExpr(struct SymbolNode *className, struct SymbolNode *attrName);
Expr *SpkParser_NewExpr(ExprKind, Oper oper, Expr *, Expr *, Expr *);
Stmt *SpkParser_NewStmt(StmtKind, Expr *, Stmt *, Stmt *);
Stmt *SpkParser_ParseFile(const char *filename);


#endif /* __parser_h__ */
