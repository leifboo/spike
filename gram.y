
%name Parser_Parse
%token_type {Token}
%token_prefix TOKEN_
%extra_argument { Parser *parser }

%include {
    #include <assert.h>
    #include "lexer.h"
    #include "parser.h"
    #include "st.h"
}

start ::= statement_list(stmtList).                                             { parser->root = stmtList.first; }

%type statement_list {StmtList}
statement_list(r) ::= statement(stmt).                                          { r.first = stmt; r.last = stmt; r.expr = 0; }
statement_list(r) ::= statement_list(stmtList) statement(stmt).                 { r = stmtList; Parser_SetNextStmt(r.last, stmt, parser); r.last = stmt; r.expr = 0; }

%type statement_list_expr {StmtList}
statement_list_expr(r) ::= statement_list(stmtList) expr(e).                    { r = stmtList; r.expr = e.first; } 
statement_list_expr(r) ::= statement_list(stmtList).                            { r = stmtList; } 
statement_list_expr(r) ::= expr(e).                                             { r.first = 0; r.last = 0; r.expr = e.first; } 

%type statement {Stmt *}
statement(r) ::= open_statement(stmt).                                          { r = stmt; }
statement(r) ::= closed_statement(stmt).                                        { r = stmt; }
//statement(r) ::= IDENTIFIER COLON statement(stmt).                              { r = stmt; }

%type open_statement {Stmt *}
open_statement(r) ::= IF LPAREN expr(expr) RPAREN statement(ifTrue).            { r = Parser_NewStmt(STMT_IF_ELSE, expr.first, ifTrue, 0, parser); }
open_statement(r) ::= IF LPAREN expr(expr) RPAREN closed_statement(ifTrue)
                      ELSE open_statement(ifFalse).                             { r = Parser_NewStmt(STMT_IF_ELSE, expr.first, ifTrue, ifFalse, parser); }
open_statement(r) ::= WHILE LPAREN expr(expr) RPAREN statement(body).           { r = Parser_NewStmt(STMT_WHILE, expr.first, body, 0, parser); }
open_statement(r) ::= FOR LPAREN             SEMI             SEMI             RPAREN statement(body).
                                                                                { r = Parser_NewForStmt(          0,           0,           0, body, parser); }
open_statement(r) ::= FOR LPAREN             SEMI             SEMI expr(expr3) RPAREN statement(body).
                                                                                { r = Parser_NewForStmt(          0,           0, expr3.first, body, parser); }
open_statement(r) ::= FOR LPAREN             SEMI expr(expr2) SEMI             RPAREN statement(body).
                                                                                { r = Parser_NewForStmt(          0, expr2.first,           0, body, parser); }
open_statement(r) ::= FOR LPAREN             SEMI expr(expr2) SEMI expr(expr3) RPAREN statement(body).
                                                                                { r = Parser_NewForStmt(          0, expr2.first, expr3.first, body, parser); }
open_statement(r) ::= FOR LPAREN expr(expr1) SEMI             SEMI             RPAREN statement(body).
                                                                                { r = Parser_NewForStmt(expr1.first,           0,           0, body, parser); }
open_statement(r) ::= FOR LPAREN expr(expr1) SEMI             SEMI expr(expr3) RPAREN statement(body).
                                                                                { r = Parser_NewForStmt(expr1.first,           0, expr3.first, body, parser); }
open_statement(r) ::= FOR LPAREN expr(expr1) SEMI expr(expr2) SEMI             RPAREN statement(body).
                                                                                { r = Parser_NewForStmt(expr1.first, expr2.first,           0, body, parser); }
open_statement(r) ::= FOR LPAREN expr(expr1) SEMI expr(expr2) SEMI expr(expr3) RPAREN statement(body).
                                                                                { r = Parser_NewForStmt(expr1.first, expr2.first, expr3.first, body, parser); }

%type closed_statement {Stmt *}
closed_statement(r) ::= IF LPAREN expr(expr) RPAREN closed_statement(ifTrue)
                        ELSE closed_statement(ifFalse).                         { r = Parser_NewStmt(STMT_IF_ELSE, expr.first, ifTrue, ifFalse, parser); }
closed_statement(r) ::= SEMI.                                                   { r = Parser_NewStmt(STMT_EXPR, 0, 0, 0, parser); }
closed_statement(r) ::= expr(expr) SEMI.                                        { r = Parser_NewStmt(STMT_EXPR, expr.first, 0, 0, parser); }
closed_statement(r) ::= compound_statement(stmt).                               { r = stmt; }
closed_statement(r) ::= DO statement(body) WHILE LPAREN expr(expr) RPAREN SEMI. { r = Parser_NewStmt(STMT_DO_WHILE, expr.first, body, 0, parser); }
closed_statement(r) ::= CONTINUE SEMI.                                          { r = Parser_NewStmt(STMT_CONTINUE, 0, 0, 0, parser); }
closed_statement(r) ::= BREAK SEMI.                                             { r = Parser_NewStmt(STMT_BREAK, 0, 0, 0, parser); }
closed_statement(r) ::= RETURN            SEMI.                                 { r = Parser_NewStmt(STMT_RETURN, 0, 0, 0, parser); }
closed_statement(r) ::= RETURN expr(expr) SEMI.                                 { r = Parser_NewStmt(STMT_RETURN, expr.first, 0, 0, parser); }
closed_statement(r) ::= YIELD            SEMI.                                  { r = Parser_NewStmt(STMT_YIELD, 0, 0, 0, parser); }
closed_statement(r) ::= YIELD expr(expr) SEMI.                                  { r = Parser_NewStmt(STMT_YIELD, expr.first, 0, 0, parser); }
closed_statement(r) ::= expr(expr) compound_statement(stmt).                    { r = Parser_NewStmt(STMT_DEF_METHOD, expr.first, stmt, 0, parser); }
closed_statement(r) ::= CLASS IDENTIFIER(name) class_body(body).                { r = Parser_NewClassDef(&name, 0, body.top, body.bottom, parser); }
closed_statement(r) ::= CLASS SPECIFIER(name) class_body(body).                 { r = Parser_NewClassDef(&name, 0, body.top, body.bottom, parser); }
closed_statement(r) ::= CLASS IDENTIFIER(name) COLON IDENTIFIER(super) class_body(body).
                                                                                { r = Parser_NewClassDef(&name, &super, body.top, body.bottom, parser); }
closed_statement(r) ::= CLASS IDENTIFIER(name) COLON SPECIFIER(super) class_body(body).
                                                                                { r = Parser_NewClassDef(&name, &super, body.top, body.bottom, parser); }
closed_statement(r) ::= CLASS SPECIFIER(name) COLON IDENTIFIER(super) class_body(body).
                                                                                { r = Parser_NewClassDef(&name, &super, body.top, body.bottom, parser); }
closed_statement(r) ::= CLASS SPECIFIER(name) COLON SPECIFIER(super) class_body(body).
                                                                                { r = Parser_NewClassDef(&name, &super, body.top, body.bottom, parser); }

%type class_body {StmtPair}
class_body(r) ::= compound_statement(body).                                     { r.top = body; r.bottom = 0; }
class_body(r) ::= compound_statement(body) META compound_statement(metaBody).   { r.top = body; r.bottom = metaBody; } 

%type compound_statement {Stmt *}
compound_statement(r) ::= LCURLY                          RCURLY.               { r = Parser_NewStmt(STMT_COMPOUND, 0, 0, 0, parser); }
compound_statement(r) ::= LCURLY statement_list(stmtList) RCURLY.               { r = Parser_NewStmt(STMT_COMPOUND, 0, stmtList.first, 0, parser); }

%type expr {ExprList}
expr(r) ::= comma_expr(expr).                                                   { r = expr; }
expr(r) ::= decl_spec_list(declSpecList) comma_expr(expr).                      { r = expr; Parser_SetDeclSpecs(r.first, declSpecList.first, parser); }

%type decl_spec_list {ExprList}
decl_spec_list(r) ::= decl_spec(declSpec).                                      { r.first = declSpec; r.last = declSpec; }
decl_spec_list(r) ::= decl_spec_list(declSpecList) decl_spec(declSpec).         { r = declSpecList; Parser_SetNextExpr(r.last, declSpec, parser); r.last = declSpec; }

%type decl_spec {Expr *}
decl_spec(r) ::= SPECIFIER(name).                                               { r = Parser_NewExpr(EXPR_NAME, 0, 0, 0, 0, &name, parser); }

%type comma_expr {ExprList}
comma_expr(r) ::= colon_expr(expr).                                             { r.first = expr; r.last = expr; }
comma_expr(r) ::= comma_expr(left) COMMA colon_expr(right).                     { r = left; Parser_SetNextExpr(r.last, right, parser); r.last = right; }

%type colon_expr {Expr *}
colon_expr(r) ::= assignment_expr(expr).                                        { r = expr; }
//colon_expr(r) ::= colon_expr COLON assignment_expr(expr).                       { r = expr; }

%type assignment_expr {Expr *}
assignment_expr(r) ::= keyword_expr(expr).                                      { r = expr; }
assignment_expr(r) ::= unary_expr(left) ASSIGN(t)        assignment_expr(right).
                                                                                { r = Parser_NewExpr(EXPR_ASSIGN, OPER_EQ,     0, left, right, &t, parser); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_TIMES(t)  assignment_expr(right).
                                                                                { r = Parser_NewExpr(EXPR_ASSIGN, OPER_MUL,    0, left, right, &t, parser); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_DIVIDE(t) assignment_expr(right).
                                                                                { r = Parser_NewExpr(EXPR_ASSIGN, OPER_DIV,    0, left, right, &t, parser); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_MOD(t)    assignment_expr(right).
                                                                                { r = Parser_NewExpr(EXPR_ASSIGN, OPER_MOD,    0, left, right, &t, parser); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_PLUS(t)   assignment_expr(right).
                                                                                { r = Parser_NewExpr(EXPR_ASSIGN, OPER_ADD,    0, left, right, &t, parser); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_MINUS(t)  assignment_expr(right).
                                                                                { r = Parser_NewExpr(EXPR_ASSIGN, OPER_SUB,    0, left, right, &t, parser); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_LSHIFT(t) assignment_expr(right).
                                                                                { r = Parser_NewExpr(EXPR_ASSIGN, OPER_LSHIFT, 0, left, right, &t, parser); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_RSHIFT(t) assignment_expr(right).
                                                                                { r = Parser_NewExpr(EXPR_ASSIGN, OPER_RSHIFT, 0, left, right, &t, parser); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_BAND(t)   assignment_expr(right).
                                                                                { r = Parser_NewExpr(EXPR_ASSIGN, OPER_BAND,   0, left, right, &t, parser); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_BXOR(t)   assignment_expr(right).
                                                                                { r = Parser_NewExpr(EXPR_ASSIGN, OPER_BXOR,   0, left, right, &t, parser); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_BOR(t)    assignment_expr(right).
                                                                                { r = Parser_NewExpr(EXPR_ASSIGN, OPER_BOR,    0, left, right, &t, parser); }

%type keyword_expr {Expr *}
keyword_expr(r) ::= conditional_expr(expr).                                     { r = expr; }
keyword_expr(r) ::= conditional_expr(receiver) keyword_message(expr).           { r = expr; Parser_SetLeftExpr(r, receiver, parser); }

%type keyword_message {Expr *}
keyword_message(r) ::= keyword_list(expr).                                      { r = Parser_FreezeKeywords(expr, 0, parser); }
keyword_message(r) ::= keyword(kw).                                             { r = Parser_FreezeKeywords(0, &kw, parser); }
keyword_message(r) ::= keyword(kw) keyword_list(expr).                          { r = Parser_FreezeKeywords(expr, &kw, parser); }

%type keyword_list {Expr *}
keyword_list(r) ::= keyword(kw) COLON conditional_expr(arg).                    { r = Parser_NewKeywordExpr(&kw, arg, parser); }
keyword_list(r) ::= keyword_list(expr) keyword(kw) COLON conditional_expr(arg). { r = Parser_AppendKeyword(expr, &kw, arg, parser); }

%type keyword {Token}
keyword(r) ::= IDENTIFIER(t).                                                   { r = t; }
keyword(r) ::= BREAK(t).                                                        { r = t; }
keyword(r) ::= CLASS(t).                                                        { r = t; }
keyword(r) ::= CONTINUE(t).                                                     { r = t; }
keyword(r) ::= DO(t).                                                           { r = t; }
keyword(r) ::= ELSE(t).                                                         { r = t; }
keyword(r) ::= FOR(t).                                                          { r = t; }
keyword(r) ::= IF(t).                                                           { r = t; }
keyword(r) ::= META(t).                                                         { r = t; }
keyword(r) ::= RETURN(t).                                                       { r = t; }
keyword(r) ::= WHILE(t).                                                        { r = t; }
keyword(r) ::= YIELD(t).                                                        { r = t; }

%type conditional_expr {Expr *}
conditional_expr(r) ::= logical_expr(expr).                                     { r = expr; }
conditional_expr(r) ::= logical_expr(cond) QM(t) logical_expr(left) COLON conditional_expr(right).
                                                                                { r = Parser_NewExpr(EXPR_COND, 0, cond, left, right, &t, parser); }

%left OR.
%left AND.

%type logical_expr {Expr *}
logical_expr(r) ::= binary_expr(expr).                                          { r = expr; }
logical_expr(r) ::= logical_expr(left) OR(t) logical_expr(right).               { r = Parser_NewExpr(EXPR_OR, 0, 0, left, right, &t, parser); }
logical_expr(r) ::= logical_expr(left) AND(t) logical_expr(right).              { r = Parser_NewExpr(EXPR_AND, 0, 0, left, right, &t, parser); }

%left ID NI EQ NE.
%left GT GE LT LE.
%left BOR.
%left BXOR.
%left AMP.
%left LSHIFT RSHIFT.
%left PLUS MINUS.
%left TIMES DIVIDE MOD.
%left DOT_STAR.

%type binary_expr {Expr *}
binary_expr(r) ::= unary_expr(expr).                                            { r = expr; }
binary_expr(r) ::= binary_expr(left) ID(t) binary_expr(right).                  { r = Parser_NewExpr(EXPR_ID, 0, 0, left, right, &t, parser); }
binary_expr(r) ::= binary_expr(left) NI(t) binary_expr(right).                  { r = Parser_NewExpr(EXPR_NI, 0, 0, left, right, &t, parser); }
binary_expr(r) ::= binary_expr(left) EQ(t) binary_expr(right).                  { r = Parser_NewExpr(EXPR_BINARY, OPER_EQ, 0, left, right, &t, parser); }
binary_expr(r) ::= binary_expr(left) NE(t) binary_expr(right).                  { r = Parser_NewExpr(EXPR_BINARY, OPER_NE, 0, left, right, &t, parser); }
binary_expr(r) ::= binary_expr(left) GT(t) binary_expr(right).                  { r = Parser_NewExpr(EXPR_BINARY, OPER_GT, 0, left, right, &t, parser); }
binary_expr(r) ::= binary_expr(left) GE(t) binary_expr(right).                  { r = Parser_NewExpr(EXPR_BINARY, OPER_GE, 0, left, right, &t, parser); }
binary_expr(r) ::= binary_expr(left) LT(t) binary_expr(right).                  { r = Parser_NewExpr(EXPR_BINARY, OPER_LT, 0, left, right, &t, parser); }
binary_expr(r) ::= binary_expr(left) LE(t) binary_expr(right).                  { r = Parser_NewExpr(EXPR_BINARY, OPER_LE, 0, left, right, &t, parser); }
binary_expr(r) ::= binary_expr(left) BOR(t) binary_expr(right).                 { r = Parser_NewExpr(EXPR_BINARY, OPER_BOR, 0, left, right, &t, parser); }
binary_expr(r) ::= binary_expr(left) BXOR(t) binary_expr(right).                { r = Parser_NewExpr(EXPR_BINARY, OPER_BXOR, 0, left, right, &t, parser); }
binary_expr(r) ::= binary_expr(left) AMP(t) binary_expr(right).                 { r = Parser_NewExpr(EXPR_BINARY, OPER_BAND, 0, left, right, &t, parser); }
binary_expr(r) ::= binary_expr(left) LSHIFT(t) binary_expr(right).              { r = Parser_NewExpr(EXPR_BINARY, OPER_LSHIFT, 0, left, right, &t, parser); }
binary_expr(r) ::= binary_expr(left) RSHIFT(t) binary_expr(right).              { r = Parser_NewExpr(EXPR_BINARY, OPER_RSHIFT, 0, left, right, &t, parser); }
binary_expr(r) ::= binary_expr(left) PLUS(t) binary_expr(right).                { r = Parser_NewExpr(EXPR_BINARY, OPER_ADD, 0, left, right, &t, parser); }
binary_expr(r) ::= binary_expr(left) MINUS(t) binary_expr(right).               { r = Parser_NewExpr(EXPR_BINARY, OPER_SUB, 0, left, right, &t, parser); }
binary_expr(r) ::= binary_expr(left) TIMES(t) binary_expr(right).               { r = Parser_NewExpr(EXPR_BINARY, OPER_MUL, 0, left, right, &t, parser); }
binary_expr(r) ::= binary_expr(left) DIVIDE(t) binary_expr(right).              { r = Parser_NewExpr(EXPR_BINARY, OPER_DIV, 0, left, right, &t, parser); }
binary_expr(r) ::= binary_expr(left) MOD(t) binary_expr(right).                 { r = Parser_NewExpr(EXPR_BINARY, OPER_MOD, 0, left, right, &t, parser); }
binary_expr(r) ::= binary_expr(left) DOT_STAR(t) binary_expr(right).            { r = Parser_NewExpr(EXPR_ATTR_VAR, 0, 0, left, right, &t, parser); }

%type unary_expr {Expr *}
unary_expr(r) ::= postfix_expr(expr).                                           { r = expr; }
unary_expr(r) ::= INC(t)   unary_expr(expr).                                    { r = Parser_NewExpr(EXPR_PREOP, OPER_SUCC, 0, expr, 0, &t, parser); }
unary_expr(r) ::= DEC(t)   unary_expr(expr).                                    { r = Parser_NewExpr(EXPR_PREOP, OPER_PRED, 0, expr, 0, &t, parser); }
unary_expr(r) ::= PLUS(t)  unary_expr(expr).                                    { r = Parser_NewExpr(EXPR_UNARY, OPER_POS,  0, expr, 0, &t, parser); }
unary_expr(r) ::= MINUS(t) unary_expr(expr).                                    { r = Parser_NewExpr(EXPR_UNARY, OPER_NEG,  0, expr, 0, &t, parser); }
unary_expr(r) ::= TIMES(t) unary_expr(expr).                                    { r = Parser_NewExpr(EXPR_UNARY, OPER_IND,  0, expr, 0, &t, parser); }
unary_expr(r) ::= BNEG(t)  unary_expr(expr).                                    { r = Parser_NewExpr(EXPR_UNARY, OPER_BNEG, 0, expr, 0, &t, parser); }
unary_expr(r) ::= LNEG(t)  unary_expr(expr).                                    { r = Parser_NewExpr(EXPR_UNARY, OPER_LNEG, 0, expr, 0, &t, parser); }

%type postfix_expr {Expr *}
postfix_expr(r) ::= primary_expr(expr).                                         { r = expr; }
postfix_expr(r) ::= postfix_expr(func) LBRACK(t) argument_list(args) RBRACK.    { r = Parser_NewCallExpr(OPER_INDEX, func, &args, &t, parser); }
postfix_expr(r) ::= postfix_expr(func) LPAREN(t) RPAREN.                        { r = Parser_NewCallExpr(OPER_APPLY, func, 0,     &t, parser); }
postfix_expr(r) ::= postfix_expr(func) LPAREN(t) argument_list(args) RPAREN.    { r = Parser_NewCallExpr(OPER_APPLY, func, &args, &t, parser); }
postfix_expr(r) ::= postfix_expr(obj) DOT(t) IDENTIFIER(attr).                  { r = Parser_NewAttrExpr(obj, &attr, &t, parser); }
postfix_expr(r) ::= postfix_expr(obj) DOT(t) SPECIFIER(attr).                   { r = Parser_NewAttrExpr(obj, &attr, &t, parser); }
postfix_expr(r) ::= postfix_expr(obj) DOT(t) CLASS(attr).                       { r = Parser_NewAttrExpr(obj, &attr, &t, parser); }
postfix_expr(r) ::= SPECIFIER(name) DOT(t) IDENTIFIER(attr).                    { r = Parser_NewClassAttrExpr(&name, &attr, &t, parser); }
postfix_expr(r) ::= SPECIFIER(name) DOT(t) CLASS(attr).                         { r = Parser_NewClassAttrExpr(&name, &attr, &t, parser); }
postfix_expr(r) ::= postfix_expr(expr) INC(t).                                  { r = Parser_NewExpr(EXPR_POSTOP, OPER_SUCC, 0, expr, 0, &t, parser); }
postfix_expr(r) ::= postfix_expr(expr) DEC(t).                                  { r = Parser_NewExpr(EXPR_POSTOP, OPER_PRED, 0, expr, 0, &t, parser); }

%type primary_expr {Expr *}
primary_expr(r) ::= IDENTIFIER(name).                                           { r = Parser_NewExpr(EXPR_NAME, 0, 0, 0, 0, &name, parser); }
primary_expr(r) ::= LITERAL_SYMBOL(t).                                          { r = Parser_NewExpr(EXPR_LITERAL, 0, 0, 0, 0, &t, parser); }
primary_expr(r) ::= LITERAL_INT(t).                                             { r = Parser_NewExpr(EXPR_LITERAL, 0, 0, 0, 0, &t, parser); }
primary_expr(r) ::= LITERAL_FLOAT(t).                                           { r = Parser_NewExpr(EXPR_LITERAL, 0, 0, 0, 0, &t, parser); }
primary_expr(r) ::= LITERAL_CHAR(t).                                            { r = Parser_NewExpr(EXPR_LITERAL, 0, 0, 0, 0, &t, parser); }
primary_expr(r) ::= literal_string(expr).                                       { r = expr; }
primary_expr(r) ::= LPAREN expr(expr) RPAREN.                                   { r = expr.first; }
primary_expr(r) ::= LPAREN SPECIFIER(name) RPAREN.                              { r = Parser_NewExpr(EXPR_NAME, 0, 0, 0, 0, &name, parser); }
primary_expr(r) ::= block(expr).                                                { r = expr; }
//primary_expr(r) ::= LCURLY(t) expr(expr) RCURLY.                                { r = Parser_NewCompoundExpr(expr.first, &t, parser); }

%type literal_string {Expr *}
literal_string(r) ::= LITERAL_STR(t).                                           { r = Parser_NewExpr(EXPR_LITERAL, 0, 0, 0, 0, &t, parser); }
literal_string(r) ::= literal_string(expr) LITERAL_STR(t).                      { r = expr; Parser_Concat(r, &t, parser); }

%type block {Expr *}
block(r) ::= LBRACK(t) statement_list_expr(sle) RBRACK.                         { r = Parser_NewBlock(0,          sle.first, sle.expr, &t, parser); }
block(r) ::= LBRACK(t) block_argument_list(args) BOR statement_list_expr(sle) RBRACK.
                                                                                { r = Parser_NewBlock(args.first, sle.first, sle.expr, &t, parser); }

%type block_argument_list {ExprList}
block_argument_list(r) ::= COLON block_arg(arg).                                { r.first = arg; r.last = arg; }
block_argument_list(r) ::= block_argument_list(args) COLON block_arg(arg).      { r = args; Parser_SetNextArg(r.last, arg, parser); r.last = arg; }

%type block_arg {Expr *}
block_arg(r) ::= unary_expr(arg).                                               { r = arg; }
block_arg(r) ::= decl_spec_list(declSpecList) unary_expr(arg).                  { r = arg; Parser_SetDeclSpecs(arg, declSpecList.first, parser); }

%type argument_list {ArgList}
argument_list(r) ::= fixed_arg_list(f).                                         { r.fixed = f.first; r.var = 0; }
argument_list(r) ::= fixed_arg_list(f) COMMA ELLIPSIS assignment_expr(v).       { r.fixed = f.first; r.var = v; }
argument_list(r) ::= ELLIPSIS assignment_expr(v).                               { r.fixed = 0;       r.var = v; }

%type fixed_arg_list {ExprList}
fixed_arg_list(r) ::= arg(arg).                                                 { r.first = arg; r.last = arg; }
fixed_arg_list(r) ::= fixed_arg_list(args) COMMA arg(arg).                      { r = args; Parser_SetNextArg(r.last, arg, parser); r.last = arg; }

%type arg {Expr *}
arg(r) ::= colon_expr(arg).                                                     { r = arg; }
arg(r) ::= decl_spec_list(declSpecList) colon_expr(arg).                        { r = arg; Parser_SetDeclSpecs(arg, declSpecList.first, parser); }


%syntax_error {
    fprintf(stderr, "syntax error! token %d, line %u\n", TOKEN.id, TOKEN.lineNo);
    parser->error = 1;
}

%parse_accept {
    /*printf("parsing complete!\n");*/
}

%parse_failure {
    fprintf(stderr,"Giving up.  Parser is hopelessly lost...\n");
}
