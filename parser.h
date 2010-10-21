
#ifndef __spk_parser_h__
#define __spk_parser_h__


#include "lexer.h"
#include "tree.h"

#include <stdio.h>


struct SpkSymbolNode;


typedef struct SpkParser {
    SpkObject base;
    SpkStmt *root;
    int error;
    SpkUnknown *st;
    SpkUnknown *tb;
} SpkParser;


SpkStmt *SpkParser_NewClassDef(struct SpkToken *, struct SpkToken *, SpkStmt *, SpkStmt *,
                               SpkParser *);
SpkExpr *SpkParser_NewAttrExpr(SpkExpr *, struct SpkToken *, struct SpkToken *,
                               SpkParser *);
SpkExpr *SpkParser_NewClassAttrExpr(struct SpkToken *, struct SpkToken *, struct SpkToken *,
                                    SpkParser *);
SpkExpr *SpkParser_NewExpr(SpkExprKind, SpkOper, SpkExpr *, SpkExpr *, SpkExpr *, struct SpkToken *,
                           SpkParser *);
SpkExpr *SpkParser_NewCallExpr(SpkOper, SpkExpr *, SpkArgList *, SpkToken *, SpkParser *);
SpkExpr *SpkParser_NewBlock(SpkStmt *, SpkExpr *, struct SpkToken *, SpkParser *);
SpkExpr *SpkParser_NewKeywordExpr(struct SpkToken *, SpkExpr *, SpkParser *);
SpkExpr *SpkParser_AppendKeyword(SpkExpr *, struct SpkToken *, SpkExpr *, SpkParser *);
SpkExpr *SpkParser_FreezeKeywords(SpkExpr *, struct SpkToken *, SpkParser *);
SpkExpr *SpkParser_NewCompoundExpr(SpkExpr *, struct SpkToken *, SpkParser *);
SpkStmt *SpkParser_NewStmt(SpkStmtKind, SpkExpr *, SpkStmt *, SpkStmt *, SpkParser *);
SpkStmt *SpkParser_NewForStmt(SpkExpr *, SpkExpr *, SpkExpr *, SpkStmt *, SpkParser *);

void SpkParser_SetNextExpr(SpkExpr *, SpkExpr *, SpkParser *);
void SpkParser_SetLeftExpr(SpkExpr *, SpkExpr *, SpkParser *);
void SpkParser_SetNextArg(SpkExpr *, SpkExpr *, SpkParser *);
void SpkParser_SetDeclSpecs(SpkExpr *, SpkExpr *, SpkParser *);
void SpkParser_SetNextStmt(SpkStmt *, SpkStmt *, SpkParser *);
void SpkParser_Concat(SpkExpr *, SpkToken *, SpkParser *);

SpkStmt *SpkParser_NewModuleDef(SpkStmt *);
SpkStmt *SpkParser_ParseFileStream(FILE *, struct SpkSymbolTable *);
SpkStmt *SpkParser_ParseString(const char *, struct SpkSymbolTable *);
void SpkParser_Source(SpkStmt **, SpkUnknown *);

void SpkParser_Parse(void *yyp, int yymajor, struct SpkToken yyminor, SpkParser *);
void *SpkParser_ParseAlloc(void *(*mallocProc)(size_t));
void SpkParser_ParseFree(void *p, void (*freeProc)(void*));
void SpkParser_ParseTrace(FILE *TraceFILE, char *zTracePrompt);


extern struct SpkClassTmpl Spk_ClassParserTmpl;


#endif /* __spk_parser_h__ */
