
#include "parser.h"

#include "class.h"
#include "gram.h"
#include "heart.h"
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


static SpkSymbolNode *symbolNodeForToken(SpkToken *token, SpkSymbolTable *st) {
    switch (token->id) {
    case Spk_TOKEN_ARG:      return SpkSymbolNode_FromCString(st, "arg");
    case Spk_TOKEN_CLASS:    return SpkSymbolNode_FromCString(st, "class");
    case Spk_TOKEN_BREAK:    return SpkSymbolNode_FromCString(st, "break");
    case Spk_TOKEN_CONTINUE: return SpkSymbolNode_FromCString(st, "continue");
    case Spk_TOKEN_DO:       return SpkSymbolNode_FromCString(st, "do");
    case Spk_TOKEN_ELSE:     return SpkSymbolNode_FromCString(st, "else");
    case Spk_TOKEN_FOR:      return SpkSymbolNode_FromCString(st, "for");
    case Spk_TOKEN_IF:       return SpkSymbolNode_FromCString(st, "if");
    case Spk_TOKEN_IMPORT:   return SpkSymbolNode_FromCString(st, "import");
    case Spk_TOKEN_META:     return SpkSymbolNode_FromCString(st, "meta");
    case Spk_TOKEN_RAISE:    return SpkSymbolNode_FromCString(st, "raise");
    case Spk_TOKEN_RETURN:   return SpkSymbolNode_FromCString(st, "return");
    case Spk_TOKEN_VAR:      return SpkSymbolNode_FromCString(st, "var");
    case Spk_TOKEN_WHILE:    return SpkSymbolNode_FromCString(st, "while");
    case Spk_TOKEN_YIELD:    return SpkSymbolNode_FromCString(st, "yield");
    
    case Spk_TOKEN_IDENTIFIER:
    case Spk_TOKEN_TYPE_IDENTIFIER:
    case Spk_TOKEN_SYMBOL:
        return token->sym;
    }
    return 0;
}


/****************************************************************************/
/* grammar actions */

Stmt *SpkParser_NewClassDef(SpkToken *name, SpkToken *super,
                            Stmt *body, Stmt *metaBody,
                            SpkSymbolTable *st)
{
    Stmt *newStmt;
    
    newStmt = (Stmt *)SpkObject_New(Spk_CLASS(Stmt));
    newStmt->kind = Spk_STMT_DEF_CLASS;
    newStmt->top = body;
    newStmt->bottom = metaBody;
    newStmt->expr = SpkParser_NewExpr(Spk_EXPR_NAME, 0, 0, 0, 0, name);
    newStmt->expr->sym = name->sym;
    newStmt->u.klass.superclassName = SpkParser_NewExpr(Spk_EXPR_NAME, 0, 0, 0, 0, super);
    newStmt->u.klass.superclassName->sym
        = super
        ? super->sym
        : SpkSymbolNode_FromCString(st, "Object");
    return newStmt;
}

Expr *SpkParser_NewAttrExpr(Expr *obj, SpkToken *attrName,
                            SpkToken *dot, SpkSymbolTable *st)
{
    Expr *newExpr;
    
    newExpr = SpkParser_NewExpr(Spk_EXPR_ATTR, 0, 0, obj, 0, dot);
    newExpr->sym = symbolNodeForToken(attrName, st);
    return newExpr;
}

Expr *SpkParser_NewClassAttrExpr(SpkToken *className, SpkToken *attrName,
                                 SpkToken *dot, SpkSymbolTable *st)
{
    Expr *obj, *newExpr;
    
    obj = SpkParser_NewExpr(Spk_EXPR_NAME, 0, 0, 0, 0, className);
    obj->sym = className->sym;
    newExpr = SpkParser_NewExpr(Spk_EXPR_ATTR, 0, 0, obj, 0, dot);
    newExpr->sym = symbolNodeForToken(attrName, st);
    return newExpr;
}

Stmt *SpkParser_NewStmt(StmtKind kind, Expr *expr, Stmt *top, Stmt *bottom) {
    Stmt *newStmt;
    
    newStmt = (Stmt *)SpkObject_New(Spk_CLASS(Stmt));
    newStmt->kind = kind;
    newStmt->top = top;
    newStmt->bottom = bottom;
    newStmt->expr = expr;
    return newStmt;
}

Stmt *SpkParser_NewForStmt(Expr *expr1, Expr *expr2, Expr *expr3, Stmt *body) {
    Stmt *newStmt;
    
    newStmt = (Stmt *)SpkObject_New(Spk_CLASS(Stmt));
    newStmt->kind = Spk_STMT_FOR;
    newStmt->top = body;
    newStmt->init = expr1;
    newStmt->expr = expr2;
    newStmt->incr = expr3;
    return newStmt;
}

Expr *SpkParser_NewExpr(ExprKind kind, SpkOper oper, Expr *cond,
                        Expr *left, Expr *right,
                        SpkToken *token)
{
    Expr *newExpr;
    
    newExpr = (Expr *)SpkObject_New(Spk_CLASS(Expr));
    newExpr->kind = kind;
    newExpr->oper = oper;
    newExpr->cond = cond;
    newExpr->left = left;
    newExpr->right = right;
    if (token)
        newExpr->lineNo = token->lineNo;
    return newExpr;
}

Expr *SpkParser_NewBlock(Stmt *stmtList, Expr *expr, SpkToken *token) {
    Expr *newExpr;
    
    newExpr = (Expr *)SpkObject_New(Spk_CLASS(Expr));
    newExpr->kind = Spk_EXPR_BLOCK;
    newExpr->right = expr;
    newExpr->aux.block.stmtList = stmtList;
    newExpr->lineNo = token->lineNo;
    return newExpr;
}

Expr *SpkParser_NewKeywordExpr(SpkToken *kw, Expr *arg,
                               SpkSymbolTable *st)
{
    Expr *newExpr;
    SpkSymbolNode *kwNode;
    
    newExpr = (Expr *)SpkObject_New(Spk_CLASS(Expr));
    newExpr->kind = Spk_EXPR_KEYWORD;
    newExpr->right = arg;
    newExpr->lineNo = kw->lineNo;
    newExpr->aux.keywords = SpkHost_NewKeywordSelectorBuilder();
    kwNode = symbolNodeForToken(kw, st);
    SpkHost_AppendKeyword(&newExpr->aux.keywords, kwNode->sym);
    Spk_DECREF(kwNode);
    return newExpr;
}

Expr *SpkParser_AppendKeyword(Expr *expr, SpkToken *kw, Expr *arg,
                              SpkSymbolTable *st)
{
    Expr *e;
    SpkSymbolNode *kwNode;
    
    kwNode = symbolNodeForToken(kw, st);
    SpkHost_AppendKeyword(&expr->aux.keywords, kwNode->sym);
    for (e = expr->right; e->nextArg; e = e->nextArg)
        ;
    e->nextArg = arg;
    Spk_DECREF(kwNode);
    return expr;
}

Expr *SpkParser_FreezeKeywords(Expr *expr, SpkToken *kw,
                               SpkSymbolTable *st)
{
    SpkUnknown *tmp;
    SpkSymbolNode *kwNode;
    
    kwNode = kw ? symbolNodeForToken(kw, st) : 0;
    if (!expr) {
        expr = (Expr *)SpkObject_New(Spk_CLASS(Expr));
        expr->kind = Spk_EXPR_KEYWORD;
        expr->lineNo = kw->lineNo;
        expr->aux.keywords = SpkHost_NewKeywordSelectorBuilder();
    } else if (kw) {
        expr->lineNo = kw->lineNo;
    }
    tmp = SpkHost_GetKeywordSelector(expr->aux.keywords, kwNode ? kwNode->sym : 0);
    Spk_DECREF(expr->aux.keywords);
    expr->aux.keywords = tmp;
    Spk_XDECREF(kwNode);
    return expr;
}

Expr *SpkParser_NewCompoundExpr(Expr *args, SpkToken *token) {
    Expr *arg;
    
    /* convert comma expression to argument list */
    for (arg = args; arg; arg = arg->nextArg) {
        arg->nextArg = arg->next;
        arg->next = 0;
    }
    return SpkParser_NewExpr(Spk_EXPR_COMPOUND, 0, 0, 0, args, token);
}

Stmt *SpkParser_NewModuleDef(Stmt *stmtList) {
    /* Wrap the top-level statement list in a module (class)
       definition.  XXX: Where does this code really belong? */
    SpkStmt *compoundStmt, *moduleDef;
    
    compoundStmt = (SpkStmt *)SpkObject_New(Spk_CLASS(Stmt));
    compoundStmt->kind = Spk_STMT_COMPOUND;
    compoundStmt->top = stmtList;

    moduleDef = (SpkStmt *)SpkObject_New(Spk_CLASS(Stmt));
    moduleDef->kind = Spk_STMT_DEF_MODULE;
    moduleDef->top = compoundStmt;
    
    return moduleDef;
}

/****************************************************************************/

static Stmt *parse(yyscan_t lexer, SpkSymbolTable *st) {
    void *parser;
    SpkToken token;
    SpkParser parserState;
    
    parserState.root = 0;
    parserState.error = 0;
    parserState.st = st; Spk_INCREF(st);
    
    parser = SpkParser_ParseAlloc(&malloc);
    while (SpkLexer_GetNextToken(&token, lexer, st)) {
        SpkParser_Parse(parser, token.id, token, &parserState);
    }
    if (token.id == -1) {
        SpkParser_ParseFree(parser, &free);
        Spk_DECREF(parserState.st);
        /*Spk_XDECREF(parserState.root);*/
        return 0;
    }
    SpkParser_Parse(parser, 0, token, &parserState);
    SpkParser_ParseFree(parser, &free);
    
    Spk_DECREF(parserState.st);
    if (parserState.error) {
        /*Spk_XDECREF(parserState.root);*/
        return 0;
    }
    
    return parserState.root;
}

Stmt *SpkParser_ParseFileStream(FILE *yyin, SpkSymbolTable *st) {
    yyscan_t lexer;
    char buffer[1024];
    Stmt *tree;
    
    SpkLexer_lex_init(&lexer);
    SpkLexer_restart(yyin, lexer);
    fgets(buffer, sizeof(buffer), yyin); /* skip shebang (#!) line */
    SpkLexer_set_lineno(2, lexer);
    SpkLexer_set_column(1, lexer);
    
    tree = parse(lexer, st);
    
    SpkLexer_lex_destroy(lexer);
    
    return tree;
}

Stmt *SpkParser_ParseString(const char *input, SpkSymbolTable *st) {
    yyscan_t lexer;
    Stmt *tree;
    
    SpkLexer_lex_init(&lexer);
    SpkLexer__scan_string(input, lexer);
    SpkLexer_set_lineno(1, lexer);
    SpkLexer_set_column(1, lexer);
    tree = parse(lexer, st);
    SpkLexer_lex_destroy(lexer);
    return tree;
}

void SpkParser_Source(SpkStmt **pStmtList, SpkUnknown *source) {
    Stmt *pragma;
    
    pragma = SpkParser_NewStmt(Spk_STMT_PRAGMA_SOURCE, 0, 0, 0);
    pragma->u.source = source;  Spk_INCREF(source);
    pragma->next = *pStmtList;
    *pStmtList = pragma;
}


/*------------------------------------------------------------------------*/
/* methods */

static SpkUnknown *Parser_parse(SpkUnknown *_self,
                                SpkUnknown *input,
                                SpkUnknown *symbolTable)
{
    SpkParser *self;
    FILE *stream = 0; const char *string = 0; SpkSymbolTable *st;
    yyscan_t lexer;
    void *parser;
    SpkToken token;
    char buffer[1024];
    
    self = (SpkParser *)_self;
    
    if (SpkHost_IsFileStream(input)) {
        stream = SpkHost_FileStreamAsCFileStream(input);
    } else if (SpkHost_IsString(input)) {
        string = SpkHost_StringAsCString(input);
    } else {
        Spk_Halt(Spk_HALT_TYPE_ERROR, "a file stream or string is required");
        goto unwind;
    }
    
    st = Spk_CAST(SymbolTable, symbolTable);
    if (!st) {
        Spk_Halt(Spk_HALT_TYPE_ERROR, "a symbol table is required");
        goto unwind;
    }
    
    SpkLexer_lex_init(&lexer);
    if (stream) {
        SpkLexer_restart(stream, lexer);
        fgets(buffer, sizeof(buffer), stream); /* skip #! line */
        SpkLexer_set_lineno(2, lexer);
    } else {
        SpkLexer__scan_string(string, lexer);
        SpkLexer_set_lineno(1, lexer);
    }
    SpkLexer_set_column(1, lexer);
    
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
    return (SpkUnknown *)self->root;
    
 unwind:
    return 0;
}


/*------------------------------------------------------------------------*/
/* low-level hooks */

static void Parser_zero(SpkObject *_self) {
    SpkParser *self = (SpkParser *)_self;
    (*Spk_CLASS(Parser)->superclass->zero)(_self);
    self->root = 0;
    self->error = 0;
    self->st = 0;
}


/*------------------------------------------------------------------------*/
/* memory layout of instances */

static void Parser_traverse_init(SpkObject *self) {
    (*Spk_CLASS(Parser)->superclass->traverse.init)(self);
}

static SpkUnknown **Parser_traverse_current(SpkObject *_self) {
    SpkParser *self;
    
    self = (SpkParser *)_self;
    if (0 && self->root)
        return (SpkUnknown **)&self->root;
    if (self->st)
        return (SpkUnknown **)&self->st;
    return (*Spk_CLASS(Parser)->superclass->traverse.current)(_self);
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
    (*Spk_CLASS(Parser)->superclass->traverse.next)(_self);
}


/*------------------------------------------------------------------------*/
/* class templates */

typedef struct SpkParserSubclass {
    SpkParser base;
    SpkUnknown *variables[1]; /* stretchy */
} SpkParserSubclass;

static SpkMethodTmpl methods[] = {
    { "parse", SpkNativeCode_METH_ATTR | SpkNativeCode_ARGS_2, &Parser_parse },
    { 0, 0, 0 }
};

static SpkTraverse traverse = {
    &Parser_traverse_init,
    &Parser_traverse_current,
    &Parser_traverse_next,
};

SpkClassTmpl Spk_ClassParserTmpl = {
    Spk_HEART_CLASS_TMPL(Parser, Object), {
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
