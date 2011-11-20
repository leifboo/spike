
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


#define X(o) (o) ? (SpkUnknown *)(o) : Spk_GLOBAL(null)


typedef SpkExprKind ExprKind;
typedef SpkStmtKind StmtKind;
typedef SpkExpr Expr;
typedef SpkExprList ExprList;
typedef SpkArgList ArgList;
typedef SpkStmt Stmt;
typedef SpkStmtList StmtList;


static SpkSymbolNode *symbolNodeFromCString(SpkUnknown *st, const char *str) {
    SpkSymbol *sym;
    SpkUnknown *tmp;
    SpkSymbolNode *node;
    
    sym = SpkSymbol_FromCString(str);
    tmp = Spk_Send(Spk_GLOBAL(theInterpreter), st, Spk_symbolNodeForSymbol, sym, 0);
    node = Spk_CAST(XSymbolNode, tmp);
    return node;
}

static SpkSymbolNode *symbolNodeForToken(SpkToken *token, SpkUnknown *st) {
    switch (token->id) {
    case Spk_TOKEN_CLASS:    return symbolNodeFromCString(st, "class");
    case Spk_TOKEN_BREAK:    return symbolNodeFromCString(st, "break");
    case Spk_TOKEN_CONTINUE: return symbolNodeFromCString(st, "continue");
    case Spk_TOKEN_DO:       return symbolNodeFromCString(st, "do");
    case Spk_TOKEN_ELSE:     return symbolNodeFromCString(st, "else");
    case Spk_TOKEN_FOR:      return symbolNodeFromCString(st, "for");
    case Spk_TOKEN_IF:       return symbolNodeFromCString(st, "if");
    case Spk_TOKEN_META:     return symbolNodeFromCString(st, "meta");
    case Spk_TOKEN_RETURN:   return symbolNodeFromCString(st, "return");
    case Spk_TOKEN_WHILE:    return symbolNodeFromCString(st, "while");
    case Spk_TOKEN_YIELD:    return symbolNodeFromCString(st, "yield");
    
    case Spk_TOKEN_IDENTIFIER:
    case Spk_TOKEN_SPECIFIER:
    case Spk_TOKEN_LITERAL_SYMBOL:
        return Spk_CAST(XSymbolNode, token->value);
    }
    return 0;
}

static SpkUnknown *operSelector(SpkOper oper) {
    switch (oper) {
    case Spk_OPER_SUCC:     return Spk_operSucc;
    case Spk_OPER_PRED:     return Spk_operPred;
    case Spk_OPER_ADDR:     return Spk_operAddr;
    case Spk_OPER_IND:      return Spk_operInd;
    case Spk_OPER_POS:      return Spk_operPos;
    case Spk_OPER_NEG:      return Spk_operNeg;
    case Spk_OPER_BNEG:     return Spk_operBNeg;
    case Spk_OPER_LNEG:     return Spk_operLNeg;
    case Spk_OPER_MUL:      return Spk_operMul;
    case Spk_OPER_DIV:      return Spk_operDiv;
    case Spk_OPER_MOD:      return Spk_operMod;
    case Spk_OPER_ADD:      return Spk_operAdd;
    case Spk_OPER_SUB:      return Spk_operSub;
    case Spk_OPER_LSHIFT:   return Spk_operLShift;
    case Spk_OPER_RSHIFT:   return Spk_operRShift;
    case Spk_OPER_LT:       return Spk_operLT;
    case Spk_OPER_GT:       return Spk_operGT;
    case Spk_OPER_LE:       return Spk_operLE;
    case Spk_OPER_GE:       return Spk_operGE;
    case Spk_OPER_EQ:       return Spk_operEq;
    case Spk_OPER_NE:       return Spk_operNE;
    case Spk_OPER_BAND:     return Spk_operBAnd;
    case Spk_OPER_BXOR:     return Spk_operBXOr;
    case Spk_OPER_BOR:      return Spk_operBOr;
    }
    return 0;
}

static SpkUnknown *callOperSelector(SpkCallOper oper) {
    switch (oper) {
    case Spk_OPER_APPLY:    return Spk_operApply;
    case Spk_OPER_INDEX:    return Spk_operIndex;
    }
    return 0;
}

static SpkUnknown *exprSelector(SpkStmtKind kind) {
    switch (kind) {
    case Spk_EXPR_AND:          return Spk_exprAnd;
    case Spk_EXPR_ASSIGN:       return Spk_exprAssign;
    case Spk_EXPR_ATTR:         return Spk_exprAttr;
    case Spk_EXPR_ATTR_VAR:     return Spk_exprAttrVar;
    case Spk_EXPR_BINARY:       return Spk_exprBinaryOp;
    case Spk_EXPR_BLOCK:        return Spk_exprBlock;
    case Spk_EXPR_CALL:         return Spk_exprCall;
    case Spk_EXPR_COMPOUND:     return Spk_exprCompound;
    case Spk_EXPR_COND:         return Spk_exprCond;
    case Spk_EXPR_ID:           return Spk_exprId;
    case Spk_EXPR_KEYWORD:      return Spk_exprKeyword;
    case Spk_EXPR_LITERAL:      return Spk_exprLiteral;
    case Spk_EXPR_NAME:         return Spk_exprName;
    case Spk_EXPR_NI:           return Spk_exprNI;
    case Spk_EXPR_OR:           return Spk_exprOr;
    case Spk_EXPR_POSTOP:       return Spk_exprPostOp;
    case Spk_EXPR_PREOP:        return Spk_exprPreOp;
    case Spk_EXPR_UNARY:        return Spk_exprUnaryOp;
    }
    return 0;
}

static SpkUnknown *stmtSelector(SpkStmtKind kind) {
    switch (kind) {
    case Spk_STMT_BREAK:            return Spk_stmtBreak;
    case Spk_STMT_COMPOUND:         return Spk_stmtCompound;
    case Spk_STMT_CONTINUE:         return Spk_stmtContinue;
    case Spk_STMT_DEF_CLASS:        return Spk_stmtDefClass;
    case Spk_STMT_DEF_METHOD:       return Spk_stmtDefMethod;
    case Spk_STMT_DEF_MODULE:       return Spk_stmtDefModule;
    case Spk_STMT_DEF_SPEC:         return Spk_stmtDefSpec;
    case Spk_STMT_DEF_VAR:          return Spk_stmtDefVar;
    case Spk_STMT_DO_WHILE:         return Spk_stmtDoWhile;
    case Spk_STMT_EXPR:             return Spk_stmtExpr;
    case Spk_STMT_FOR:              return Spk_stmtFor;
    case Spk_STMT_IF_ELSE:          return Spk_stmtIfElse;
    case Spk_STMT_PRAGMA_SOURCE:    return Spk_stmtPragmaSource;
    case Spk_STMT_RETURN:           return Spk_stmtReturn;
    case Spk_STMT_WHILE:            return Spk_stmtWhile;
    case Spk_STMT_YIELD:            return Spk_stmtYield;
    }
    return 0;
}


/****************************************************************************/
/* grammar actions */

Stmt *SpkParser_NewClassDef(SpkToken *name, SpkToken *super,
                            Stmt *body, Stmt *metaBody,
                            SpkParser *parser)
{
    Stmt *newStmt;
    SpkSymbolNode *superSym;
    
    if (parser->tb) {
        /* XXX: bogus cast */
        newStmt = (SpkStmt *)Spk_Send(Spk_GLOBAL(theInterpreter),
                                      parser->tb, stmtSelector(Spk_STMT_DEF_CLASS),
                                      super ? super->value : Spk_GLOBAL(null),
                                      X(body), X(metaBody),
                                      0);
        Spk_XDECREF(body);
        Spk_XDECREF(metaBody);
        return newStmt;
    }
    
    superSym = super
               ? Spk_CAST(XSymbolNode, super->value)
               : symbolNodeFromCString(parser->st, "Object");
    
    newStmt = (Stmt *)SpkObject_New(Spk_CLASS(XStmt));
    newStmt->kind = Spk_STMT_DEF_CLASS;
    newStmt->top = body;
    newStmt->bottom = metaBody;
    newStmt->expr = SpkParser_NewExpr(Spk_EXPR_NAME, 0, 0, 0, 0, name, parser);
    newStmt->expr->sym = Spk_CAST(XSymbolNode, name->value);
    newStmt->u.klass.superclassName = SpkParser_NewExpr(Spk_EXPR_NAME, 0, 0, 0, 0, super, parser);
    newStmt->u.klass.superclassName->sym = superSym;
    return newStmt;
}

Expr *SpkParser_NewAttrExpr(Expr *obj, SpkToken *attrName,
                            SpkToken *dot, SpkParser *parser)
{
    Expr *newExpr;
    
    newExpr = SpkParser_NewExpr(Spk_EXPR_ATTR, 0, 0, obj, 0, dot, parser);
    if (!parser->tb) /*XXX*/
        newExpr->sym = symbolNodeForToken(attrName, parser->st);
    return newExpr;
}

Expr *SpkParser_NewClassAttrExpr(SpkToken *className, SpkToken *attrName,
                                 SpkToken *dot, SpkParser *parser)
{
    Expr *obj, *newExpr;
    
    obj = SpkParser_NewExpr(Spk_EXPR_NAME, 0, 0, 0, 0, className, parser);
    if (parser->tb) /*XXX*/
        return obj;
    obj->sym = Spk_CAST(XSymbolNode, className->value);
    newExpr = SpkParser_NewExpr(Spk_EXPR_ATTR, 0, 0, obj, 0, dot, parser);
    newExpr->sym = symbolNodeForToken(attrName, parser->st);
    return newExpr;
}

Stmt *SpkParser_NewStmt(StmtKind kind, Expr *expr, Stmt *top, Stmt *bottom,
                        SpkParser *parser)
{
    Stmt *newStmt;
    
    if (parser->tb) {
        /* XXX: bogus cast */
        newStmt = (SpkStmt *)Spk_Send(Spk_GLOBAL(theInterpreter),
                                      parser->tb, stmtSelector(kind),
                                      X(expr), X(top), X(bottom),
                                      0);
        Spk_XDECREF(expr);
        Spk_XDECREF(top);
        Spk_XDECREF(bottom);
        return newStmt;
    }
    
    newStmt = (Stmt *)SpkObject_New(Spk_CLASS(XStmt));
    newStmt->kind = kind;
    newStmt->top = top;
    newStmt->bottom = bottom;
    newStmt->expr = expr;
    
    if (kind == Spk_STMT_EXPR && expr && expr->declSpecs) {
        /* XXX: There are at least a dozen cases where a declarator is *not* allowed. */
        newStmt->kind = Spk_STMT_DEF_VAR;
    }
    
    return newStmt;
}

Stmt *SpkParser_NewForStmt(Expr *expr1, Expr *expr2, Expr *expr3, Stmt *body,
                           SpkParser *parser)
{
    Stmt *newStmt;
    
    if (parser->tb) {
        /* XXX: bogus cast */
        newStmt = (SpkStmt *)Spk_Send(Spk_GLOBAL(theInterpreter),
                                      parser->tb, stmtSelector(Spk_STMT_FOR),
                                      X(expr1), X(expr2), X(expr3), X(body),
                                      0);
        Spk_XDECREF(expr1);
        Spk_XDECREF(expr2);
        Spk_XDECREF(expr3);
        Spk_XDECREF(body);
        return newStmt;
    }
    
    newStmt = (Stmt *)SpkObject_New(Spk_CLASS(XStmt));
    newStmt->kind = Spk_STMT_FOR;
    newStmt->top = body;
    newStmt->init = expr1;
    newStmt->expr = expr2;
    newStmt->incr = expr3;
    return newStmt;
}

Expr *SpkParser_NewExpr(ExprKind kind, SpkOper oper, Expr *cond,
                        Expr *left, Expr *right,
                        SpkToken *token, SpkParser *parser)
{
    Expr *newExpr;
    
    if (parser->tb) {
        /* XXX: bogus cast */
        /* XXX: 'oper' is sometimes a meaningless zero */
        switch (kind) {
        default:
            newExpr = (SpkExpr *)Spk_Send(Spk_GLOBAL(theInterpreter),
                                          parser->tb, exprSelector(kind),
                                          operSelector(oper),
                                          X(cond), X(left), X(right),
                                          0);
            break;
        case Spk_EXPR_NAME:
        case Spk_EXPR_LITERAL:
            newExpr = (SpkExpr *)Spk_Send(Spk_GLOBAL(theInterpreter),
                                          parser->tb, exprSelector(kind),
                                          token->value,
                                          0);
            break;
        }
        Spk_XDECREF(cond);
        Spk_XDECREF(left);
        Spk_XDECREF(right);
        return newExpr;
    }
    
    newExpr = (Expr *)SpkObject_New(Spk_CLASS(XExpr));
    newExpr->kind = kind;
    newExpr->oper = oper;
    newExpr->cond = cond;
    newExpr->left = left;
    newExpr->right = right;
    if (token) {
        newExpr->lineNo = token->lineNo;
        switch (kind) {
        case Spk_EXPR_NAME:
            newExpr->sym = Spk_CAST(XSymbolNode, token->value);
            break;
        case Spk_EXPR_LITERAL:
            newExpr->aux.literalValue = token->value;
            break;
        }
    }
    return newExpr;
}

Expr *SpkParser_NewCallExpr(SpkOper oper,
                            Expr *func, SpkArgList *args,
                            SpkToken *token, SpkParser *parser)
{
    Expr *newExpr;
    
    if (parser->tb) {
        /* XXX: bogus cast */
        newExpr = (SpkExpr *)Spk_Send(Spk_GLOBAL(theInterpreter),
                                      parser->tb, exprSelector(Spk_EXPR_CALL),
                                      callOperSelector(oper),
                                      X(func),
                                      args ? (SpkUnknown *)args->fixed : Spk_GLOBAL(null),
                                      args && args->var ? (SpkUnknown *)args->var   : Spk_GLOBAL(null),
                                      /*X(token),*/
                                      0);
        Spk_XDECREF(func);
        Spk_XDECREF(args->fixed);
        Spk_XDECREF(args->var);
        return newExpr;
    }
    
    newExpr = (Expr *)SpkObject_New(Spk_CLASS(XExpr));
    newExpr->kind = Spk_EXPR_CALL;
    newExpr->oper = oper;
    newExpr->left = func;
    newExpr->right = args ? args->fixed : 0;
    newExpr->var   = args ? args->var   : 0;
    if (token)
        newExpr->lineNo = token->lineNo;
    return newExpr;
}

Expr *SpkParser_NewBlock(Expr *argList, Stmt *stmtList, Expr *expr, SpkToken *token,
                         SpkParser *parser)
{
    Expr *newExpr;
    
    if (parser->tb) {
        /* XXX: bogus cast */
        newExpr = (SpkExpr *)Spk_Send(Spk_GLOBAL(theInterpreter),
                                      parser->tb, exprSelector(Spk_EXPR_BLOCK),
                                      X(argList), X(stmtList), X(expr), /*X(token),*/
                                      0);
        Spk_XDECREF(argList);
        Spk_XDECREF(stmtList);
        Spk_XDECREF(expr);
        return newExpr;
    }
    
    newExpr = (Expr *)SpkObject_New(Spk_CLASS(XExpr));
    newExpr->kind = Spk_EXPR_BLOCK;
    newExpr->right = expr;
    newExpr->aux.block.argList = argList;
    newExpr->aux.block.stmtList = stmtList;
    newExpr->lineNo = token->lineNo;
    return newExpr;
}

Expr *SpkParser_NewKeywordExpr(SpkToken *kw, Expr *arg,
                               SpkParser *parser)
{
    Expr *newExpr;
    SpkSymbolNode *kwNode;
    
    if (parser->tb) {
        /* XXX: bogus cast */
        newExpr = (SpkExpr *)Spk_Send(Spk_GLOBAL(theInterpreter),
                                      parser->tb, exprSelector(Spk_EXPR_KEYWORD),
                                      0);
        /* XXX */
        Spk_XDECREF(arg);
        return newExpr;
    }
    
    newExpr = (Expr *)SpkObject_New(Spk_CLASS(XExpr));
    newExpr->kind = Spk_EXPR_KEYWORD;
    newExpr->right = arg;
    newExpr->lineNo = kw->lineNo;
    newExpr->aux.keywords = SpkHost_NewKeywordSelectorBuilder();
    kwNode = symbolNodeForToken(kw, parser->st);
    SpkHost_AppendKeyword(&newExpr->aux.keywords, kwNode->sym);
    Spk_DECREF(kwNode);
    return newExpr;
}

Expr *SpkParser_AppendKeyword(Expr *expr, SpkToken *kw, Expr *arg,
                              SpkParser *parser)
{
    Expr *e;
    SpkSymbolNode *kwNode;
    
    if (parser->tb) {
        /* XXX */
        Spk_XDECREF(arg);
        return expr;
    }
    
    kwNode = symbolNodeForToken(kw, parser->st);
    SpkHost_AppendKeyword(&expr->aux.keywords, kwNode->sym);
    for (e = expr->right; e->nextArg; e = e->nextArg)
        ;
    e->nextArg = arg;
    Spk_DECREF(kwNode);
    return expr;
}

Expr *SpkParser_FreezeKeywords(Expr *expr, SpkToken *kw,
                               SpkParser *parser)
{
    SpkUnknown *tmp;
    SpkSymbolNode *kwNode;
    
    if (parser->tb) {
        /* XXX */
        return expr;
    }
    
    kwNode = kw ? symbolNodeForToken(kw, parser->st) : 0;
    if (!expr) {
        expr = (Expr *)SpkObject_New(Spk_CLASS(XExpr));
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

Expr *SpkParser_NewCompoundExpr(Expr *args, SpkToken *token, SpkParser *parser) {
    Expr *arg;
    
    /* XXX: parser->tb */
    
    /* convert comma expression to argument list */
    for (arg = args; arg; arg = arg->nextArg) {
        arg->nextArg = arg->next;
        arg->next = 0;
    }
    return SpkParser_NewExpr(Spk_EXPR_COMPOUND, 0, 0, 0, args, token, parser);
}

void SpkParser_SetNextExpr(SpkExpr *expr, SpkExpr *nextExpr, SpkParser *parser) {
    if (parser->tb) {
        SpkUnknown *tmp = Spk_SetAttr(Spk_GLOBAL(theInterpreter),
                                      (SpkUnknown *)expr, Spk_next, (SpkUnknown *)nextExpr);
        Spk_DECREF(tmp);
        Spk_DECREF(nextExpr);
    } else {
        expr->next = nextExpr;
    }
}

void SpkParser_SetLeftExpr(SpkExpr *expr, SpkExpr *leftExpr, SpkParser *parser) {
    if (parser->tb) {
        SpkUnknown *tmp = Spk_SetAttr(Spk_GLOBAL(theInterpreter),
                                      (SpkUnknown *)expr, Spk_left, (SpkUnknown *)leftExpr);
        Spk_DECREF(tmp);
        Spk_DECREF(leftExpr);
    } else {
        expr->left = leftExpr;
    }
}

void SpkParser_SetNextArg(SpkExpr *expr, SpkExpr *nextArg, SpkParser *parser) {
    if (parser->tb) {
        SpkUnknown *tmp = Spk_SetAttr(Spk_GLOBAL(theInterpreter),
                                      (SpkUnknown *)expr, Spk_nextArg, (SpkUnknown *)nextArg);
        Spk_DECREF(tmp);
        Spk_DECREF(nextArg);
    } else {
        expr->nextArg = nextArg;
    }
}

void SpkParser_SetDeclSpecs(SpkExpr *expr, SpkExpr *declSpecs, SpkParser *parser) {
    if (parser->tb) {
        SpkUnknown *tmp = Spk_SetAttr(Spk_GLOBAL(theInterpreter),
                                      (SpkUnknown *)expr, Spk_declSpecs, (SpkUnknown *)declSpecs);
        Spk_DECREF(tmp);
        Spk_DECREF(declSpecs);
    } else {
        expr->declSpecs = declSpecs;
    }
}

void SpkParser_SetNextStmt(SpkStmt *stmt, SpkStmt *nextStmt, SpkParser *parser) {
    if (parser->tb) {
        SpkUnknown *tmp = Spk_SetAttr(Spk_GLOBAL(theInterpreter),
                                      (SpkUnknown *)stmt, Spk_next, (SpkUnknown *)nextStmt);
        Spk_DECREF(tmp);
        Spk_DECREF(nextStmt);
    } else {
        stmt->next = nextStmt;
    }
}

void SpkParser_Concat(SpkExpr *expr, SpkToken *token, SpkParser *parser) {
    if (parser->tb) {
        Spk_Send(Spk_GLOBAL(theInterpreter),
                 (SpkUnknown *)expr, Spk_concat,
                 token->value,
                 0);
    } else {
        SpkHost_StringConcat(&expr->aux.literalValue, token->value);
    }
}

Stmt *SpkParser_NewModuleDef(Stmt *stmtList) {
    /* Wrap the top-level statement list in a module (class)
       definition.  XXX: Where does this code really belong? */
    SpkStmt *compoundStmt, *moduleDef;
    
    compoundStmt = (SpkStmt *)SpkObject_New(Spk_CLASS(XStmt));
    compoundStmt->kind = Spk_STMT_COMPOUND;
    compoundStmt->top = stmtList;

    moduleDef = (SpkStmt *)SpkObject_New(Spk_CLASS(XStmt));
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
    parserState.st = (SpkUnknown *)st; Spk_INCREF(st);
    parserState.tb = 0;
    
    parser = SpkParser_ParseAlloc(&malloc);
    while (SpkLexer_GetNextToken(&token, lexer, (SpkUnknown *)st)) {
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
    (void)fgets(buffer, sizeof(buffer), yyin); /* skip shebang (#!) line */
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
    
    pragma = (Stmt *)SpkObject_New(Spk_CLASS(XStmt));
    pragma->kind = Spk_STMT_PRAGMA_SOURCE;
    pragma->u.source = source;  Spk_INCREF(source);
    pragma->next = *pStmtList;
    *pStmtList = pragma;
}


/*------------------------------------------------------------------------*/
/* methods */

static SpkUnknown *Parser_init(SpkUnknown *_self, SpkUnknown *symbolTable, SpkUnknown *treeBuilder) {
    SpkParser *self;
    
    self = (SpkParser *)_self;
    
    Spk_XDECREF(self->st);
    Spk_INCREF(symbolTable);
    self->st = symbolTable;
    
    if (treeBuilder == Spk_GLOBAL(null)) {
        Spk_CLEAR(self->tb);
    } else {
        Spk_INCREF(treeBuilder);
        Spk_XDECREF(self->tb);
        self->tb = treeBuilder;
    }
    
    Spk_INCREF(_self);
    return _self;
    
 unwind:
    return 0;
}

static SpkUnknown *Parser_parse(SpkUnknown *_self,
                                SpkUnknown *input,
                                SpkUnknown *arg1)
{
    SpkParser *self;
    FILE *stream = 0; const char *string = 0;
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
    
    SpkLexer_lex_init(&lexer);
    if (stream) {
        SpkLexer_restart(stream, lexer);
        (void)fgets(buffer, sizeof(buffer), stream); /* skip #! line */
        SpkLexer_set_lineno(2, lexer);
    } else {
        SpkLexer__scan_string(string, lexer);
        SpkLexer_set_lineno(1, lexer);
    }
    SpkLexer_set_column(1, lexer);
    
    parser = SpkParser_ParseAlloc(&malloc);
    while (SpkLexer_GetNextToken(&token, lexer, self->st)) {
        SpkParser_Parse(parser, token.id, token, self);
    }
    if (token.id == -1) {
        SpkParser_ParseFree(parser, &free);
        SpkLexer_lex_destroy(lexer);
        Spk_INCREF(Spk_GLOBAL(null));
        return Spk_GLOBAL(null);
    }
    SpkParser_Parse(parser, 0, token, self);
    SpkParser_ParseFree(parser, &free);
    
    SpkLexer_lex_destroy(lexer);
    
    if (self->error) {
        Spk_INCREF(Spk_GLOBAL(null));
        return Spk_GLOBAL(null);
    }
    return (SpkUnknown *)self->root;
    
 unwind:
    return 0;
}


/*------------------------------------------------------------------------*/
/* meta-methods */

static SpkUnknown *ClassParser_new(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    SpkUnknown *newParser, *tmp;
    
    newParser = Spk_Send(Spk_GLOBAL(theInterpreter), Spk_SUPER, Spk_new, 0);
    if (!newParser)
        return 0;
    tmp = newParser;
    newParser = Spk_Send(Spk_GLOBAL(theInterpreter), newParser, Spk_init, arg0, arg1, 0);
    Spk_DECREF(tmp);
    return newParser;
}


/*------------------------------------------------------------------------*/
/* low-level hooks */

static void Parser_zero(SpkObject *_self) {
    SpkParser *self = (SpkParser *)_self;
    (*Spk_CLASS(Parser)->superclass->zero)(_self);
    self->root = 0;
    self->error = 0;
    self->st = 0;
    self->tb = 0;
}

static void Parser_dealloc(SpkObject *_self, SpkUnknown **l) {
    SpkParser *self = (SpkParser *)_self;
    if (0) { /* XXX: why? */
        Spk_XLDECREF(self->root, l);
    }
    Spk_XLDECREF(self->st, l);
    Spk_XLDECREF(self->tb, l);
    (*Spk_CLASS(Parser)->superclass->dealloc)(_self, l);
}


/*------------------------------------------------------------------------*/
/* class templates */

typedef struct SpkParserSubclass {
    SpkParser base;
    SpkUnknown *variables[1]; /* stretchy */
} SpkParserSubclass;

static SpkMethodTmpl methods[] = {
    { "init", SpkNativeCode_ARGS_2, &Parser_init },
    { "parse", SpkNativeCode_ARGS_1, &Parser_parse },
    { 0 }
};

static SpkMethodTmpl metaMethods[] = {
    { "new", SpkNativeCode_ARGS_2, &ClassParser_new },
    { 0 }
};

SpkClassTmpl Spk_ClassParserTmpl = {
    Spk_HEART_CLASS_TMPL(Parser, Object), {
        /*accessors*/ 0,
        methods,
        /*lvalueMethods*/ 0,
        offsetof(SpkParserSubclass, variables),
        /*itemSize*/ 0,
        &Parser_zero,
        &Parser_dealloc
    }, /*meta*/ {
        /*accessors*/ 0,
        metaMethods,
        /*lvalueMethods*/ 0
    }
};
