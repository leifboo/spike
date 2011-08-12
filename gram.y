
%name SpkParser_Parse
%token_type {SpkToken}
%token_prefix Spk_TOKEN_
%extra_argument { SpkParser *parser }

%include {
    #include <assert.h>
    #include "host.h"
    #include "lexer.h"
    #include "parser.h"
    #include "st.h"
}

start ::= statement_list(stmtList).                                             { parser->root = stmtList.first; }

%type statement_list {SpkStmtList}
statement_list(r) ::= statement(stmt).                                          { r.first = stmt; r.last = stmt; }
statement_list(r) ::= statement_list(stmtList) statement(stmt).                 { r = stmtList; SpkParser_SetNextStmt(r.last, stmt, parser); r.last = stmt; }

%type statement {SpkStmt *}
statement(r) ::= open_statement(stmt).                                          { r = stmt; }
statement(r) ::= closed_statement(stmt).                                        { r = stmt; }
//statement(r) ::= IDENTIFIER COLON statement(stmt).                              { r = stmt; }

%type open_statement {SpkStmt *}
open_statement(r) ::= IF LPAREN expr(expr) RPAREN statement(ifTrue).            { r = SpkParser_NewStmt(Spk_STMT_IF_ELSE, expr.first, ifTrue, 0, parser); }
open_statement(r) ::= IF LPAREN expr(expr) RPAREN closed_statement(ifTrue)
                      ELSE open_statement(ifFalse).                             { r = SpkParser_NewStmt(Spk_STMT_IF_ELSE, expr.first, ifTrue, ifFalse, parser); }
open_statement(r) ::= WHILE LPAREN expr(expr) RPAREN statement(body).           { r = SpkParser_NewStmt(Spk_STMT_WHILE, expr.first, body, 0, parser); }
open_statement(r) ::= FOR LPAREN             SEMI             SEMI             RPAREN statement(body).
                                                                                { r = SpkParser_NewForStmt(          0,           0,           0, body, parser); }
open_statement(r) ::= FOR LPAREN             SEMI             SEMI expr(expr3) RPAREN statement(body).
                                                                                { r = SpkParser_NewForStmt(          0,           0, expr3.first, body, parser); }
open_statement(r) ::= FOR LPAREN             SEMI expr(expr2) SEMI             RPAREN statement(body).
                                                                                { r = SpkParser_NewForStmt(          0, expr2.first,           0, body, parser); }
open_statement(r) ::= FOR LPAREN             SEMI expr(expr2) SEMI expr(expr3) RPAREN statement(body).
                                                                                { r = SpkParser_NewForStmt(          0, expr2.first, expr3.first, body, parser); }
open_statement(r) ::= FOR LPAREN expr(expr1) SEMI             SEMI             RPAREN statement(body).
                                                                                { r = SpkParser_NewForStmt(expr1.first,           0,           0, body, parser); }
open_statement(r) ::= FOR LPAREN expr(expr1) SEMI             SEMI expr(expr3) RPAREN statement(body).
                                                                                { r = SpkParser_NewForStmt(expr1.first,           0, expr3.first, body, parser); }
open_statement(r) ::= FOR LPAREN expr(expr1) SEMI expr(expr2) SEMI             RPAREN statement(body).
                                                                                { r = SpkParser_NewForStmt(expr1.first, expr2.first,           0, body, parser); }
open_statement(r) ::= FOR LPAREN expr(expr1) SEMI expr(expr2) SEMI expr(expr3) RPAREN statement(body).
                                                                                { r = SpkParser_NewForStmt(expr1.first, expr2.first, expr3.first, body, parser); }

%type closed_statement {SpkStmt *}
closed_statement(r) ::= IF LPAREN expr(expr) RPAREN closed_statement(ifTrue)
                        ELSE closed_statement(ifFalse).                         { r = SpkParser_NewStmt(Spk_STMT_IF_ELSE, expr.first, ifTrue, ifFalse, parser); }
closed_statement(r) ::= SEMI.                                                   { r = SpkParser_NewStmt(Spk_STMT_EXPR, 0, 0, 0, parser); }
closed_statement(r) ::= expr(expr) SEMI.                                        { r = SpkParser_NewStmt(Spk_STMT_EXPR, expr.first, 0, 0, parser); }
closed_statement(r) ::= compound_statement(stmt).                               { r = stmt; }
closed_statement(r) ::= DO statement(body) WHILE LPAREN expr(expr) RPAREN SEMI. { r = SpkParser_NewStmt(Spk_STMT_DO_WHILE, expr.first, body, 0, parser); }
closed_statement(r) ::= CONTINUE SEMI.                                          { r = SpkParser_NewStmt(Spk_STMT_CONTINUE, 0, 0, 0, parser); }
closed_statement(r) ::= BREAK SEMI.                                             { r = SpkParser_NewStmt(Spk_STMT_BREAK, 0, 0, 0, parser); }
closed_statement(r) ::= RETURN            SEMI.                                 { r = SpkParser_NewStmt(Spk_STMT_RETURN, 0, 0, 0, parser); }
closed_statement(r) ::= RETURN expr(expr) SEMI.                                 { r = SpkParser_NewStmt(Spk_STMT_RETURN, expr.first, 0, 0, parser); }
closed_statement(r) ::= YIELD            SEMI.                                  { r = SpkParser_NewStmt(Spk_STMT_YIELD, 0, 0, 0, parser); }
closed_statement(r) ::= YIELD expr(expr) SEMI.                                  { r = SpkParser_NewStmt(Spk_STMT_YIELD, expr.first, 0, 0, parser); }
closed_statement(r) ::= expr(expr) compound_statement(stmt).                    { r = SpkParser_NewStmt(Spk_STMT_DEF_METHOD, expr.first, stmt, 0, parser); }
closed_statement(r) ::= CLASS IDENTIFIER(name) class_body(body).                { r = SpkParser_NewClassDef(&name, 0, body.top, body.bottom, parser); }
closed_statement(r) ::= CLASS SPECIFIER(name) class_body(body).                 { r = SpkParser_NewClassDef(&name, 0, body.top, body.bottom, parser); }
closed_statement(r) ::= CLASS IDENTIFIER(name) COLON IDENTIFIER(super) class_body(body).
                                                                                { r = SpkParser_NewClassDef(&name, &super, body.top, body.bottom, parser); }
closed_statement(r) ::= CLASS IDENTIFIER(name) COLON SPECIFIER(super) class_body(body).
                                                                                { r = SpkParser_NewClassDef(&name, &super, body.top, body.bottom, parser); }
closed_statement(r) ::= CLASS SPECIFIER(name) COLON IDENTIFIER(super) class_body(body).
                                                                                { r = SpkParser_NewClassDef(&name, &super, body.top, body.bottom, parser); }
closed_statement(r) ::= CLASS SPECIFIER(name) COLON SPECIFIER(super) class_body(body).
                                                                                { r = SpkParser_NewClassDef(&name, &super, body.top, body.bottom, parser); }

%type class_body {SpkStmtPair}
class_body(r) ::= compound_statement(body).                                     { r.top = body; r.bottom = 0; }
class_body(r) ::= compound_statement(body) META compound_statement(metaBody).   { r.top = body; r.bottom = metaBody; } 

%type compound_statement {SpkStmt *}
compound_statement(r) ::= LCURLY                          RCURLY.               { r = SpkParser_NewStmt(Spk_STMT_COMPOUND, 0, 0, 0, parser); }
compound_statement(r) ::= LCURLY statement_list(stmtList) RCURLY.               { r = SpkParser_NewStmt(Spk_STMT_COMPOUND, 0, stmtList.first, 0, parser); }

%type expr {SpkExprList}
expr(r) ::= comma_expr(expr).                                                   { r = expr; }
expr(r) ::= decl_spec_list(declSpecList) comma_expr(expr).                      { r = expr; SpkParser_SetDeclSpecs(r.first, declSpecList.first, parser); }

%type decl_spec_list {SpkExprList}
decl_spec_list(r) ::= decl_spec(declSpec).                                      { r.first = declSpec; r.last = declSpec; }
decl_spec_list(r) ::= decl_spec_list(declSpecList) decl_spec(declSpec).         { r = declSpecList; SpkParser_SetNextExpr(r.last, declSpec, parser); r.last = declSpec; }

%type decl_spec {SpkExpr *}
decl_spec(r) ::= SPECIFIER(name).                                               { r = SpkParser_NewExpr(Spk_EXPR_NAME, 0, 0, 0, 0, &name, parser); }

%type comma_expr {SpkExprList}
comma_expr(r) ::= colon_expr(expr).                                             { r.first = expr; r.last = expr; }
comma_expr(r) ::= comma_expr(left) COMMA colon_expr(right).                     { r = left; SpkParser_SetNextExpr(r.last, right, parser); r.last = right; }

%type colon_expr {SpkExpr *}
colon_expr(r) ::= assignment_expr(expr).                                        { r = expr; }
//colon_expr(r) ::= colon_expr COLON assignment_expr(expr).                       { r = expr; }

%type assignment_expr {SpkExpr *}
assignment_expr(r) ::= keyword_expr(expr).                                      { r = expr; }
assignment_expr(r) ::= unary_expr(left) ASSIGN(t)        assignment_expr(right).
                                                                                { r = SpkParser_NewExpr(Spk_EXPR_ASSIGN, Spk_OPER_EQ,     0, left, right, &t, parser); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_TIMES(t)  assignment_expr(right).
                                                                                { r = SpkParser_NewExpr(Spk_EXPR_ASSIGN, Spk_OPER_MUL,    0, left, right, &t, parser); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_DIVIDE(t) assignment_expr(right).
                                                                                { r = SpkParser_NewExpr(Spk_EXPR_ASSIGN, Spk_OPER_DIV,    0, left, right, &t, parser); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_MOD(t)    assignment_expr(right).
                                                                                { r = SpkParser_NewExpr(Spk_EXPR_ASSIGN, Spk_OPER_MOD,    0, left, right, &t, parser); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_PLUS(t)   assignment_expr(right).
                                                                                { r = SpkParser_NewExpr(Spk_EXPR_ASSIGN, Spk_OPER_ADD,    0, left, right, &t, parser); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_MINUS(t)  assignment_expr(right).
                                                                                { r = SpkParser_NewExpr(Spk_EXPR_ASSIGN, Spk_OPER_SUB,    0, left, right, &t, parser); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_LSHIFT(t) assignment_expr(right).
                                                                                { r = SpkParser_NewExpr(Spk_EXPR_ASSIGN, Spk_OPER_LSHIFT, 0, left, right, &t, parser); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_RSHIFT(t) assignment_expr(right).
                                                                                { r = SpkParser_NewExpr(Spk_EXPR_ASSIGN, Spk_OPER_RSHIFT, 0, left, right, &t, parser); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_BAND(t)   assignment_expr(right).
                                                                                { r = SpkParser_NewExpr(Spk_EXPR_ASSIGN, Spk_OPER_BAND,   0, left, right, &t, parser); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_BXOR(t)   assignment_expr(right).
                                                                                { r = SpkParser_NewExpr(Spk_EXPR_ASSIGN, Spk_OPER_BXOR,   0, left, right, &t, parser); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_BOR(t)    assignment_expr(right).
                                                                                { r = SpkParser_NewExpr(Spk_EXPR_ASSIGN, Spk_OPER_BOR,    0, left, right, &t, parser); }

%type keyword_expr {SpkExpr *}
keyword_expr(r) ::= conditional_expr(expr).                                     { r = expr; }
keyword_expr(r) ::= conditional_expr(receiver) keyword_message(expr).           { r = expr; SpkParser_SetLeftExpr(r, receiver, parser); }

%type keyword_message {SpkExpr *}
keyword_message(r) ::= keyword_list(expr).                                      { r = SpkParser_FreezeKeywords(expr, 0, parser); }
keyword_message(r) ::= keyword(kw).                                             { r = SpkParser_FreezeKeywords(0, &kw, parser); }
keyword_message(r) ::= keyword(kw) keyword_list(expr).                          { r = SpkParser_FreezeKeywords(expr, &kw, parser); }

%type keyword_list {SpkExpr *}
keyword_list(r) ::= keyword(kw) COLON conditional_expr(arg).                    { r = SpkParser_NewKeywordExpr(&kw, arg, parser); }
keyword_list(r) ::= keyword_list(expr) keyword(kw) COLON conditional_expr(arg). { r = SpkParser_AppendKeyword(expr, &kw, arg, parser); }

%type keyword {SpkToken}
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

%type conditional_expr {SpkExpr *}
conditional_expr(r) ::= logical_expr(expr).                                     { r = expr; }
conditional_expr(r) ::= logical_expr(cond) QM(t) logical_expr(left) COLON conditional_expr(right).
                                                                                { r = SpkParser_NewExpr(Spk_EXPR_COND, 0, cond, left, right, &t, parser); }

%left OR.
%left AND.

%type logical_expr {SpkExpr *}
logical_expr(r) ::= binary_expr(expr).                                          { r = expr; }
logical_expr(r) ::= logical_expr(left) OR(t) logical_expr(right).               { r = SpkParser_NewExpr(Spk_EXPR_OR, 0, 0, left, right, &t, parser); }
logical_expr(r) ::= logical_expr(left) AND(t) logical_expr(right).              { r = SpkParser_NewExpr(Spk_EXPR_AND, 0, 0, left, right, &t, parser); }

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
binary_expr(r) ::= binary_expr(left) ID(t) binary_expr(right).                  { r = SpkParser_NewExpr(Spk_EXPR_ID, 0, 0, left, right, &t, parser); }
binary_expr(r) ::= binary_expr(left) NI(t) binary_expr(right).                  { r = SpkParser_NewExpr(Spk_EXPR_NI, 0, 0, left, right, &t, parser); }
binary_expr(r) ::= binary_expr(left) EQ(t) binary_expr(right).                  { r = SpkParser_NewExpr(Spk_EXPR_BINARY, Spk_OPER_EQ, 0, left, right, &t, parser); }
binary_expr(r) ::= binary_expr(left) NE(t) binary_expr(right).                  { r = SpkParser_NewExpr(Spk_EXPR_BINARY, Spk_OPER_NE, 0, left, right, &t, parser); }
binary_expr(r) ::= binary_expr(left) GT(t) binary_expr(right).                  { r = SpkParser_NewExpr(Spk_EXPR_BINARY, Spk_OPER_GT, 0, left, right, &t, parser); }
binary_expr(r) ::= binary_expr(left) GE(t) binary_expr(right).                  { r = SpkParser_NewExpr(Spk_EXPR_BINARY, Spk_OPER_GE, 0, left, right, &t, parser); }
binary_expr(r) ::= binary_expr(left) LT(t) binary_expr(right).                  { r = SpkParser_NewExpr(Spk_EXPR_BINARY, Spk_OPER_LT, 0, left, right, &t, parser); }
binary_expr(r) ::= binary_expr(left) LE(t) binary_expr(right).                  { r = SpkParser_NewExpr(Spk_EXPR_BINARY, Spk_OPER_LE, 0, left, right, &t, parser); }
binary_expr(r) ::= binary_expr(left) BOR(t) binary_expr(right).                 { r = SpkParser_NewExpr(Spk_EXPR_BINARY, Spk_OPER_BOR, 0, left, right, &t, parser); }
binary_expr(r) ::= binary_expr(left) BXOR(t) binary_expr(right).                { r = SpkParser_NewExpr(Spk_EXPR_BINARY, Spk_OPER_BXOR, 0, left, right, &t, parser); }
binary_expr(r) ::= binary_expr(left) AMP(t) binary_expr(right).                 { r = SpkParser_NewExpr(Spk_EXPR_BINARY, Spk_OPER_BAND, 0, left, right, &t, parser); }
binary_expr(r) ::= binary_expr(left) LSHIFT(t) binary_expr(right).              { r = SpkParser_NewExpr(Spk_EXPR_BINARY, Spk_OPER_LSHIFT, 0, left, right, &t, parser); }
binary_expr(r) ::= binary_expr(left) RSHIFT(t) binary_expr(right).              { r = SpkParser_NewExpr(Spk_EXPR_BINARY, Spk_OPER_RSHIFT, 0, left, right, &t, parser); }
binary_expr(r) ::= binary_expr(left) PLUS(t) binary_expr(right).                { r = SpkParser_NewExpr(Spk_EXPR_BINARY, Spk_OPER_ADD, 0, left, right, &t, parser); }
binary_expr(r) ::= binary_expr(left) MINUS(t) binary_expr(right).               { r = SpkParser_NewExpr(Spk_EXPR_BINARY, Spk_OPER_SUB, 0, left, right, &t, parser); }
binary_expr(r) ::= binary_expr(left) TIMES(t) binary_expr(right).               { r = SpkParser_NewExpr(Spk_EXPR_BINARY, Spk_OPER_MUL, 0, left, right, &t, parser); }
binary_expr(r) ::= binary_expr(left) DIVIDE(t) binary_expr(right).              { r = SpkParser_NewExpr(Spk_EXPR_BINARY, Spk_OPER_DIV, 0, left, right, &t, parser); }
binary_expr(r) ::= binary_expr(left) MOD(t) binary_expr(right).                 { r = SpkParser_NewExpr(Spk_EXPR_BINARY, Spk_OPER_MOD, 0, left, right, &t, parser); }
binary_expr(r) ::= binary_expr(left) DOT_STAR(t) binary_expr(right).            { r = SpkParser_NewExpr(Spk_EXPR_ATTR_VAR, 0, 0, left, right, &t, parser); }

%type unary_expr {SpkExpr *}
unary_expr(r) ::= postfix_expr(expr).                                           { r = expr; }
unary_expr(r) ::= INC(t)   unary_expr(expr).                                    { r = SpkParser_NewExpr(Spk_EXPR_PREOP, Spk_OPER_SUCC, 0, expr, 0, &t, parser); }
unary_expr(r) ::= DEC(t)   unary_expr(expr).                                    { r = SpkParser_NewExpr(Spk_EXPR_PREOP, Spk_OPER_PRED, 0, expr, 0, &t, parser); }
unary_expr(r) ::= PLUS(t)  unary_expr(expr).                                    { r = SpkParser_NewExpr(Spk_EXPR_UNARY, Spk_OPER_POS,  0, expr, 0, &t, parser); }
unary_expr(r) ::= MINUS(t) unary_expr(expr).                                    { r = SpkParser_NewExpr(Spk_EXPR_UNARY, Spk_OPER_NEG,  0, expr, 0, &t, parser); }
unary_expr(r) ::= TIMES(t) unary_expr(expr).                                    { r = SpkParser_NewExpr(Spk_EXPR_UNARY, Spk_OPER_IND,  0, expr, 0, &t, parser); }
unary_expr(r) ::= BNEG(t)  unary_expr(expr).                                    { r = SpkParser_NewExpr(Spk_EXPR_UNARY, Spk_OPER_BNEG, 0, expr, 0, &t, parser); }
unary_expr(r) ::= LNEG(t)  unary_expr(expr).                                    { r = SpkParser_NewExpr(Spk_EXPR_UNARY, Spk_OPER_LNEG, 0, expr, 0, &t, parser); }

%type postfix_expr {SpkExpr *}
postfix_expr(r) ::= primary_expr(expr).                                         { r = expr; }
postfix_expr(r) ::= postfix_expr(func) LBRACK(t) argument_list(args) RBRACK.    { r = SpkParser_NewCallExpr(Spk_OPER_INDEX, func, &args, &t, parser); }
postfix_expr(r) ::= postfix_expr(func) LPAREN(t) RPAREN.                        { r = SpkParser_NewCallExpr(Spk_OPER_APPLY, func, 0,     &t, parser); }
postfix_expr(r) ::= postfix_expr(func) LPAREN(t) argument_list(args) RPAREN.    { r = SpkParser_NewCallExpr(Spk_OPER_APPLY, func, &args, &t, parser); }
postfix_expr(r) ::= postfix_expr(obj) DOT(t) IDENTIFIER(attr).                  { r = SpkParser_NewAttrExpr(obj, &attr, &t, parser); }
postfix_expr(r) ::= postfix_expr(obj) DOT(t) SPECIFIER(attr).                   { r = SpkParser_NewAttrExpr(obj, &attr, &t, parser); }
postfix_expr(r) ::= postfix_expr(obj) DOT(t) CLASS(attr).                       { r = SpkParser_NewAttrExpr(obj, &attr, &t, parser); }
postfix_expr(r) ::= SPECIFIER(name) DOT(t) IDENTIFIER(attr).                    { r = SpkParser_NewClassAttrExpr(&name, &attr, &t, parser); }
postfix_expr(r) ::= SPECIFIER(name) DOT(t) CLASS(attr).                         { r = SpkParser_NewClassAttrExpr(&name, &attr, &t, parser); }
postfix_expr(r) ::= postfix_expr(expr) INC(t).                                  { r = SpkParser_NewExpr(Spk_EXPR_POSTOP, Spk_OPER_SUCC, 0, expr, 0, &t, parser); }
postfix_expr(r) ::= postfix_expr(expr) DEC(t).                                  { r = SpkParser_NewExpr(Spk_EXPR_POSTOP, Spk_OPER_PRED, 0, expr, 0, &t, parser); }

%type primary_expr {SpkExpr *}
primary_expr(r) ::= IDENTIFIER(name).                                           { r = SpkParser_NewExpr(Spk_EXPR_NAME, 0, 0, 0, 0, &name, parser); }
primary_expr(r) ::= LITERAL_SYMBOL(t).                                          { r = SpkParser_NewExpr(Spk_EXPR_LITERAL, 0, 0, 0, 0, &t, parser); }
primary_expr(r) ::= LITERAL_INT(t).                                             { r = SpkParser_NewExpr(Spk_EXPR_LITERAL, 0, 0, 0, 0, &t, parser); }
primary_expr(r) ::= LITERAL_FLOAT(t).                                           { r = SpkParser_NewExpr(Spk_EXPR_LITERAL, 0, 0, 0, 0, &t, parser); }
primary_expr(r) ::= LITERAL_CHAR(t).                                            { r = SpkParser_NewExpr(Spk_EXPR_LITERAL, 0, 0, 0, 0, &t, parser); }
primary_expr(r) ::= literal_string(expr).                                       { r = expr; }
primary_expr(r) ::= LPAREN expr(expr) RPAREN.                                   { r = expr.first; }
primary_expr(r) ::= LPAREN SPECIFIER(name) RPAREN.                              { r = SpkParser_NewExpr(Spk_EXPR_NAME, 0, 0, 0, 0, &name, parser); }
primary_expr(r) ::= block(expr).                                                { r = expr; }
primary_expr(r) ::= LCURLY(t) expr(expr) RCURLY.                                { r = SpkParser_NewCompoundExpr(expr.first, &t, parser); }

%type literal_string {SpkExpr *}
literal_string(r) ::= LITERAL_STR(t).                                           { r = SpkParser_NewExpr(Spk_EXPR_LITERAL, 0, 0, 0, 0, &t, parser); }
literal_string(r) ::= literal_string(expr) LITERAL_STR(t).                      { r = expr; SpkParser_Concat(r, &t, parser); }

%type block {SpkExpr *}
block(r) ::= LBRACK(t) statement_list(stmtList) expr(expr) RBRACK.              { r = SpkParser_NewBlock(stmtList.first, expr.first, &t, parser); } 
block(r) ::= LBRACK(t) statement_list(stmtList) RBRACK.                         { r = SpkParser_NewBlock(stmtList.first, 0, &t, parser); } 
block(r) ::= LBRACK(t) expr(expr) RBRACK.                                       { r = SpkParser_NewBlock(0, expr.first, &t, parser); } 

%type argument_list {SpkArgList}
argument_list(r) ::= expr_list(f).                                              { r.fixed = f.first; r.var = 0; }
argument_list(r) ::= expr_list(f) COMMA ELLIPSIS assignment_expr(v).            { r.fixed = f.first; r.var = v; }
argument_list(r) ::= ELLIPSIS assignment_expr(v).                               { r.fixed = 0;       r.var = v; }

%type expr_list {SpkExprList}
expr_list(r) ::= colon_expr(arg).                                               { r.first = arg; r.last = arg; }
expr_list(r) ::= decl_spec_list(declSpecList) colon_expr(arg).                  { r.first = arg; r.last = arg; SpkParser_SetDeclSpecs(arg, declSpecList.first, parser); }
expr_list(r) ::= expr_list(args) COMMA colon_expr(arg).                         { r = args; SpkParser_SetNextArg(r.last, arg, parser); r.last = arg; }
expr_list(r) ::= expr_list(args) COMMA decl_spec_list(declSpecList) colon_expr(arg).
                                                                                { r = args; SpkParser_SetNextArg(r.last, arg, parser); r.last = arg; SpkParser_SetDeclSpecs(arg, declSpecList.first, parser); }

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
