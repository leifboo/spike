
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
open_statement(r) ::= IF LPAREN expr(expr) RPAREN statement(ifTrue).            { r = SpkParser_NewStmt(STMT_IF_ELSE, expr.first, ifTrue, 0); }
open_statement(r) ::= IF LPAREN expr(expr) RPAREN closed_statement(ifTrue)
                      ELSE open_statement(ifFalse).                             { r = SpkParser_NewStmt(STMT_IF_ELSE, expr.first, ifTrue, ifFalse); }
open_statement(r) ::= WHILE LPAREN expr(expr) RPAREN statement(body).           { r = SpkParser_NewStmt(STMT_WHILE, expr.first, body, 0); }
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

%type closed_statement {Stmt *}
closed_statement(r) ::= IF LPAREN expr(expr) RPAREN closed_statement(ifTrue)
                        ELSE closed_statement(ifFalse).                         { r = SpkParser_NewStmt(STMT_IF_ELSE, expr.first, ifTrue, ifFalse); }
closed_statement(r) ::= SEMI.                                                   { r = SpkParser_NewStmt(STMT_EXPR, 0, 0, 0); }
closed_statement(r) ::= expr(expr) SEMI.                                        { r = SpkParser_NewStmt(STMT_EXPR, expr.first, 0, 0); }
closed_statement(r) ::= compound_statement(stmt).                               { r = stmt; }
closed_statement(r) ::= DO statement(body) WHILE LPAREN expr(expr) RPAREN SEMI. { r = SpkParser_NewStmt(STMT_DO_WHILE, expr.first, body, 0); }
closed_statement(r) ::= CONTINUE SEMI.                                          { r = SpkParser_NewStmt(STMT_CONTINUE, 0, 0, 0); }
closed_statement(r) ::= BREAK SEMI.                                             { r = SpkParser_NewStmt(STMT_BREAK, 0, 0, 0); }
closed_statement(r) ::= RETURN            SEMI.                                 { r = SpkParser_NewStmt(STMT_RETURN, 0, 0, 0); }
closed_statement(r) ::= RETURN expr(expr) SEMI.                                 { r = SpkParser_NewStmt(STMT_RETURN, expr.first, 0, 0); }
closed_statement(r) ::= VAR expr(declList) SEMI.                                { r = SpkParser_NewStmt(STMT_DEF_VAR, declList.first, 0, 0); }
closed_statement(r) ::= expr(expr) compound_statement(stmt).                    { r = SpkParser_NewStmt(STMT_DEF_METHOD, expr.first, stmt, 0); }
closed_statement(r) ::= CLASS TYPE_IDENTIFIER(name) compound_statement(stmt).   { r = SpkParser_NewClassDef(name.sym, 0, stmt); }
closed_statement(r) ::= CLASS TYPE_IDENTIFIER(name) COLON TYPE_IDENTIFIER(super) compound_statement(stmt).
                                                                                { r = SpkParser_NewClassDef(name.sym, super.sym, stmt); }

%type compound_statement {Stmt *}
compound_statement(r) ::= LCURLY                          RCURLY.               { r = SpkParser_NewStmt(STMT_COMPOUND, 0, 0, 0); }
compound_statement(r) ::= LCURLY statement_list(stmtList) RCURLY.               { r = SpkParser_NewStmt(STMT_COMPOUND, 0, stmtList.first, stmtList.last); }

%type expr {ExprList}
expr(r) ::= assignment_expr(expr).                                              { r.first = expr; r.last = expr; }
expr(r) ::= expr(left) COMMA assignment_expr(right).                            { r = left; r.last->next = right; r.last = right; }

%type assignment_expr {Expr *}
assignment_expr(r) ::= conditional_expr(expr).                                  { r = expr; }
assignment_expr(r) ::= unary_expr(left) ASSIGN        assignment_expr(right).   { r = SpkParser_NewExpr(EXPR_ASSIGN, OPER_EQ,     0, left, right); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_TIMES  assignment_expr(right).   { r = SpkParser_NewExpr(EXPR_ASSIGN, OPER_MUL,    0, left, right); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_DIVIDE assignment_expr(right).   { r = SpkParser_NewExpr(EXPR_ASSIGN, OPER_DIV,    0, left, right); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_MOD    assignment_expr(right).   { r = SpkParser_NewExpr(EXPR_ASSIGN, OPER_MOD,    0, left, right); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_PLUS   assignment_expr(right).   { r = SpkParser_NewExpr(EXPR_ASSIGN, OPER_ADD,    0, left, right); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_MINUS  assignment_expr(right).   { r = SpkParser_NewExpr(EXPR_ASSIGN, OPER_SUB,    0, left, right); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_LSHIFT assignment_expr(right).   { r = SpkParser_NewExpr(EXPR_ASSIGN, OPER_LSHIFT, 0, left, right); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_RSHIFT assignment_expr(right).   { r = SpkParser_NewExpr(EXPR_ASSIGN, OPER_RSHIFT, 0, left, right); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_BAND   assignment_expr(right).   { r = SpkParser_NewExpr(EXPR_ASSIGN, OPER_BAND,   0, left, right); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_BXOR   assignment_expr(right).   { r = SpkParser_NewExpr(EXPR_ASSIGN, OPER_BXOR,   0, left, right); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_BOR    assignment_expr(right).   { r = SpkParser_NewExpr(EXPR_ASSIGN, OPER_BOR,    0, left, right); }

%type conditional_expr {Expr *}
conditional_expr(r) ::= logical_expr(expr).                                     { r = expr; }
conditional_expr(r) ::= logical_expr(cond) QM logical_expr(left) COLON conditional_expr(right).
                                                                                { r = SpkParser_NewExpr(EXPR_COND, 0, cond, left, right); }

%left OR.
%left AND.

%type logical_expr {Expr *}
logical_expr(r) ::= binary_expr(expr).                                          { r = expr; }
logical_expr(r) ::= logical_expr(left) OR logical_expr(right).                  { r = SpkParser_NewExpr(EXPR_OR, 0, 0, left, right); }
logical_expr(r) ::= logical_expr(left) AND logical_expr(right).                 { r = SpkParser_NewExpr(EXPR_AND, 0, 0, left, right); }

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
binary_expr(r) ::= binary_expr(left) ID binary_expr(right).                     { r = SpkParser_NewExpr(EXPR_ID, 0, 0, left, right); }
binary_expr(r) ::= binary_expr(left) NI binary_expr(right).                     { r = SpkParser_NewExpr(EXPR_NI, 0, 0, left, right); }
binary_expr(r) ::= binary_expr(left) EQ binary_expr(right).                     { r = SpkParser_NewExpr(EXPR_BINARY, OPER_EQ, 0, left, right); }
binary_expr(r) ::= binary_expr(left) NE binary_expr(right).                     { r = SpkParser_NewExpr(EXPR_BINARY, OPER_NE, 0, left, right); }
binary_expr(r) ::= binary_expr(left) GT binary_expr(right).                     { r = SpkParser_NewExpr(EXPR_BINARY, OPER_GT, 0, left, right); }
binary_expr(r) ::= binary_expr(left) GE binary_expr(right).                     { r = SpkParser_NewExpr(EXPR_BINARY, OPER_GE, 0, left, right); }
binary_expr(r) ::= binary_expr(left) LT binary_expr(right).                     { r = SpkParser_NewExpr(EXPR_BINARY, OPER_LT, 0, left, right); }
binary_expr(r) ::= binary_expr(left) LE binary_expr(right).                     { r = SpkParser_NewExpr(EXPR_BINARY, OPER_LE, 0, left, right); }
binary_expr(r) ::= binary_expr(left) BOR binary_expr(right).                    { r = SpkParser_NewExpr(EXPR_BINARY, OPER_BOR, 0, left, right); }
binary_expr(r) ::= binary_expr(left) BXOR binary_expr(right).                   { r = SpkParser_NewExpr(EXPR_BINARY, OPER_BXOR, 0, left, right); }
binary_expr(r) ::= binary_expr(left) AMP binary_expr(right).                    { r = SpkParser_NewExpr(EXPR_BINARY, OPER_BAND, 0, left, right); }
binary_expr(r) ::= binary_expr(left) LSHIFT binary_expr(right).                 { r = SpkParser_NewExpr(EXPR_BINARY, OPER_LSHIFT, 0, left, right); }
binary_expr(r) ::= binary_expr(left) RSHIFT binary_expr(right).                 { r = SpkParser_NewExpr(EXPR_BINARY, OPER_RSHIFT, 0, left, right); }
binary_expr(r) ::= binary_expr(left) PLUS binary_expr(right).                   { r = SpkParser_NewExpr(EXPR_BINARY, OPER_ADD, 0, left, right); }
binary_expr(r) ::= binary_expr(left) MINUS binary_expr(right).                  { r = SpkParser_NewExpr(EXPR_BINARY, OPER_SUB, 0, left, right); }
binary_expr(r) ::= binary_expr(left) TIMES binary_expr(right).                  { r = SpkParser_NewExpr(EXPR_BINARY, OPER_MUL, 0, left, right); }
binary_expr(r) ::= binary_expr(left) DIVIDE binary_expr(right).                 { r = SpkParser_NewExpr(EXPR_BINARY, OPER_DIV, 0, left, right); }
binary_expr(r) ::= binary_expr(left) MOD binary_expr(right).                    { r = SpkParser_NewExpr(EXPR_BINARY, OPER_MOD, 0, left, right); }
binary_expr(r) ::= binary_expr(left) DOT_STAR binary_expr(right).               { r = SpkParser_NewExpr(EXPR_ATTR_VAR, 0, 0, left, right); }

%type unary_expr {Expr *}
unary_expr(r) ::= postfix_expr(expr).                                           { r = expr; }
unary_expr(r) ::= INC   unary_expr(expr).                                       { r = SpkParser_NewExpr(EXPR_PREOP, OPER_SUCC, 0, expr, 0); }
unary_expr(r) ::= DEC   unary_expr(expr).                                       { r = SpkParser_NewExpr(EXPR_PREOP, OPER_PRED, 0, expr, 0); }
unary_expr(r) ::= PLUS  unary_expr(expr).                                       { r = SpkParser_NewExpr(EXPR_UNARY, OPER_POS,  0, expr, 0); }
unary_expr(r) ::= MINUS unary_expr(expr).                                       { r = SpkParser_NewExpr(EXPR_UNARY, OPER_NEG,  0, expr, 0); }
unary_expr(r) ::= BNEG  unary_expr(expr).                                       { r = SpkParser_NewExpr(EXPR_UNARY, OPER_BNEG, 0, expr, 0); }
unary_expr(r) ::= LNEG  unary_expr(expr).                                       { r = SpkParser_NewExpr(EXPR_UNARY, OPER_LNEG, 0, expr, 0); }

%type postfix_expr {Expr *}
postfix_expr(r) ::= primary_expr(expr).                                         { r = expr; }
postfix_expr(r) ::= postfix_expr(func) LBRACK argument_list(args) RBRACK.       { r = SpkParser_NewExpr(EXPR_CALL, OPER_GET_ITEM, 0, func, args.fixed); r->var = args.var; }
postfix_expr(r) ::= postfix_expr(func) LPAREN RPAREN.                           { r = SpkParser_NewExpr(EXPR_CALL, OPER_APPLY, 0, func, 0); }
postfix_expr(r) ::= postfix_expr(func) LPAREN argument_list(args) RPAREN.       { r = SpkParser_NewExpr(EXPR_CALL, OPER_APPLY, 0, func, args.fixed); r->var = args.var; }
postfix_expr(r) ::= postfix_expr(obj) DOT IDENTIFIER(attr).                     { r = SpkParser_NewExpr(EXPR_ATTR, 0, 0, obj, 0); r->sym = attr.sym; }
postfix_expr(r) ::= postfix_expr(obj) DOT CLASS(attr).                          { r = SpkParser_NewExpr(EXPR_ATTR, 0, 0, obj, 0); r->sym = attr.sym; }
postfix_expr(r) ::= TYPE_IDENTIFIER(name) DOT IDENTIFIER(attr).                 { r = SpkParser_NewClassAttrExpr(name.sym, attr.sym); }
postfix_expr(r) ::= TYPE_IDENTIFIER(name) DOT CLASS(attr).                      { r = SpkParser_NewClassAttrExpr(name.sym, attr.sym); }
postfix_expr(r) ::= postfix_expr(expr) INC.                                     { r = SpkParser_NewExpr(EXPR_POSTOP, OPER_SUCC, 0, expr, 0); }
postfix_expr(r) ::= postfix_expr(expr) DEC.                                     { r = SpkParser_NewExpr(EXPR_POSTOP, OPER_PRED, 0, expr, 0); }

%type primary_expr {Expr *}
primary_expr(r) ::= IDENTIFIER(token).                                          { r = SpkParser_NewExpr(EXPR_NAME, 0, 0, 0, 0); r->sym = token.sym; }
primary_expr(r) ::= SYMBOL(token).                                              { r = SpkParser_NewExpr(EXPR_SYMBOL, 0, 0, 0, 0); r->sym = token.sym; }
primary_expr(r) ::= INT(token).                                                 { r = SpkParser_NewExpr(EXPR_INT, 0, 0, 0, 0); r->lit.intValue = token.lit.intValue; }
primary_expr(r) ::= FLOAT(token).                                               { r = SpkParser_NewExpr(EXPR_FLOAT, 0, 0, 0, 0); r->lit.floatValue = token.lit.floatValue; }
primary_expr(r) ::= CHAR(token).                                                { r = SpkParser_NewExpr(EXPR_CHAR, 0, 0, 0, 0); r->lit.charValue = token.lit.charValue; }
primary_expr(r) ::= STR(token).                                                 { r = SpkParser_NewExpr(EXPR_STR, 0, 0, 0, 0); r->lit.strValue = token.lit.strValue; }
primary_expr(r) ::= LPAREN expr(expr) RPAREN.                                   { r = expr.first; }

%type argument_list {ArgList}
argument_list(r) ::= expr_list(f).                                              { r.fixed = f.first; r.var = 0; }
argument_list(r) ::= expr_list(f) COMMA ELLIPSIS assignment_expr(v).            { r.fixed = f.first; r.var = v; }
argument_list(r) ::= ELLIPSIS assignment_expr(v).                               { r.fixed = 0;       r.var = v; }

%type expr_list {ExprList}
expr_list(r) ::= assignment_expr(arg).                                          { r.first = arg; r.last = arg; }
expr_list(r) ::= expr_list(args) COMMA assignment_expr(arg).                    { r = args; r.last->nextArg = arg; r.last = arg; }

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
