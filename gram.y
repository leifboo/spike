
%name SpkParser_Parse
%token_type {Token}
%token_prefix TOKEN_
%extra_argument { ParserState *parserState }

%include {
    #include <assert.h>
    #include "lexer.h"
    #include "parser.h"
}

start ::= statement_list(stmtList).                                             { parserState->root = stmtList.first; }

%type statement_list {StmtList}
statement_list(r) ::= statement(stmt).                                          { r.first = stmt; r.last = stmt; }
statement_list(r) ::= statement_list(stmtList) statement(stmt).                 { r = stmtList; r.last->next = stmt; r.last = stmt; }

%type statement {Stmt *}
statement(r) ::= open_statement(stmt).                                          { r = stmt; }
statement(r) ::= closed_statement(stmt).                                        { r = stmt; }

%type open_statement {Stmt *}
open_statement(r) ::= IF LPAREN expr(expr) RPAREN statement(ifTrue).            { r = SpkParser_NewStmt(STMT_IF_ELSE, expr, ifTrue, 0); }
open_statement(r) ::= IF LPAREN expr(expr) RPAREN closed_statement(ifTrue)
                      ELSE open_statement(ifFalse).                             { r = SpkParser_NewStmt(STMT_IF_ELSE, expr, ifTrue, ifFalse); }
open_statement(r) ::= WHILE LPAREN expr(expr) RPAREN statement(body).           { r = SpkParser_NewStmt(STMT_WHILE, expr, body, 0); }
open_statement(r) ::= FOR LPAREN             SEMI             SEMI             RPAREN statement(body).
                                                                                { r = SpkParser_NewForStmt(    0,     0,     0, body); }
open_statement(r) ::= FOR LPAREN             SEMI             SEMI expr(expr3) RPAREN statement(body).
                                                                                { r = SpkParser_NewForStmt(    0,     0, expr3, body); }
open_statement(r) ::= FOR LPAREN             SEMI expr(expr2) SEMI             RPAREN statement(body).
                                                                                { r = SpkParser_NewForStmt(    0, expr2,     0, body); }
open_statement(r) ::= FOR LPAREN             SEMI expr(expr2) SEMI expr(expr3) RPAREN statement(body).
                                                                                { r = SpkParser_NewForStmt(    0, expr2, expr3, body); }
open_statement(r) ::= FOR LPAREN expr(expr1) SEMI             SEMI             RPAREN statement(body).
                                                                                { r = SpkParser_NewForStmt(expr1,     0,     0, body); }
open_statement(r) ::= FOR LPAREN expr(expr1) SEMI             SEMI expr(expr3) RPAREN statement(body).
                                                                                { r = SpkParser_NewForStmt(expr1,     0, expr3, body); }
open_statement(r) ::= FOR LPAREN expr(expr1) SEMI expr(expr2) SEMI             RPAREN statement(body).
                                                                                { r = SpkParser_NewForStmt(expr1, expr2,     0, body); }
open_statement(r) ::= FOR LPAREN expr(expr1) SEMI expr(expr2) SEMI expr(expr3) RPAREN statement(body).
                                                                                { r = SpkParser_NewForStmt(expr1, expr2, expr3, body); }

%type closed_statement {Stmt *}
closed_statement(r) ::= IF LPAREN expr(expr) RPAREN closed_statement(ifTrue)
                        ELSE closed_statement(ifFalse).                         { r = SpkParser_NewStmt(STMT_IF_ELSE, expr, ifTrue, ifFalse); }
closed_statement(r) ::= SEMI.                                                   { r = SpkParser_NewStmt(STMT_EXPR, 0, 0, 0); }
closed_statement(r) ::= expr(expr) SEMI.                                        { r = SpkParser_NewStmt(STMT_EXPR, expr, 0, 0); }
closed_statement(r) ::= compound_statement(stmt).                               { r = stmt; }
closed_statement(r) ::= DO statement(body) WHILE LPAREN expr(expr) RPAREN SEMI. { r = SpkParser_NewStmt(STMT_DO_WHILE, expr, body, 0); }
closed_statement(r) ::= CONTINUE SEMI.                                          { r = SpkParser_NewStmt(STMT_CONTINUE, 0, 0, 0); }
closed_statement(r) ::= BREAK SEMI.                                             { r = SpkParser_NewStmt(STMT_BREAK, 0, 0, 0); }
closed_statement(r) ::= RETURN            SEMI.                                 { r = SpkParser_NewStmt(STMT_RETURN, 0, 0, 0); }
closed_statement(r) ::= RETURN expr(expr) SEMI.                                 { r = SpkParser_NewStmt(STMT_RETURN, expr, 0, 0); }
closed_statement(r) ::= VAR expr(declList) SEMI.                                { r = SpkParser_NewStmt(STMT_DEF_VAR, declList, 0, 0); }
closed_statement(r) ::= expr(expr) compound_statement(stmt).                    { r = SpkParser_NewStmt(STMT_DEF_METHOD, expr, stmt, 0); }
closed_statement(r) ::= CLASS TYPE_IDENTIFIER(name) compound_statement(stmt).   { r = SpkParser_NewClassDef(name.sym, 0, stmt); }
closed_statement(r) ::= CLASS TYPE_IDENTIFIER(name) COLON TYPE_IDENTIFIER(super) compound_statement(stmt).
                                                                                { r = SpkParser_NewClassDef(name.sym, super.sym, stmt); }

%type compound_statement {Stmt *}
compound_statement(r) ::= LCURLY                          RCURLY.               { r = SpkParser_NewStmt(STMT_COMPOUND, 0, 0, 0); }
compound_statement(r) ::= LCURLY statement_list(stmtList) RCURLY.               { r = SpkParser_NewStmt(STMT_COMPOUND, 0, stmtList.first, stmtList.last); }

%type expr {Expr *}
expr(r) ::= assignment_expr(expr).                                              { r = expr; }
expr(r) ::= expr(left) COMMA assignment_expr(right).                            { r = SpkParser_Comma(left, right); }

%left EQ NE ID NI.
%left GT GE LT LE.
%left PLUS MINUS.
%left TIMES DIVIDE.

%type assignment_expr {Expr *}
assignment_expr(r) ::= binary_expr(expr).                                       { r = expr; }
assignment_expr(r) ::= postfix_expr(left) ASSIGN assignment_expr(right).        { r = SpkParser_NewExpr(EXPR_ASSIGN, OPER_EQ, 0, left, right); }

%type binary_expr {Expr *}
binary_expr(r) ::= binary_expr(left) ID binary_expr(right).                     { r = SpkParser_NewExpr(EXPR_ID, 0, 0, left, right); }
binary_expr(r) ::= binary_expr(left) NI binary_expr(right).                     { r = SpkParser_NewExpr(EXPR_NI, 0, 0, left, right); }
binary_expr(r) ::= binary_expr(left) EQ binary_expr(right).                     { r = SpkParser_NewExpr(EXPR_BINARY, OPER_EQ, 0, left, right); }
binary_expr(r) ::= binary_expr(left) NE binary_expr(right).                     { r = SpkParser_NewExpr(EXPR_BINARY, OPER_NE, 0, left, right); }
binary_expr(r) ::= binary_expr(left) GT binary_expr(right).                     { r = SpkParser_NewExpr(EXPR_BINARY, OPER_GT, 0, left, right); }
binary_expr(r) ::= binary_expr(left) GE binary_expr(right).                     { r = SpkParser_NewExpr(EXPR_BINARY, OPER_GE, 0, left, right); }
binary_expr(r) ::= binary_expr(left) LT binary_expr(right).                     { r = SpkParser_NewExpr(EXPR_BINARY, OPER_LT, 0, left, right); }
binary_expr(r) ::= binary_expr(left) LE binary_expr(right).                     { r = SpkParser_NewExpr(EXPR_BINARY, OPER_LE, 0, left, right); }
binary_expr(r) ::= binary_expr(left) PLUS binary_expr(right).                   { r = SpkParser_NewExpr(EXPR_BINARY, OPER_ADD, 0, left, right); }
binary_expr(r) ::= binary_expr(left) MINUS binary_expr(right).                  { r = SpkParser_NewExpr(EXPR_BINARY, OPER_SUB, 0, left, right); }
binary_expr(r) ::= binary_expr(left) TIMES binary_expr(right).                  { r = SpkParser_NewExpr(EXPR_BINARY, OPER_MUL, 0, left, right); }
binary_expr(r) ::= binary_expr(left) DIVIDE binary_expr(right).                 { r = SpkParser_NewExpr(EXPR_BINARY, OPER_DIV, 0, left, right); }
binary_expr(r) ::= postfix_expr(expr).                                          { r = expr; }

%type postfix_expr {Expr *}
postfix_expr(r) ::= postfix_expr(func) LPAREN RPAREN.                           { r = SpkParser_NewExpr(EXPR_POSTFIX, OPER_CALL, 0, func, 0); }
postfix_expr(r) ::= postfix_expr(func) LPAREN argument_expr_list(args) RPAREN.  { r = SpkParser_NewExpr(EXPR_POSTFIX, OPER_CALL, 0, func, args.first); }
postfix_expr(r) ::= postfix_expr(obj) DOT IDENTIFIER(attr).                     { r = SpkParser_NewExpr(EXPR_ATTR, 0, 0, obj, 0); r->sym = attr.sym; }
postfix_expr(r) ::= TYPE_IDENTIFIER(name) DOT IDENTIFIER(attr).                 { r = SpkParser_NewClassAttrExpr(name.sym, attr.sym); }
postfix_expr(r) ::= primary_expr(expr).                                         { r = expr; }

%type primary_expr {Expr *}
primary_expr(r) ::= IDENTIFIER(token).                                          { r = SpkParser_NewExpr(EXPR_NAME, 0, 0, 0, 0); r->sym = token.sym; }
primary_expr(r) ::= INT(token).                                                 { r = SpkParser_NewExpr(EXPR_INT, 0, 0, 0, 0); r->intValue = token.intValue; }
primary_expr(r) ::= LPAREN expr(expr) RPAREN.                                   { r = expr; }

%type argument_expr_list {ExprList}
argument_expr_list(r) ::= binary_expr(arg).                                     { r.first = arg; r.last = arg; }
argument_expr_list(r) ::= argument_expr_list(args) COMMA binary_expr(arg).      { r = args; r.last->next = arg; r.last = arg; }

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
