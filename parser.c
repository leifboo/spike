
#include "parser.h"

#include "class.h"
#include "host.h"
#include "lexer.h"
#include "st.h"

#include <stdio.h>
#include <stdlib.h>


typedef SpkExprKind ExprKind;
typedef SpkStmtKind StmtKind;
typedef SpkExpr Expr;
typedef SpkExprList ExprList;
typedef SpkArgList ArgList;
typedef SpkStmt Stmt;
typedef SpkStmtList StmtList;


void SpkParser_Parse(void *yyp, int yymajor, SpkToken yyminor, SpkParser *);
void *SpkParser_ParseAlloc(void *(*mallocProc)(size_t));
void SpkParser_ParseFree(void *p, void (*freeProc)(void*));


SpkBehavior *Spk_ClassParser;
SpkClassTmpl Spk_ClassParserTmpl;


/****************************************************************************/
/* grammar actions */

Stmt *SpkParser_NewClassDef(SpkSymbolNode *name, SpkSymbolNode *super,
                            Stmt *body, Stmt *metaBody,
                            SpkSymbolTable *st)
{
    Stmt *newStmt;
    
    newStmt = (Stmt *)SpkObject_New(Spk_ClassStmt);
    newStmt->kind = Spk_STMT_DEF_CLASS;
    newStmt->top = body;
    newStmt->bottom = metaBody;
    newStmt->expr = SpkParser_NewExpr(Spk_EXPR_NAME, 0, 0, 0, 0);
    newStmt->expr->sym = name;
    if (!super)
        super = SpkSymbolNode_FromString(st, "Object");
    newStmt->u.klass.superclassName = SpkParser_NewExpr(Spk_EXPR_NAME, 0, 0, 0, 0);
    newStmt->u.klass.superclassName->sym = super;
    return newStmt;
}

Expr *SpkParser_NewClassAttrExpr(SpkSymbolNode *className, SpkSymbolNode *attrName) {
    Expr *obj, *newExpr;
    
    obj = SpkParser_NewExpr(Spk_EXPR_NAME, 0, 0, 0, 0);
    obj->sym = className;
    newExpr = SpkParser_NewExpr(Spk_EXPR_ATTR, 0, 0, obj, 0);
    newExpr->sym = attrName;
    return newExpr;
}

Stmt *SpkParser_NewStmt(StmtKind kind, Expr *expr, Stmt *top, Stmt *bottom) {
    Stmt *newStmt;
    
    newStmt = (Stmt *)SpkObject_New(Spk_ClassStmt);
    newStmt->kind = kind;
    newStmt->top = top;
    newStmt->bottom = bottom;
    newStmt->expr = expr;
    return newStmt;
}

Stmt *SpkParser_NewForStmt(Expr *expr1, Expr *expr2, Expr *expr3, Stmt *body) {
    Stmt *newStmt;
    
    newStmt = (Stmt *)SpkObject_New(Spk_ClassStmt);
    newStmt->kind = Spk_STMT_FOR;
    newStmt->top = body;
    newStmt->init = expr1;
    newStmt->expr = expr2;
    newStmt->incr = expr3;
    return newStmt;
}

Expr *SpkParser_NewExpr(ExprKind kind, SpkOper oper, Expr *cond,
                        Expr *left, Expr *right) {
    Expr *newExpr;
    
    newExpr = (Expr *)SpkObject_New(Spk_ClassExpr);
    newExpr->kind = kind;
    newExpr->oper = oper;
    newExpr->cond = cond;
    newExpr->left = left;
    newExpr->right = right;
    return newExpr;
}

Expr *SpkParser_NewBlock(Stmt *stmtList, Expr *expr) {
    Expr *newExpr;
    
    newExpr = (Expr *)SpkObject_New(Spk_ClassExpr);
    newExpr->kind = Spk_EXPR_BLOCK;
    newExpr->right = expr;
    newExpr->aux.block.stmtList = stmtList;
    return newExpr;
}

Expr *SpkParser_NewKeywordExpr(SpkSymbolNode *kw, Expr *arg) {
    Expr *newExpr;
    
    newExpr = (Expr *)SpkObject_New(Spk_ClassExpr);
    newExpr->kind = Spk_EXPR_KEYWORD;
    newExpr->right = arg;
    newExpr->aux.keywords = SpkHost_NewKeywordSelectorBuilder();
    SpkHost_AppendKeyword(&newExpr->aux.keywords, kw->sym);
    Spk_DECREF(kw);
    return newExpr;
}

Expr *SpkParser_AppendKeyword(Expr *expr, SpkSymbolNode *kw, Expr *arg) {
    Expr *e;
    
    SpkHost_AppendKeyword(&expr->aux.keywords, kw->sym);
    for (e = expr->right; e->nextArg; e = e->nextArg)
        ;
    e->nextArg = arg;
    Spk_DECREF(kw);
    return expr;
}

Expr *SpkParser_FreezeKeywords(Expr *expr, SpkSymbolNode *kw) {
    SpkUnknown *tmp;
    
    if (!expr) {
        expr = (Expr *)SpkObject_New(Spk_ClassExpr);
        expr->kind = Spk_EXPR_KEYWORD;
        expr->aux.keywords = SpkHost_NewKeywordSelectorBuilder();
    }
    tmp = SpkHost_GetKeywordSelector(expr->aux.keywords, kw ? kw->sym : 0);
    Spk_DECREF(expr->aux.keywords);
    expr->aux.keywords = tmp;
    Spk_XDECREF(kw);
    return expr;
}

Stmt *SpkParser_NewModuleDef(Stmt *stmtList) {
    /* Wrap the top-level statement list in a module (class)
       definition.  XXX: Where does this code really belong? */
    SpkStmt *compoundStmt, *moduleDef;
    
    compoundStmt = (SpkStmt *)SpkObject_New(Spk_ClassStmt);
    compoundStmt->kind = Spk_STMT_COMPOUND;
    compoundStmt->top = stmtList;

    moduleDef = (SpkStmt *)SpkObject_New(Spk_ClassStmt);
    moduleDef->kind = Spk_STMT_DEF_CLASS;
    moduleDef->top = compoundStmt;
    
    return moduleDef;
}

/****************************************************************************/

Stmt *SpkParser_ParseFile(FILE *yyin, SpkSymbolTable *st) {
    yyscan_t lexer;
    void *parser;
    SpkToken token;
    SpkParser parserState;
    char buffer[1024];
    
    SpkLexer_lex_init(&lexer);
    SpkLexer_restart(yyin, lexer);
    fgets(buffer, sizeof(buffer), yyin); /* skip #! line */
    SpkLexer_set_lineno(2, lexer);
    
    parserState.root = 0;
    parserState.error = 0;
    parserState.st = st; Spk_INCREF(st);
    
    parser = SpkParser_ParseAlloc(&malloc);
    while (SpkLexer_GetNextToken(&token, lexer, st)) {
        SpkParser_Parse(parser, token.id, token, &parserState);
    }
    if (token.id == -1) {
        SpkParser_ParseFree(parser, &free);
        SpkLexer_lex_destroy(lexer);
        Spk_DECREF(parserState.st);
        /*Spk_XDECREF(parserState.root);*/
        return 0;
    }
    SpkParser_Parse(parser, 0, token, &parserState);
    SpkParser_ParseFree(parser, &free);
    
    SpkLexer_lex_destroy(lexer);
    
    Spk_DECREF(parserState.st);
    if (parserState.error) {
        /*Spk_XDECREF(parserState.root);*/
        return 0;
    }
    
    return parserState.root;
}


/*------------------------------------------------------------------------*/
/* methods */

static SpkUnknown *Parser_parseFile(SpkUnknown *_self,
                                    SpkUnknown *stream,
                                    SpkUnknown *symbolTable)
{
    SpkParser *self;
    FILE *yyin; SpkSymbolTable *st;
    yyscan_t lexer;
    void *parser;
    SpkToken token;
    char buffer[1024];
    
    self = (SpkParser *)_self;
    
    if (!SpkHost_IsFileStream(stream)) {
        Spk_Halt(Spk_HALT_TYPE_ERROR, "a stream is required");
        goto unwind;
    }
    yyin = SpkHost_FileStreamAsCFileStream(stream);
    
    st = Spk_CAST(SymbolTable, symbolTable);
    if (!st) {
        Spk_Halt(Spk_HALT_TYPE_ERROR, "a symbol table is required");
        goto unwind;
    }
    
    SpkLexer_lex_init(&lexer);
    SpkLexer_restart(yyin, lexer);
    fgets(buffer, sizeof(buffer), yyin); /* skip #! line */
    SpkLexer_set_lineno(2, lexer);
    
    Spk_XDECREF(self->st);
    Spk_INCREF(st);
    self->st = st;
    
    parser = SpkParser_ParseAlloc(&malloc);
    while (SpkLexer_GetNextToken(&token, lexer, st)) {
        SpkParser_Parse(parser, token.id, token, self);
    }
    if (token.id == -1) {
        SpkParser_ParseFree(parser, &free);
        SpkLexer_lex_destroy(lexer);
        Spk_INCREF(Spk_null);
        return Spk_null;
    }
    SpkParser_Parse(parser, 0, token, self);
    SpkParser_ParseFree(parser, &free);
    
    SpkLexer_lex_destroy(lexer);
    
    if (self->error) {
        Spk_INCREF(Spk_null);
        return Spk_null;
    }
    return self->root;
    
 unwind:
    return 0;
}


/*------------------------------------------------------------------------*/
/* low-level hooks */

static void Parser_zero(SpkObject *_self) {
    SpkParser *self = (SpkParser *)_self;
    (*Spk_ClassParser->superclass->zero)(_self);
    self->root = 0;
    self->error = 0;
    self->st = 0;
}


/*------------------------------------------------------------------------*/
/* memory layout of instances */

static void Parser_traverse_init(SpkObject *self) {
    (*Spk_ClassParser->superclass->traverse.init)(self);
}

static SpkUnknown **Parser_traverse_current(SpkObject *_self) {
    SpkParser *self;
    
    self = (SpkParser *)_self;
    if (0 && self->root)
        return (SpkUnknown **)&self->root;
    if (self->st)
        return (SpkUnknown **)&self->st;
    return (*Spk_ClassParser->superclass->traverse.current)(_self);
}

static void Parser_traverse_next(SpkObject *_self) {
    SpkParser *self;
    
    self = (SpkParser *)_self;
    if (0 && self->root) {
        self->root = 0;
        return;
    }
    if (self->st) {
        self->st = 0;
        return;
    }
    (*Spk_ClassParser->superclass->traverse.next)(_self);
}


/*------------------------------------------------------------------------*/
/* class templates */

static SpkMethodTmpl methods[] = {
    { "parseFile", SpkNativeCode_METH_ATTR | SpkNativeCode_ARGS_2, &Parser_parseFile },
    { 0, 0, 0 }
};

static SpkTraverse traverse = {
    &Parser_traverse_init,
    &Parser_traverse_current,
    &Parser_traverse_next,
};

SpkClassTmpl Spk_ClassParserTmpl = {
    "Parser", {
        /*accessors*/ 0,
        methods,
        /*lvalueMethods*/ 0,
        offsetof(SpkParserSubclass, variables),
        /*itemSize*/ 0,
        &Parser_zero,
        /*dealloc*/ 0,
        &traverse
    }, /*meta*/ {
    }
};
