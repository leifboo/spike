
#include "parser.h"

#include "class.h"
#include "gram.h"
#include "heart.h"
#include "host.h"
#include "lexer.h"
#include "rodata.h"
#include "st.h"
#include "sym.h"

#include <stdio.h>
#include <stdlib.h>


#define X(o) (o) ? (Unknown *)(o) : GLOBAL(null)


static SymbolNode *symbolNodeFromCString(Unknown *st, const char *str) {
    Symbol *sym;
    Unknown *tmp;
    SymbolNode *node;
    
    sym = Symbol_FromCString(str);
    tmp = Send(GLOBAL(theInterpreter), st, symbolNodeForSymbol, sym, 0);
    node = CAST(XSymbolNode, tmp);
    return node;
}

static SymbolNode *symbolNodeForToken(Token *token, Unknown *st) {
    switch (token->id) {
    case TOKEN_CLASS:    return symbolNodeFromCString(st, "class");
    case TOKEN_BREAK:    return symbolNodeFromCString(st, "break");
    case TOKEN_CONTINUE: return symbolNodeFromCString(st, "continue");
    case TOKEN_DO:       return symbolNodeFromCString(st, "do");
    case TOKEN_ELSE:     return symbolNodeFromCString(st, "else");
    case TOKEN_FOR:      return symbolNodeFromCString(st, "for");
    case TOKEN_IF:       return symbolNodeFromCString(st, "if");
    case TOKEN_META:     return symbolNodeFromCString(st, "meta");
    case TOKEN_RETURN:   return symbolNodeFromCString(st, "return");
    case TOKEN_WHILE:    return symbolNodeFromCString(st, "while");
    case TOKEN_YIELD:    return symbolNodeFromCString(st, "yield");
    
    case TOKEN_IDENTIFIER:
    case TOKEN_SPECIFIER:
    case TOKEN_LITERAL_SYMBOL:
        return CAST(XSymbolNode, token->value);
    }
    return 0;
}

static Unknown *operSelector(Oper oper) {
    switch (oper) {
    case OPER_SUCC:     return operSucc;
    case OPER_PRED:     return operPred;
    case OPER_ADDR:     return operAddr;
    case OPER_IND:      return operInd;
    case OPER_POS:      return operPos;
    case OPER_NEG:      return operNeg;
    case OPER_BNEG:     return operBNeg;
    case OPER_LNEG:     return operLNeg;
    case OPER_MUL:      return operMul;
    case OPER_DIV:      return operDiv;
    case OPER_MOD:      return operMod;
    case OPER_ADD:      return operAdd;
    case OPER_SUB:      return operSub;
    case OPER_LSHIFT:   return operLShift;
    case OPER_RSHIFT:   return operRShift;
    case OPER_LT:       return operLT;
    case OPER_GT:       return operGT;
    case OPER_LE:       return operLE;
    case OPER_GE:       return operGE;
    case OPER_EQ:       return operEq;
    case OPER_NE:       return operNE;
    case OPER_BAND:     return operBAnd;
    case OPER_BXOR:     return operBXOr;
    case OPER_BOR:      return operBOr;
    }
    return 0;
}

static Unknown *callOperSelector(CallOper oper) {
    switch (oper) {
    case OPER_APPLY:    return operApply;
    case OPER_INDEX:    return operIndex;
    }
    return 0;
}

static Unknown *exprSelector(StmtKind kind) {
    switch (kind) {
    case EXPR_AND:          return exprAnd;
    case EXPR_ASSIGN:       return exprAssign;
    case EXPR_ATTR:         return exprAttr;
    case EXPR_ATTR_VAR:     return exprAttrVar;
    case EXPR_BINARY:       return exprBinaryOp;
    case EXPR_BLOCK:        return exprBlock;
    case EXPR_CALL:         return exprCall;
    case EXPR_COMPOUND:     return exprCompound;
    case EXPR_COND:         return exprCond;
    case EXPR_ID:           return exprId;
    case EXPR_KEYWORD:      return exprKeyword;
    case EXPR_LITERAL:      return exprLiteral;
    case EXPR_NAME:         return exprName;
    case EXPR_NI:           return exprNI;
    case EXPR_OR:           return exprOr;
    case EXPR_POSTOP:       return exprPostOp;
    case EXPR_PREOP:        return exprPreOp;
    case EXPR_UNARY:        return exprUnaryOp;
    }
    return 0;
}

static Unknown *stmtSelector(StmtKind kind) {
    switch (kind) {
    case STMT_BREAK:            return stmtBreak;
    case STMT_COMPOUND:         return stmtCompound;
    case STMT_CONTINUE:         return stmtContinue;
    case STMT_DEF_CLASS:        return stmtDefClass;
    case STMT_DEF_METHOD:       return stmtDefMethod;
    case STMT_DEF_MODULE:       return stmtDefModule;
    case STMT_DEF_SPEC:         return stmtDefSpec;
    case STMT_DEF_VAR:          return stmtDefVar;
    case STMT_DO_WHILE:         return stmtDoWhile;
    case STMT_EXPR:             return stmtExpr;
    case STMT_FOR:              return stmtFor;
    case STMT_IF_ELSE:          return stmtIfElse;
    case STMT_PRAGMA_SOURCE:    return stmtPragmaSource;
    case STMT_RETURN:           return stmtReturn;
    case STMT_WHILE:            return stmtWhile;
    case STMT_YIELD:            return stmtYield;
    }
    return 0;
}


/****************************************************************************/
/* grammar actions */

Stmt *Parser_NewClassDef(Token *name, Token *super,
                            Stmt *body, Stmt *metaBody,
                            Parser *parser)
{
    Stmt *newStmt;
    SymbolNode *superSym;
    
    if (parser->tb) {
        /* XXX: bogus cast */
        newStmt = (Stmt *)Send(GLOBAL(theInterpreter),
                                      parser->tb, stmtSelector(STMT_DEF_CLASS),
                                      super ? super->value : GLOBAL(null),
                                      X(body), X(metaBody),
                                      0);
        XDECREF(body);
        XDECREF(metaBody);
        return newStmt;
    }
    
    superSym = super
               ? CAST(XSymbolNode, super->value)
               : symbolNodeFromCString(parser->st, "Object");
    
    newStmt = (Stmt *)Object_New(CLASS(XStmt));
    newStmt->kind = STMT_DEF_CLASS;
    newStmt->top = body;
    newStmt->bottom = metaBody;
    newStmt->expr = Parser_NewExpr(EXPR_NAME, 0, 0, 0, 0, name, parser);
    newStmt->expr->sym = CAST(XSymbolNode, name->value);
    newStmt->u.klass.superclassName = Parser_NewExpr(EXPR_NAME, 0, 0, 0, 0, super, parser);
    newStmt->u.klass.superclassName->sym = superSym;
    return newStmt;
}

Expr *Parser_NewAttrExpr(Expr *obj, Token *attrName,
                            Token *dot, Parser *parser)
{
    Expr *newExpr;
    
    newExpr = Parser_NewExpr(EXPR_ATTR, 0, 0, obj, 0, dot, parser);
    if (!parser->tb) /*XXX*/
        newExpr->sym = symbolNodeForToken(attrName, parser->st);
    return newExpr;
}

Expr *Parser_NewClassAttrExpr(Token *className, Token *attrName,
                                 Token *dot, Parser *parser)
{
    Expr *obj, *newExpr;
    
    obj = Parser_NewExpr(EXPR_NAME, 0, 0, 0, 0, className, parser);
    if (parser->tb) /*XXX*/
        return obj;
    obj->sym = CAST(XSymbolNode, className->value);
    newExpr = Parser_NewExpr(EXPR_ATTR, 0, 0, obj, 0, dot, parser);
    newExpr->sym = symbolNodeForToken(attrName, parser->st);
    return newExpr;
}

Stmt *Parser_NewStmt(StmtKind kind, Expr *expr, Stmt *top, Stmt *bottom,
                        Parser *parser)
{
    Stmt *newStmt;
    
    if (parser->tb) {
        /* XXX: bogus cast */
        newStmt = (Stmt *)Send(GLOBAL(theInterpreter),
                                      parser->tb, stmtSelector(kind),
                                      X(expr), X(top), X(bottom),
                                      0);
        XDECREF(expr);
        XDECREF(top);
        XDECREF(bottom);
        return newStmt;
    }
    
    newStmt = (Stmt *)Object_New(CLASS(XStmt));
    newStmt->kind = kind;
    newStmt->top = top;
    newStmt->bottom = bottom;
    newStmt->expr = expr;
    
    if (kind == STMT_EXPR && expr && expr->declSpecs) {
        /* XXX: There are at least a dozen cases where a declarator is *not* allowed. */
        newStmt->kind = STMT_DEF_VAR;
    }
    
    return newStmt;
}

Stmt *Parser_NewForStmt(Expr *expr1, Expr *expr2, Expr *expr3, Stmt *body,
                           Parser *parser)
{
    Stmt *newStmt;
    
    if (parser->tb) {
        /* XXX: bogus cast */
        newStmt = (Stmt *)Send(GLOBAL(theInterpreter),
                                      parser->tb, stmtSelector(STMT_FOR),
                                      X(expr1), X(expr2), X(expr3), X(body),
                                      0);
        XDECREF(expr1);
        XDECREF(expr2);
        XDECREF(expr3);
        XDECREF(body);
        return newStmt;
    }
    
    newStmt = (Stmt *)Object_New(CLASS(XStmt));
    newStmt->kind = STMT_FOR;
    newStmt->top = body;
    newStmt->init = expr1;
    newStmt->expr = expr2;
    newStmt->incr = expr3;
    return newStmt;
}

Expr *Parser_NewExpr(ExprKind kind, Oper oper, Expr *cond,
                        Expr *left, Expr *right,
                        Token *token, Parser *parser)
{
    Expr *newExpr;
    
    if (parser->tb) {
        /* XXX: bogus cast */
        /* XXX: 'oper' is sometimes a meaningless zero */
        switch (kind) {
        default:
            newExpr = (Expr *)Send(GLOBAL(theInterpreter),
                                          parser->tb, exprSelector(kind),
                                          operSelector(oper),
                                          X(cond), X(left), X(right),
                                          0);
            break;
        case EXPR_NAME:
        case EXPR_LITERAL:
            newExpr = (Expr *)Send(GLOBAL(theInterpreter),
                                          parser->tb, exprSelector(kind),
                                          token->value,
                                          0);
            break;
        }
        XDECREF(cond);
        XDECREF(left);
        XDECREF(right);
        return newExpr;
    }
    
    newExpr = (Expr *)Object_New(CLASS(XExpr));
    newExpr->kind = kind;
    newExpr->oper = oper;
    newExpr->cond = cond;
    newExpr->left = left;
    newExpr->right = right;
    if (token) {
        newExpr->lineNo = token->lineNo;
        switch (kind) {
        case EXPR_NAME:
            newExpr->sym = CAST(XSymbolNode, token->value);
            break;
        case EXPR_LITERAL:
            newExpr->aux.literalValue = token->value;
            break;
        }
    }
    return newExpr;
}

Expr *Parser_NewCallExpr(Oper oper,
                            Expr *func, ArgList *args,
                            Token *token, Parser *parser)
{
    Expr *newExpr;
    
    if (parser->tb) {
        /* XXX: bogus cast */
        newExpr = (Expr *)Send(GLOBAL(theInterpreter),
                                      parser->tb, exprSelector(EXPR_CALL),
                                      callOperSelector(oper),
                                      X(func),
                                      args ? (Unknown *)args->fixed : GLOBAL(null),
                                      args && args->var ? (Unknown *)args->var   : GLOBAL(null),
                                      /*X(token),*/
                                      0);
        XDECREF(func);
        XDECREF(args->fixed);
        XDECREF(args->var);
        return newExpr;
    }
    
    newExpr = (Expr *)Object_New(CLASS(XExpr));
    newExpr->kind = EXPR_CALL;
    newExpr->oper = oper;
    newExpr->left = func;
    newExpr->right = args ? args->fixed : 0;
    newExpr->var   = args ? args->var   : 0;
    if (token)
        newExpr->lineNo = token->lineNo;
    return newExpr;
}

Expr *Parser_NewBlock(Expr *argList, Stmt *stmtList, Expr *expr, Token *token,
                         Parser *parser)
{
    Expr *newExpr;
    
    if (parser->tb) {
        /* XXX: bogus cast */
        newExpr = (Expr *)Send(GLOBAL(theInterpreter),
                                      parser->tb, exprSelector(EXPR_BLOCK),
                                      X(argList), X(stmtList), X(expr), /*X(token),*/
                                      0);
        XDECREF(argList);
        XDECREF(stmtList);
        XDECREF(expr);
        return newExpr;
    }
    
    newExpr = (Expr *)Object_New(CLASS(XExpr));
    newExpr->kind = EXPR_BLOCK;
    newExpr->right = expr;
    newExpr->aux.block.argList = argList;
    newExpr->aux.block.stmtList = stmtList;
    newExpr->lineNo = token->lineNo;
    return newExpr;
}

Expr *Parser_NewKeywordExpr(Token *kw, Expr *arg,
                               Parser *parser)
{
    Expr *newExpr;
    SymbolNode *kwNode;
    
    if (parser->tb) {
        /* XXX: bogus cast */
        newExpr = (Expr *)Send(GLOBAL(theInterpreter),
                                      parser->tb, exprSelector(EXPR_KEYWORD),
                                      0);
        /* XXX */
        XDECREF(arg);
        return newExpr;
    }
    
    newExpr = (Expr *)Object_New(CLASS(XExpr));
    newExpr->kind = EXPR_KEYWORD;
    newExpr->right = arg;
    newExpr->lineNo = kw->lineNo;
    newExpr->aux.keywords = Host_NewKeywordSelectorBuilder();
    kwNode = symbolNodeForToken(kw, parser->st);
    Host_AppendKeyword(&newExpr->aux.keywords, kwNode->sym);
    DECREF(kwNode);
    return newExpr;
}

Expr *Parser_AppendKeyword(Expr *expr, Token *kw, Expr *arg,
                              Parser *parser)
{
    Expr *e;
    SymbolNode *kwNode;
    
    if (parser->tb) {
        /* XXX */
        XDECREF(arg);
        return expr;
    }
    
    kwNode = symbolNodeForToken(kw, parser->st);
    Host_AppendKeyword(&expr->aux.keywords, kwNode->sym);
    for (e = expr->right; e->nextArg; e = e->nextArg)
        ;
    e->nextArg = arg;
    DECREF(kwNode);
    return expr;
}

Expr *Parser_FreezeKeywords(Expr *expr, Token *kw,
                               Parser *parser)
{
    Unknown *tmp;
    SymbolNode *kwNode;
    
    if (parser->tb) {
        /* XXX */
        return expr;
    }
    
    kwNode = kw ? symbolNodeForToken(kw, parser->st) : 0;
    if (!expr) {
        expr = (Expr *)Object_New(CLASS(XExpr));
        expr->kind = EXPR_KEYWORD;
        expr->lineNo = kw->lineNo;
        expr->aux.keywords = Host_NewKeywordSelectorBuilder();
    } else if (kw) {
        expr->lineNo = kw->lineNo;
    }
    tmp = Host_GetKeywordSelector(expr->aux.keywords, kwNode ? kwNode->sym : 0);
    DECREF(expr->aux.keywords);
    expr->aux.keywords = tmp;
    XDECREF(kwNode);
    return expr;
}

Expr *Parser_NewCompoundExpr(Expr *args, Token *token, Parser *parser) {
    Expr *arg;
    
    /* XXX: parser->tb */
    
    /* convert comma expression to argument list */
    for (arg = args; arg; arg = arg->nextArg) {
        arg->nextArg = arg->next;
        arg->next = 0;
    }
    return Parser_NewExpr(EXPR_COMPOUND, 0, 0, 0, args, token, parser);
}

void Parser_SetNextExpr(Expr *expr, Expr *nextExpr, Parser *parser) {
    if (parser->tb) {
        Unknown *tmp = SetAttr(GLOBAL(theInterpreter),
                                      (Unknown *)expr, next, (Unknown *)nextExpr);
        DECREF(tmp);
        DECREF(nextExpr);
    } else {
        expr->next = nextExpr;
    }
}

void Parser_SetLeftExpr(Expr *expr, Expr *leftExpr, Parser *parser) {
    if (parser->tb) {
        Unknown *tmp = SetAttr(GLOBAL(theInterpreter),
                                      (Unknown *)expr, left, (Unknown *)leftExpr);
        DECREF(tmp);
        DECREF(leftExpr);
    } else {
        expr->left = leftExpr;
    }
}

void Parser_SetNextArg(Expr *expr, Expr *_nextArg, Parser *parser) {
    if (parser->tb) {
        Unknown *tmp = SetAttr(GLOBAL(theInterpreter),
                                      (Unknown *)expr, nextArg, (Unknown *)_nextArg);
        DECREF(tmp);
        DECREF(nextArg);
    } else {
        expr->nextArg = _nextArg;
    }
}

void Parser_SetDeclSpecs(Expr *expr, Expr *_declSpecs, Parser *parser) {
    if (parser->tb) {
        Unknown *tmp = SetAttr(GLOBAL(theInterpreter),
                                      (Unknown *)expr, declSpecs, (Unknown *)_declSpecs);
        DECREF(tmp);
        DECREF(declSpecs);
    } else {
        expr->declSpecs = _declSpecs;
    }
}

void Parser_SetNextStmt(Stmt *stmt, Stmt *nextStmt, Parser *parser) {
    if (parser->tb) {
        Unknown *tmp = SetAttr(GLOBAL(theInterpreter),
                                      (Unknown *)stmt, next, (Unknown *)nextStmt);
        DECREF(tmp);
        DECREF(nextStmt);
    } else {
        stmt->next = nextStmt;
    }
}

void Parser_Concat(Expr *expr, Token *token, Parser *parser) {
    if (parser->tb) {
        Send(GLOBAL(theInterpreter),
                 (Unknown *)expr, concat,
                 token->value,
                 0);
    } else {
        Host_StringConcat(&expr->aux.literalValue, token->value);
    }
}

Stmt *Parser_NewModuleDef(Stmt *stmtList) {
    /* Wrap the top-level statement list in a module (class)
       definition.  XXX: Where does this code really belong? */
    Stmt *compoundStmt, *moduleDef;
    
    compoundStmt = (Stmt *)Object_New(CLASS(XStmt));
    compoundStmt->kind = STMT_COMPOUND;
    compoundStmt->top = stmtList;

    moduleDef = (Stmt *)Object_New(CLASS(XStmt));
    moduleDef->kind = STMT_DEF_MODULE;
    moduleDef->top = compoundStmt;
    
    return moduleDef;
}

/****************************************************************************/

static Stmt *parse(yyscan_t lexer, SymbolTable *st) {
    void *parser;
    Token token;
    Parser parserState;
    
    parserState.root = 0;
    parserState.error = 0;
    parserState.st = (Unknown *)st; INCREF(st);
    parserState.tb = 0;
    
    parser = Parser_ParseAlloc(&malloc);
    while (Lexer_GetNextToken(&token, lexer, (Unknown *)st)) {
        Parser_Parse(parser, token.id, token, &parserState);
    }
    if (token.id == -1) {
        Parser_ParseFree(parser, &free);
        DECREF(parserState.st);
        /*XDECREF(parserState.root);*/
        return 0;
    }
    Parser_Parse(parser, 0, token, &parserState);
    Parser_ParseFree(parser, &free);
    
    DECREF(parserState.st);
    if (parserState.error) {
        /*XDECREF(parserState.root);*/
        return 0;
    }
    
    return parserState.root;
}

Stmt *Parser_ParseFileStream(FILE *yyin, SymbolTable *st) {
    yyscan_t lexer;
    char buffer[1024];
    Stmt *tree;
    
    Lexer_lex_init(&lexer);
    Lexer_restart(yyin, lexer);
    (void)fgets(buffer, sizeof(buffer), yyin); /* skip shebang (#!) line */
    Lexer_set_lineno(2, lexer);
    Lexer_set_column(1, lexer);
    
    tree = parse(lexer, st);
    
    Lexer_lex_destroy(lexer);
    
    return tree;
}

Stmt *Parser_ParseString(const char *input, SymbolTable *st) {
    yyscan_t lexer;
    Stmt *tree;
    
    Lexer_lex_init(&lexer);
    Lexer__scan_string(input, lexer);
    Lexer_set_lineno(1, lexer);
    Lexer_set_column(1, lexer);
    tree = parse(lexer, st);
    Lexer_lex_destroy(lexer);
    return tree;
}

void Parser_Source(Stmt **pStmtList, Unknown *source) {
    Stmt *pragma;
    
    pragma = (Stmt *)Object_New(CLASS(XStmt));
    pragma->kind = STMT_PRAGMA_SOURCE;
    pragma->u.source = source;  INCREF(source);
    pragma->next = *pStmtList;
    *pStmtList = pragma;
}


/*------------------------------------------------------------------------*/
/* methods */

static Unknown *Parser_init(Unknown *_self, Unknown *symbolTable, Unknown *treeBuilder) {
    Parser *self;
    
    self = (Parser *)_self;
    
    XDECREF(self->st);
    INCREF(symbolTable);
    self->st = symbolTable;
    
    if (treeBuilder == GLOBAL(null)) {
        CLEAR(self->tb);
    } else {
        INCREF(treeBuilder);
        XDECREF(self->tb);
        self->tb = treeBuilder;
    }
    
    INCREF(_self);
    return _self;
    
 unwind:
    return 0;
}

static Unknown *Parser_parse(Unknown *_self,
                                Unknown *input,
                                Unknown *arg1)
{
    Parser *self;
    FILE *stream = 0; const char *string = 0;
    yyscan_t lexer;
    void *parser;
    Token token;
    char buffer[1024];
    
    self = (Parser *)_self;
    
    if (Host_IsFileStream(input)) {
        stream = Host_FileStreamAsCFileStream(input);
    } else if (Host_IsString(input)) {
        string = Host_StringAsCString(input);
    } else {
        Halt(HALT_TYPE_ERROR, "a file stream or string is required");
        goto unwind;
    }
    
    Lexer_lex_init(&lexer);
    if (stream) {
        Lexer_restart(stream, lexer);
        (void)fgets(buffer, sizeof(buffer), stream); /* skip #! line */
        Lexer_set_lineno(2, lexer);
    } else {
        Lexer__scan_string(string, lexer);
        Lexer_set_lineno(1, lexer);
    }
    Lexer_set_column(1, lexer);
    
    parser = Parser_ParseAlloc(&malloc);
    while (Lexer_GetNextToken(&token, lexer, self->st)) {
        Parser_Parse(parser, token.id, token, self);
    }
    if (token.id == -1) {
        Parser_ParseFree(parser, &free);
        Lexer_lex_destroy(lexer);
        INCREF(GLOBAL(null));
        return GLOBAL(null);
    }
    Parser_Parse(parser, 0, token, self);
    Parser_ParseFree(parser, &free);
    
    Lexer_lex_destroy(lexer);
    
    if (self->error) {
        INCREF(GLOBAL(null));
        return GLOBAL(null);
    }
    return (Unknown *)self->root;
    
 unwind:
    return 0;
}


/*------------------------------------------------------------------------*/
/* meta-methods */

static Unknown *ClassParser_new(Unknown *self, Unknown *arg0, Unknown *arg1) {
    Unknown *newParser, *tmp;
    
    newParser = Send(GLOBAL(theInterpreter), SUPER, new, 0);
    if (!newParser)
        return 0;
    tmp = newParser;
    newParser = Send(GLOBAL(theInterpreter), newParser, init, arg0, arg1, 0);
    DECREF(tmp);
    return newParser;
}


/*------------------------------------------------------------------------*/
/* low-level hooks */

static void Parser_zero(Object *_self) {
    Parser *self = (Parser *)_self;
    (*CLASS(Parser)->superclass->zero)(_self);
    self->root = 0;
    self->error = 0;
    self->st = 0;
    self->tb = 0;
}

static void Parser_dealloc(Object *_self, Unknown **l) {
    Parser *self = (Parser *)_self;
    if (0) { /* XXX: why? */
        XLDECREF(self->root, l);
    }
    XLDECREF(self->st, l);
    XLDECREF(self->tb, l);
    (*CLASS(Parser)->superclass->dealloc)(_self, l);
}


/*------------------------------------------------------------------------*/
/* class templates */

typedef struct ParserSubclass {
    Parser base;
    Unknown *variables[1]; /* stretchy */
} ParserSubclass;

static MethodTmpl methods[] = {
    { "init", NativeCode_ARGS_2, &Parser_init },
    { "parse", NativeCode_ARGS_1, &Parser_parse },
    { 0 }
};

static MethodTmpl metaMethods[] = {
    { "new", NativeCode_ARGS_2, &ClassParser_new },
    { 0 }
};

ClassTmpl ClassParserTmpl = {
    HEART_CLASS_TMPL(Parser, Object), {
        /*accessors*/ 0,
        methods,
        /*lvalueMethods*/ 0,
        offsetof(ParserSubclass, variables),
        /*itemSize*/ 0,
        &Parser_zero,
        &Parser_dealloc
    }, /*meta*/ {
        /*accessors*/ 0,
        metaMethods,
        /*lvalueMethods*/ 0
    }
};
