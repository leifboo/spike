
%name SpkParser_Parse
%token_type {SpkToken}
%token_prefix Spk_TOKEN_
%extra_argument { SpkParserState *parserState }

%include {
    #include <assert.h>
    #include "host.h"
    #include "lexer.h"
    #include "parser.h"
    #include "st.h"
}

start ::= statement_list(stmtList).                                             { parserState->root = stmtList.first; }

%type statement_list {SpkStmtList}
statement_list(r) ::= statement(stmt).                                          { r.first = stmt; r.last = stmt; }
statement_list(r) ::= statement_list(stmtList) statement(stmt).                 { r = stmtList; r.last->next = stmt; r.last = stmt; }

%type statement {SpkStmt *}
statement(r) ::= open_statement(stmt).                                          { r = stmt; }
statement(r) ::= closed_statement(stmt).                                        { r = stmt; }
//statement(r) ::= IDENTIFIER COLON statement(stmt).                              { r = stmt; }

%type open_statement {SpkStmt *}
open_statement(r) ::= IF LPAREN expr(expr) RPAREN statement(ifTrue).            { r = SpkParser_NewStmt(Spk_STMT_IF_ELSE, expr.first, ifTrue, 0); }
open_statement(r) ::= IF LPAREN expr(expr) RPAREN closed_statement(ifTrue)
                      ELSE open_statement(ifFalse).                             { r = SpkParser_NewStmt(Spk_STMT_IF_ELSE, expr.first, ifTrue, ifFalse); }
open_statement(r) ::= WHILE LPAREN expr(expr) RPAREN statement(body).           { r = SpkParser_NewStmt(Spk_STMT_WHILE, expr.first, body, 0); }
open_statement(r) ::= FOR LPAREN             SEMI             SEMI             RPAREN statement(body).
                                                                                { r = SpkParser_NewForStmt(          0,           0,           0, body); }
open_statement(r) ::= FOR LPAREN             SEMI             SEMI expr(expr3) RPAREN statement(body).
                                                                                { r = SpkParser_NewForStmt(          0,           0, expr3.first, body); }
open_statement(r) ::= FOR LPAREN             SEMI expr(expr2) SEMI             RPAREN statement(body).
                                                                                { r = SpkParser_NewForStmt(          0, expr2.first,           0, body); }
open_statement(r) ::= FOR LPAREN             SEMI expr(expr2) SEMI expr(expr3) RPAREN statement(body).
                                                                                { r = SpkParser_NewForStmt(          0, expr2.first, expr3.first, body); }
open_statement(r) ::= FOR LPAREN expr(expr1) SEMI             SEMI             RPAREN statement(body).
                                                                                { r = SpkParser_NewForStmt(expr1.first,           0,           0, body); }
open_statement(r) ::= FOR LPAREN expr(expr1) SEMI             SEMI expr(expr3) RPAREN statement(body).
                                                                                { r = SpkParser_NewForStmt(expr1.first,           0, expr3.first, body); }
open_statement(r) ::= FOR LPAREN expr(expr1) SEMI expr(expr2) SEMI             RPAREN statement(body).
                                                                                { r = SpkParser_NewForStmt(expr1.first, expr2.first,           0, body); }
open_statement(r) ::= FOR LPAREN expr(expr1) SEMI expr(expr2) SEMI expr(expr3) RPAREN statement(body).
                                                                                { r = SpkParser_NewForStmt(expr1.first, expr2.first, expr3.first, body); }

%type closed_statement {SpkStmt *}
closed_statement(r) ::= IF LPAREN expr(expr) RPAREN closed_statement(ifTrue)
                        ELSE closed_statement(ifFalse).                         { r = SpkParser_NewStmt(Spk_STMT_IF_ELSE, expr.first, ifTrue, ifFalse); }
closed_statement(r) ::= SEMI.                                                   { r = SpkParser_NewStmt(Spk_STMT_EXPR, 0, 0, 0); }
closed_statement(r) ::= expr(expr) SEMI.                                        { r = SpkParser_NewStmt(Spk_STMT_EXPR, expr.first, 0, 0); }
closed_statement(r) ::= compound_statement(stmt).                               { r = stmt; }
closed_statement(r) ::= DO statement(body) WHILE LPAREN expr(expr) RPAREN SEMI. { r = SpkParser_NewStmt(Spk_STMT_DO_WHILE, expr.first, body, 0); }
closed_statement(r) ::= CONTINUE SEMI.                                          { r = SpkParser_NewStmt(Spk_STMT_CONTINUE, 0, 0, 0); }
closed_statement(r) ::= BREAK SEMI.                                             { r = SpkParser_NewStmt(Spk_STMT_BREAK, 0, 0, 0); }
closed_statement(r) ::= RETURN            SEMI.                                 { r = SpkParser_NewStmt(Spk_STMT_RETURN, 0, 0, 0); }
closed_statement(r) ::= RETURN expr(expr) SEMI.                                 { r = SpkParser_NewStmt(Spk_STMT_RETURN, expr.first, 0, 0); }
closed_statement(r) ::= YIELD            SEMI.                                  { r = SpkParser_NewStmt(Spk_STMT_YIELD, 0, 0, 0); }
closed_statement(r) ::= YIELD expr(expr) SEMI.                                  { r = SpkParser_NewStmt(Spk_STMT_YIELD, expr.first, 0, 0); }
closed_statement(r) ::= RAISE expr(expr) SEMI.                                  { r = SpkParser_NewStmt(Spk_STMT_RAISE, expr.first, 0, 0); }
closed_statement(r) ::= IMPORT expr(expr) SEMI.                                 { r = SpkParser_NewStmt(Spk_STMT_IMPORT, expr.first, 0, 0); }
closed_statement(r) ::= IMPORT expr(names) FROM expr(module) SEMI.              { r = SpkParser_NewStmt(Spk_STMT_IMPORT, module.first, 0, 0); r->init = names.first; }
closed_statement(r) ::= ARG expr(declList) SEMI.                                { r = SpkParser_NewStmt(Spk_STMT_DEF_ARG, declList.first, 0, 0); }
closed_statement(r) ::= VAR expr(declList) SEMI.                                { r = SpkParser_NewStmt(Spk_STMT_DEF_VAR, declList.first, 0, 0); }
closed_statement(r) ::= expr(expr) compound_statement(stmt).                    { r = SpkParser_NewStmt(Spk_STMT_DEF_METHOD, expr.first, stmt, 0); }
closed_statement(r) ::= CLASS TYPE_IDENTIFIER(name) class_body(body).           { r = SpkParser_NewClassDef(name.sym, 0, body.top, body.bottom, parserState->st); }
closed_statement(r) ::= CLASS TYPE_IDENTIFIER(name) COLON TYPE_IDENTIFIER(super) class_body(body).
                                                                                { r = SpkParser_NewClassDef(name.sym, super.sym, body.top, body.bottom, parserState->st); }

%type class_body {SpkStmtPair}
class_body(r) ::= compound_statement(body).                                     { r.top = body; r.bottom = 0; }
class_body(r) ::= compound_statement(body) META compound_statement(metaBody).   { r.top = body; r.bottom = metaBody; } 

%type compound_statement {SpkStmt *}
compound_statement(r) ::= LCURLY                          RCURLY.               { r = SpkParser_NewStmt(Spk_STMT_COMPOUND, 0, 0, 0); }
compound_statement(r) ::= LCURLY statement_list(stmtList) RCURLY.               { r = SpkParser_NewStmt(Spk_STMT_COMPOUND, 0, stmtList.first, 0); }

%type expr {SpkExprList}
expr(r) ::= colon_expr(expr).                                                   { r.first = expr; r.last = expr; }
expr(r) ::= expr(left) COMMA colon_expr(right).                                 { r = left; r.last->next = right; r.last = right; }

%type colon_expr {SpkExpr *}
colon_expr(r) ::= assignment_expr(expr).                                        { r = expr; }
//colon_expr(r) ::= colon_expr COLON assignment_expr(expr).                       { r = expr; }

%type assignment_expr {SpkExpr *}
assignment_expr(r) ::= keyword_expr(expr).                                      { r = expr; }
assignment_expr(r) ::= unary_expr(left) ASSIGN        assignment_expr(right).   { r = SpkParser_NewExpr(Spk_EXPR_ASSIGN, Spk_OPER_EQ,     0, left, right); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_TIMES  assignment_expr(right).   { r = SpkParser_NewExpr(Spk_EXPR_ASSIGN, Spk_OPER_MUL,    0, left, right); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_DIVIDE assignment_expr(right).   { r = SpkParser_NewExpr(Spk_EXPR_ASSIGN, Spk_OPER_DIV,    0, left, right); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_MOD    assignment_expr(right).   { r = SpkParser_NewExpr(Spk_EXPR_ASSIGN, Spk_OPER_MOD,    0, left, right); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_PLUS   assignment_expr(right).   { r = SpkParser_NewExpr(Spk_EXPR_ASSIGN, Spk_OPER_ADD,    0, left, right); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_MINUS  assignment_expr(right).   { r = SpkParser_NewExpr(Spk_EXPR_ASSIGN, Spk_OPER_SUB,    0, left, right); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_LSHIFT assignment_expr(right).   { r = SpkParser_NewExpr(Spk_EXPR_ASSIGN, Spk_OPER_LSHIFT, 0, left, right); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_RSHIFT assignment_expr(right).   { r = SpkParser_NewExpr(Spk_EXPR_ASSIGN, Spk_OPER_RSHIFT, 0, left, right); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_BAND   assignment_expr(right).   { r = SpkParser_NewExpr(Spk_EXPR_ASSIGN, Spk_OPER_BAND,   0, left, right); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_BXOR   assignment_expr(right).   { r = SpkParser_NewExpr(Spk_EXPR_ASSIGN, Spk_OPER_BXOR,   0, left, right); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_BOR    assignment_expr(right).   { r = SpkParser_NewExpr(Spk_EXPR_ASSIGN, Spk_OPER_BOR,    0, left, right); }

%type keyword_expr {SpkExpr *}
keyword_expr(r) ::= conditional_expr(expr).                                     { r = expr; }
keyword_expr(r) ::= conditional_expr(receiver) keyword_message(expr).           { r = expr; r->left = receiver; }

%type keyword_message {SpkExpr *}
keyword_message(r) ::= keyword_list(expr).                                      { r = SpkParser_FreezeKeywords(expr, 0); }
keyword_message(r) ::= keyword(kw) keyword_list(expr).                          { r = SpkParser_FreezeKeywords(expr, kw); }

%type keyword_list {SpkExpr *}
keyword_list(r) ::= keyword(kw) COLON conditional_expr(arg).                    { r = SpkParser_NewKeywordExpr(kw, arg); }
keyword_list(r) ::= keyword_list(expr) keyword(kw) COLON conditional_expr(arg). { r = SpkParser_AppendKeyword(expr, kw, arg); }

%type keyword {SpkSymbolNode *}
keyword(r) ::= IDENTIFIER(token).                                               { r = token.sym; }
keyword(r) ::= ARG.                                                             { r = SpkSymbolNode_FromString(parserState->st, "arg"); }
keyword(r) ::= CLASS.                                                           { r = SpkSymbolNode_FromString(parserState->st, "class"); }
keyword(r) ::= BREAK.                                                           { r = SpkSymbolNode_FromString(parserState->st, "break"); }
keyword(r) ::= CONTINUE.                                                        { r = SpkSymbolNode_FromString(parserState->st, "continue"); }
keyword(r) ::= DO.                                                              { r = SpkSymbolNode_FromString(parserState->st, "do"); }
keyword(r) ::= ELSE.                                                            { r = SpkSymbolNode_FromString(parserState->st, "else"); }
keyword(r) ::= FOR.                                                             { r = SpkSymbolNode_FromString(parserState->st, "for"); }
keyword(r) ::= IF.                                                              { r = SpkSymbolNode_FromString(parserState->st, "if"); }
keyword(r) ::= IMPORT.                                                          { r = SpkSymbolNode_FromString(parserState->st, "import"); }
keyword(r) ::= META.                                                            { r = SpkSymbolNode_FromString(parserState->st, "meta"); }
keyword(r) ::= RAISE.                                                           { r = SpkSymbolNode_FromString(parserState->st, "raise"); }
keyword(r) ::= RETURN.                                                          { r = SpkSymbolNode_FromString(parserState->st, "return"); }
keyword(r) ::= VAR.                                                             { r = SpkSymbolNode_FromString(parserState->st, "var"); }
keyword(r) ::= WHILE.                                                           { r = SpkSymbolNode_FromString(parserState->st, "while"); }
keyword(r) ::= YIELD.                                                           { r = SpkSymbolNode_FromString(parserState->st, "yield"); }

%type conditional_expr {SpkExpr *}
conditional_expr(r) ::= logical_expr(expr).                                     { r = expr; }
conditional_expr(r) ::= logical_expr(cond) QM logical_expr(left) COLON conditional_expr(right).
                                                                                { r = SpkParser_NewExpr(Spk_EXPR_COND, 0, cond, left, right); }

%left OR.
%left AND.

%type logical_expr {SpkExpr *}
logical_expr(r) ::= binary_expr(expr).                                          { r = expr; }
logical_expr(r) ::= logical_expr(left) OR logical_expr(right).                  { r = SpkParser_NewExpr(Spk_EXPR_OR, 0, 0, left, right); }
logical_expr(r) ::= logical_expr(left) AND logical_expr(right).                 { r = SpkParser_NewExpr(Spk_EXPR_AND, 0, 0, left, right); }

%left ID NI EQ NE.
%left GT GE LT LE.
%left BOR.
%left BXOR.
%left AMP.
%left LSHIFT RSHIFT.
%left PLUS MINUS.
%left TIMES DIVIDE MOD.
%left DOT_STAR.

%type binary_expr {SpkExpr *}
binary_expr(r) ::= unary_expr(expr).                                            { r = expr; }
binary_expr(r) ::= binary_expr(left) ID binary_expr(right).                     { r = SpkParser_NewExpr(Spk_EXPR_ID, 0, 0, left, right); }
binary_expr(r) ::= binary_expr(left) NI binary_expr(right).                     { r = SpkParser_NewExpr(Spk_EXPR_NI, 0, 0, left, right); }
binary_expr(r) ::= binary_expr(left) EQ binary_expr(right).                     { r = SpkParser_NewExpr(Spk_EXPR_BINARY, Spk_OPER_EQ, 0, left, right); }
binary_expr(r) ::= binary_expr(left) NE binary_expr(right).                     { r = SpkParser_NewExpr(Spk_EXPR_BINARY, Spk_OPER_NE, 0, left, right); }
binary_expr(r) ::= binary_expr(left) GT binary_expr(right).                     { r = SpkParser_NewExpr(Spk_EXPR_BINARY, Spk_OPER_GT, 0, left, right); }
binary_expr(r) ::= binary_expr(left) GE binary_expr(right).                     { r = SpkParser_NewExpr(Spk_EXPR_BINARY, Spk_OPER_GE, 0, left, right); }
binary_expr(r) ::= binary_expr(left) LT binary_expr(right).                     { r = SpkParser_NewExpr(Spk_EXPR_BINARY, Spk_OPER_LT, 0, left, right); }
binary_expr(r) ::= binary_expr(left) LE binary_expr(right).                     { r = SpkParser_NewExpr(Spk_EXPR_BINARY, Spk_OPER_LE, 0, left, right); }
binary_expr(r) ::= binary_expr(left) BOR binary_expr(right).                    { r = SpkParser_NewExpr(Spk_EXPR_BINARY, Spk_OPER_BOR, 0, left, right); }
binary_expr(r) ::= binary_expr(left) BXOR binary_expr(right).                   { r = SpkParser_NewExpr(Spk_EXPR_BINARY, Spk_OPER_BXOR, 0, left, right); }
binary_expr(r) ::= binary_expr(left) AMP binary_expr(right).                    { r = SpkParser_NewExpr(Spk_EXPR_BINARY, Spk_OPER_BAND, 0, left, right); }
binary_expr(r) ::= binary_expr(left) LSHIFT binary_expr(right).                 { r = SpkParser_NewExpr(Spk_EXPR_BINARY, Spk_OPER_LSHIFT, 0, left, right); }
binary_expr(r) ::= binary_expr(left) RSHIFT binary_expr(right).                 { r = SpkParser_NewExpr(Spk_EXPR_BINARY, Spk_OPER_RSHIFT, 0, left, right); }
binary_expr(r) ::= binary_expr(left) PLUS binary_expr(right).                   { r = SpkParser_NewExpr(Spk_EXPR_BINARY, Spk_OPER_ADD, 0, left, right); }
binary_expr(r) ::= binary_expr(left) MINUS binary_expr(right).                  { r = SpkParser_NewExpr(Spk_EXPR_BINARY, Spk_OPER_SUB, 0, left, right); }
binary_expr(r) ::= binary_expr(left) TIMES binary_expr(right).                  { r = SpkParser_NewExpr(Spk_EXPR_BINARY, Spk_OPER_MUL, 0, left, right); }
binary_expr(r) ::= binary_expr(left) DIVIDE binary_expr(right).                 { r = SpkParser_NewExpr(Spk_EXPR_BINARY, Spk_OPER_DIV, 0, left, right); }
binary_expr(r) ::= binary_expr(left) MOD binary_expr(right).                    { r = SpkParser_NewExpr(Spk_EXPR_BINARY, Spk_OPER_MOD, 0, left, right); }
binary_expr(r) ::= binary_expr(left) DOT_STAR binary_expr(right).               { r = SpkParser_NewExpr(Spk_EXPR_ATTR_VAR, 0, 0, left, right); }

%type unary_expr {SpkExpr *}
unary_expr(r) ::= postfix_expr(expr).                                           { r = expr; }
unary_expr(r) ::= INC   unary_expr(expr).                                       { r = SpkParser_NewExpr(Spk_EXPR_PREOP, Spk_OPER_SUCC, 0, expr, 0); }
unary_expr(r) ::= DEC   unary_expr(expr).                                       { r = SpkParser_NewExpr(Spk_EXPR_PREOP, Spk_OPER_PRED, 0, expr, 0); }
unary_expr(r) ::= PLUS  unary_expr(expr).                                       { r = SpkParser_NewExpr(Spk_EXPR_UNARY, Spk_OPER_POS,  0, expr, 0); }
unary_expr(r) ::= MINUS unary_expr(expr).                                       { r = SpkParser_NewExpr(Spk_EXPR_UNARY, Spk_OPER_NEG,  0, expr, 0); }
unary_expr(r) ::= BNEG  unary_expr(expr).                                       { r = SpkParser_NewExpr(Spk_EXPR_UNARY, Spk_OPER_BNEG, 0, expr, 0); }
unary_expr(r) ::= LNEG  unary_expr(expr).                                       { r = SpkParser_NewExpr(Spk_EXPR_UNARY, Spk_OPER_LNEG, 0, expr, 0); }

%type postfix_expr {SpkExpr *}
postfix_expr(r) ::= primary_expr(expr).                                         { r = expr; }
postfix_expr(r) ::= postfix_expr(func) LBRACK argument_list(args) RBRACK.       { r = SpkParser_NewExpr(Spk_EXPR_CALL, Spk_OPER_INDEX, 0, func, args.fixed); r->var = args.var; }
postfix_expr(r) ::= postfix_expr(func) LPAREN RPAREN.                           { r = SpkParser_NewExpr(Spk_EXPR_CALL, Spk_OPER_APPLY, 0, func, 0); }
postfix_expr(r) ::= postfix_expr(func) LPAREN argument_list(args) RPAREN.       { r = SpkParser_NewExpr(Spk_EXPR_CALL, Spk_OPER_APPLY, 0, func, args.fixed); r->var = args.var; }
postfix_expr(r) ::= postfix_expr(obj) DOT IDENTIFIER(attr).                     { r = SpkParser_NewExpr(Spk_EXPR_ATTR, 0, 0, obj, 0); r->sym = attr.sym; }
postfix_expr(r) ::= postfix_expr(obj) DOT TYPE_IDENTIFIER(attr).                { r = SpkParser_NewExpr(Spk_EXPR_ATTR, 0, 0, obj, 0); r->sym = attr.sym; }
postfix_expr(r) ::= postfix_expr(obj) DOT CLASS(attr).                          { r = SpkParser_NewExpr(Spk_EXPR_ATTR, 0, 0, obj, 0); r->sym = attr.sym; }
postfix_expr(r) ::= TYPE_IDENTIFIER(name) DOT IDENTIFIER(attr).                 { r = SpkParser_NewClassAttrExpr(name.sym, attr.sym); }
postfix_expr(r) ::= TYPE_IDENTIFIER(name) DOT CLASS(attr).                      { r = SpkParser_NewClassAttrExpr(name.sym, attr.sym); }
postfix_expr(r) ::= postfix_expr(expr) INC.                                     { r = SpkParser_NewExpr(Spk_EXPR_POSTOP, Spk_OPER_SUCC, 0, expr, 0); }
postfix_expr(r) ::= postfix_expr(expr) DEC.                                     { r = SpkParser_NewExpr(Spk_EXPR_POSTOP, Spk_OPER_PRED, 0, expr, 0); }

%type primary_expr {SpkExpr *}
primary_expr(r) ::= IDENTIFIER(token).                                          { r = SpkParser_NewExpr(Spk_EXPR_NAME, 0, 0, 0, 0); r->sym = token.sym; }
primary_expr(r) ::= SYMBOL(token).                                              { r = SpkParser_NewExpr(Spk_EXPR_SYMBOL, 0, 0, 0, 0); r->sym = token.sym; }
primary_expr(r) ::= INT(token).                                                 { r = SpkParser_NewExpr(Spk_EXPR_LITERAL, 0, 0, 0, 0); r->aux.literalValue = token.literalValue; }
primary_expr(r) ::= FLOAT(token).                                               { r = SpkParser_NewExpr(Spk_EXPR_LITERAL, 0, 0, 0, 0); r->aux.literalValue = token.literalValue; }
primary_expr(r) ::= CHAR(token).                                                { r = SpkParser_NewExpr(Spk_EXPR_LITERAL, 0, 0, 0, 0); r->aux.literalValue = token.literalValue; }
primary_expr(r) ::= literal_string(expr).                                       { r = expr; }
primary_expr(r) ::= LPAREN expr(expr) RPAREN.                                   { r = expr.first; }
primary_expr(r) ::= LPAREN TYPE_IDENTIFIER(name) RPAREN.                        { r = SpkParser_NewExpr(Spk_EXPR_NAME, 0, 0, 0, 0); r->sym = name.sym; }
primary_expr(r) ::= block(expr).                                                { r = expr; }
primary_expr(r) ::= LCURLY expr(expr) RCURLY.                                   { r = SpkParser_NewExpr(Spk_EXPR_COMPOUND, 0, 0, expr.first, 0); }

%type literal_string {SpkExpr *}
literal_string(r) ::= STR(token).                                               { r = SpkParser_NewExpr(Spk_EXPR_LITERAL, 0, 0, 0, 0); r->aux.literalValue = token.literalValue; }
literal_string(r) ::= literal_string(expr) STR(token).                          { r = expr; SpkHost_StringConcat(&r->aux.literalValue, token.literalValue); }

%type block {SpkExpr *}
block(r) ::= LBRACK statement_list(stmtList) expr(expr) RBRACK.                 { r = SpkParser_NewBlock(stmtList.first, expr.first); } 
block(r) ::= LBRACK statement_list(stmtList) RBRACK.                            { r = SpkParser_NewBlock(stmtList.first, 0); } 
block(r) ::= LBRACK expr(expr) RBRACK.                                          { r = SpkParser_NewBlock(0, expr.first); } 

%type argument_list {SpkArgList}
argument_list(r) ::= expr_list(f).                                              { r.fixed = f.first; r.var = 0; }
argument_list(r) ::= expr_list(f) COMMA ELLIPSIS assignment_expr(v).            { r.fixed = f.first; r.var = v; }
argument_list(r) ::= ELLIPSIS assignment_expr(v).                               { r.fixed = 0;       r.var = v; }

%type expr_list {SpkExprList}
expr_list(r) ::= colon_expr(arg).                                               { r.first = arg; r.last = arg; }
expr_list(r) ::= expr_list(args) COMMA colon_expr(arg).                         { r = args; r.last->nextArg = arg; r.last = arg; }

%syntax_error {
    printf("syntax error! token %d, line %u\n", TOKEN.id, TOKEN.lineNo);
    parserState->error = 1;
}

%parse_accept {
    /*printf("parsing complete!\n");*/
}

%parse_failure {
    fprintf(stderr,"Giving up.  Parser is hopelessly lost...\n");
}
