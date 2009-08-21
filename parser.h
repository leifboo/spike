
#ifndef __spk_parser_h__
#define __spk_parser_h__


#include "tree.h"
#include <stdio.h>


typedef struct SpkParser {
    SpkObject base;
    SpkStmt *root;
    int error;
    struct SpkSymbolTable *st;
} SpkParser;

typedef struct SpkParserSubclass {
    SpkParser base;
    SpkUnknown *variables[1]; /* stretchy */
} SpkParserSubclass;


SpkStmt *SpkParser_NewClassDef(struct SpkSymbolNode *, struct SpkSymbolNode *, SpkStmt *, SpkStmt *,
                               struct SpkSymbolTable *);
SpkExpr *SpkParser_NewClassAttrExpr(struct SpkSymbolNode *, struct SpkSymbolNode *);
SpkExpr *SpkParser_NewExpr(SpkExprKind, SpkOper, SpkExpr *, SpkExpr *, SpkExpr *);
SpkExpr *SpkParser_NewBlock(SpkStmt *, SpkExpr *);
SpkExpr *SpkParser_NewKeywordExpr(struct SpkSymbolNode *, SpkExpr *);
SpkExpr *SpkParser_AppendKeyword(SpkExpr *, struct SpkSymbolNode *, SpkExpr *);
SpkExpr *SpkParser_FreezeKeywords(SpkExpr *, struct SpkSymbolNode *);
SpkStmt *SpkParser_NewStmt(SpkStmtKind, SpkExpr *, SpkStmt *, SpkStmt *);
SpkStmt *SpkParser_NewForStmt(SpkExpr *, SpkExpr *, SpkExpr *, SpkStmt *);
SpkStmt *SpkParser_NewModuleDef(SpkStmt *);
SpkStmt *SpkParser_ParseFile(FILE *, struct SpkSymbolTable *);


extern struct SpkBehavior *Spk_ClassParser;
extern struct SpkClassTmpl Spk_ClassParserTmpl;


#endif /* __spk_parser_h__ */
