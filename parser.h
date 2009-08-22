
#ifndef __spk_parser_h__
#define __spk_parser_h__


#include "tree.h"
#include <stdio.h>


struct SpkSymbolNode;
struct SpkToken;


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


SpkStmt *SpkParser_NewClassDef(struct SpkToken *, struct SpkToken *, SpkStmt *, SpkStmt *,
                               struct SpkSymbolTable *);
SpkExpr *SpkParser_NewAttrExpr(SpkExpr *, struct SpkToken *, struct SpkToken *,
                               struct SpkSymbolTable *);
SpkExpr *SpkParser_NewClassAttrExpr(struct SpkToken *, struct SpkToken *, struct SpkToken *,
                                    struct SpkSymbolTable *);
SpkExpr *SpkParser_NewExpr(SpkExprKind, SpkOper, SpkExpr *, SpkExpr *, SpkExpr *, struct SpkToken *);
SpkExpr *SpkParser_NewBlock(SpkStmt *, SpkExpr *, struct SpkToken *);
SpkExpr *SpkParser_NewKeywordExpr(struct SpkToken *, SpkExpr *, struct SpkSymbolTable *);
SpkExpr *SpkParser_AppendKeyword(SpkExpr *, struct SpkToken *, SpkExpr *, struct SpkSymbolTable *);
SpkExpr *SpkParser_FreezeKeywords(SpkExpr *, struct SpkToken *, struct SpkSymbolTable *);
SpkStmt *SpkParser_NewStmt(SpkStmtKind, SpkExpr *, SpkStmt *, SpkStmt *);
SpkStmt *SpkParser_NewForStmt(SpkExpr *, SpkExpr *, SpkExpr *, SpkStmt *);
SpkStmt *SpkParser_NewModuleDef(SpkStmt *);
SpkStmt *SpkParser_ParseFile(FILE *, struct SpkSymbolTable *);
void SpkParser_Source(SpkStmt **, SpkUnknown *);


extern struct SpkBehavior *Spk_ClassParser;
extern struct SpkClassTmpl Spk_ClassParserTmpl;


#endif /* __spk_parser_h__ */
