
%name Parser_Parse
%token_type {int}
%token_prefix TOKEN_
%extra_argument { yyscan_t lexer }

%include {
    #include "lexer.h"
    #include <assert.h>
    extern FILE *spkout, *spkerr;
    static int S, E, L, TMP;
}

start ::= statement_list(stmtList).                                             { fprintf(spkout, "root = l%d\n", stmtList); }

statement_list(r) ::= statement(stmt).                                          { fprintf(spkout, "l%d = [s%d]\n", r = ++L, stmt); }
statement_list(r) ::= statement_list(stmtList) statement(stmt).                 { r = stmtList; fprintf(spkout, "l%d.append(s%d)\n", r, stmt); }

statement_list_expr(r) ::= statement_list(stmtList) expr(expr).                 { fprintf(spkout, "tmp%d = (l%d, e%d)\n", r = ++TMP, stmtList, expr); } 
statement_list_expr(r) ::= statement_list(stmtList).                            { fprintf(spkout, "tmp%d = (l%d, [])\n", r = ++TMP, stmtList       ); } 
statement_list_expr(r) ::= expr(expr).                                          { fprintf(spkout, "tmp%d = ([],  e%d)\n", r = ++TMP,           expr); } 

statement(r) ::= open_statement(stmt).                                          { r = stmt; }
statement(r) ::= closed_statement(stmt).                                        { r = stmt; }
//statement(r) ::= IDENTIFIER COLON statement(stmt).                              { r = stmt; }

open_statement(r) ::= IF LPAREN expr(expr) RPAREN statement(ifTrue).            { fprintf(spkout, "s%d = f.stmtIfElse(e%d, s%d, None)\n", r = ++S, expr, ifTrue); }
open_statement(r) ::= IF LPAREN expr(expr) RPAREN closed_statement(ifTrue)
                      ELSE open_statement(ifFalse).                             { fprintf(spkout, "s%d = f.stmtIfElse(e%d, s%d, s%d)\n", r = ++S, expr, ifTrue, ifFalse); }
open_statement(r) ::= WHILE LPAREN expr(expr) RPAREN statement(body).           { fprintf(spkout, "s%d = f.stmtWhile(e%d, s%d)\n", r = ++S, expr, body); }
open_statement(r) ::= FOR LPAREN             SEMI             SEMI             RPAREN statement(body).
                                                                                { fprintf(spkout, "s%d = f.stmtFor(None, None, None, s%d)\n", r = ++S,                      body); }
open_statement(r) ::= FOR LPAREN             SEMI             SEMI expr(expr3) RPAREN statement(body).
                                                                                { fprintf(spkout, "s%d = f.stmtFor(None, None, e%d,  s%d)\n", r = ++S,               expr3, body); }
open_statement(r) ::= FOR LPAREN             SEMI expr(expr2) SEMI             RPAREN statement(body).
                                                                                { fprintf(spkout, "s%d = f.stmtFor(None, e%d,  None, s%d)\n", r = ++S,        expr2,        body); }
open_statement(r) ::= FOR LPAREN             SEMI expr(expr2) SEMI expr(expr3) RPAREN statement(body).
                                                                                { fprintf(spkout, "s%d = f.stmtFor(None, e%d,  e%d,  s%d)\n", r = ++S,        expr2, expr3, body); }
open_statement(r) ::= FOR LPAREN expr(expr1) SEMI             SEMI             RPAREN statement(body).
                                                                                { fprintf(spkout, "s%d = f.stmtFor(e%d,  None, None, s%d)\n", r = ++S, expr1,               body); }
open_statement(r) ::= FOR LPAREN expr(expr1) SEMI             SEMI expr(expr3) RPAREN statement(body).
                                                                                { fprintf(spkout, "s%d = f.stmtFor(e%d,  None, e%d,  s%d)\n", r = ++S, expr1,        expr3, body); }
open_statement(r) ::= FOR LPAREN expr(expr1) SEMI expr(expr2) SEMI             RPAREN statement(body).
                                                                                { fprintf(spkout, "s%d = f.stmtFor(e%d,  e%d,  None, s%d)\n", r = ++S, expr1, expr2,        body); }
open_statement(r) ::= FOR LPAREN expr(expr1) SEMI expr(expr2) SEMI expr(expr3) RPAREN statement(body).
                                                                                { fprintf(spkout, "s%d = f.stmtFor(e%d,  e%d,  e%d,  s%d)\n", r = ++S, expr1, expr2, expr3, body); }

closed_statement(r) ::= IF LPAREN expr(expr) RPAREN closed_statement(ifTrue)
                        ELSE closed_statement(ifFalse).                         { fprintf(spkout, "s%d = f.stmtIfElse(e%d, s%d, s%d)\n", r = ++S, expr, ifTrue, ifFalse); }
closed_statement(r) ::= SEMI.                                                   { fprintf(spkout, "s%d = f.stmtExpr(None)\n", r = ++S); }
closed_statement(r) ::= expr(expr) SEMI.                                        { fprintf(spkout, "s%d = f.stmtExprOrDefVar(e%d)\n", r = ++S, expr); }
closed_statement(r) ::= compound_statement(stmt).                               { r = stmt; }
closed_statement(r) ::= DO statement(body) WHILE LPAREN expr(expr) RPAREN SEMI. { fprintf(spkout, "s%d = f.stmtDoWhile(s%d, e%d)\n", r = ++S, body, expr); }
closed_statement(r) ::= CONTINUE SEMI.                                          { fprintf(spkout, "s%d = f.stmtContinue()\n", r = ++S); }
closed_statement(r) ::= BREAK SEMI.                                             { fprintf(spkout, "s%d = f.stmtBreak()\n", r = ++S); }
closed_statement(r) ::= RETURN            SEMI.                                 { fprintf(spkout, "s%d = f.stmtReturn(None)\n", r = ++S); }
closed_statement(r) ::= RETURN expr(expr) SEMI.                                 { fprintf(spkout, "s%d = f.stmtReturn(e%d)\n", r = ++S, expr); }
closed_statement(r) ::= TRAP SEMI.                                              { fprintf(spkout, "s%d = f.stmtTrap()\n", r = ++S); }
closed_statement(r) ::= YIELD            SEMI.                                  { fprintf(spkout, "s%d = f.stmtYield(None)\n", r = ++S); }
closed_statement(r) ::= YIELD expr(expr) SEMI.                                  { fprintf(spkout, "s%d = f.stmtYield(e%d)\n", r = ++S, expr); }
closed_statement(r) ::= expr(expr) compound_statement(stmt).                    { fprintf(spkout, "s%d = f.stmtDefMethod(e%d, s%d)\n", r = ++S, expr, stmt); }
closed_statement(r) ::= CLASS name(name) COLON name(super) class_body(body).    { fprintf(spkout, "s%d = f.stmtDefClass(e%d, e%d,  tmp%d[0], tmp%d[1])\n", r = ++S, name, super, body, body); }

class_body(r) ::= compound_statement(body).                                     { fprintf(spkout, "tmp%d = (s%d, None)\n", r = ++TMP, body); }
class_body(r) ::= compound_statement(body) META compound_statement(metaBody).   { fprintf(spkout, "tmp%d = (s%d,  s%d)\n", r = ++TMP, body, metaBody); } 

compound_statement(r) ::= LCURLY                          RCURLY.               { fprintf(spkout, "s%d = f.stmtCompound([])\n", r = ++S); }
compound_statement(r) ::= LCURLY statement_list(stmtList) RCURLY.               { fprintf(spkout, "s%d = f.stmtCompound(l%d)\n", r = ++S, stmtList); }

expr(r) ::= comma_expr(expr).                                                   { r = expr; }
expr(r) ::= decl_spec_list(declSpecList) comma_expr(expr).                      { r = expr; fprintf(spkout, "e%d.declSpecs = l%d\n", r, declSpecList); }

decl_spec_list(r) ::= decl_spec(declSpec).                                      { fprintf(spkout, "l%d = [e%d]\n", r = ++L, declSpec); }
decl_spec_list(r) ::= decl_spec_list(declSpecList) decl_spec(declSpec).         { r = declSpecList; fprintf(spkout, "l%d.append(e%d)\n", r, declSpec); }

decl_spec(r) ::= SPECIFIER(name).                                               { fprintf(spkout, "e%d = f.exprName(t%d)\n", r = ++E, name); }

comma_expr(r) ::= colon_expr(expr).                                             { r = expr; }
comma_expr(r) ::= comma_expr(left) COMMA colon_expr(right).                     { fprintf(spkout, "e%d = e%d.comma(e%d)\n", r = ++E, left, right); }

colon_expr(r) ::= assignment_expr(expr).                                        { r = expr; }
//colon_expr(r) ::= colon_expr COLON assignment_expr(expr).                       { r = expr; }

assignment_expr(r) ::= keyword_expr(expr).                                      { r = expr; }
assignment_expr(r) ::= unary_expr(left) ASSIGN        assignment_expr(right).   { fprintf(spkout, "e%d = f.exprAssign(f.operEq,     e%d, e%d)\n", r = ++E, left, right); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_TIMES  assignment_expr(right).   { fprintf(spkout, "e%d = f.exprAssign(f.operMul,    e%d, e%d)\n", r = ++E, left, right); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_DIVIDE assignment_expr(right).   { fprintf(spkout, "e%d = f.exprAssign(f.operDiv,    e%d, e%d)\n", r = ++E, left, right); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_MOD    assignment_expr(right).   { fprintf(spkout, "e%d = f.exprAssign(f.operMod,    e%d, e%d)\n", r = ++E, left, right); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_PLUS   assignment_expr(right).   { fprintf(spkout, "e%d = f.exprAssign(f.operAdd,    e%d, e%d)\n", r = ++E, left, right); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_MINUS  assignment_expr(right).   { fprintf(spkout, "e%d = f.exprAssign(f.operSub,    e%d, e%d)\n", r = ++E, left, right); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_LSHIFT assignment_expr(right).   { fprintf(spkout, "e%d = f.exprAssign(f.operLShift, e%d, e%d)\n", r = ++E, left, right); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_RSHIFT assignment_expr(right).   { fprintf(spkout, "e%d = f.exprAssign(f.operRShift, e%d, e%d)\n", r = ++E, left, right); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_BAND   assignment_expr(right).   { fprintf(spkout, "e%d = f.exprAssign(f.operBAnd,   e%d, e%d)\n", r = ++E, left, right); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_BXOR   assignment_expr(right).   { fprintf(spkout, "e%d = f.exprAssign(f.operBXOr,   e%d, e%d)\n", r = ++E, left, right); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_BOR    assignment_expr(right).   { fprintf(spkout, "e%d = f.exprAssign(f.operBOr,    e%d, e%d)\n", r = ++E, left, right); }

keyword_expr(r) ::= conditional_expr(expr).                                     { r = expr; }
keyword_expr(r) ::= conditional_expr(receiver) keyword_message(message).        { fprintf(spkout, "e%d = f.exprKeyword(e%d, *tmp%d)\n", r = ++E, receiver, message); }

keyword_message(r) ::= keyword_list(kwList).                                    { fprintf(spkout, "tmp%d = (None, tmp%d[0], tmp%d[1])\n", r = ++TMP,     kwList, kwList); }
keyword_message(r) ::= keyword(kw).                                             { fprintf(spkout, "tmp%d = (t%d,  None,     []      )\n", r = ++TMP, kw                ); }
keyword_message(r) ::= keyword(kw) keyword_list(kwList).                        { fprintf(spkout, "tmp%d = (t%d,  tmp%d[0], tmp%d[1])\n", r = ++TMP, kw, kwList, kwList); }

keyword_list(r) ::= keyword(kw) COLON conditional_expr(arg).                    { fprintf(spkout, "tmp%d = ([t%d], [e%d])\n", r = ++TMP, kw, arg); }
keyword_list(r) ::= keyword_list(kwList) keyword(kw) COLON conditional_expr(arg).
                                                                                { r = kwList; fprintf(spkout, "tmp%d[0].append(t%d); tmp%d[1].append(e%d)\n", r, kw, r, arg); }

keyword(r) ::= IDENTIFIER(kw).                                                  { r = kw; }
keyword(r) ::= BREAK(kw).                                                       { r = kw; }
keyword(r) ::= CLASS(kw).                                                       { r = kw; }
keyword(r) ::= CONTINUE(kw).                                                    { r = kw; }
keyword(r) ::= DO(kw).                                                          { r = kw; }
keyword(r) ::= ELSE(kw).                                                        { r = kw; }
keyword(r) ::= FOR(kw).                                                         { r = kw; }
keyword(r) ::= IF(kw).                                                          { r = kw; }
keyword(r) ::= META(kw).                                                        { r = kw; }
keyword(r) ::= RETURN(kw).                                                      { r = kw; }
keyword(r) ::= WHILE(kw).                                                       { r = kw; }
keyword(r) ::= YIELD(kw).                                                       { r = kw; }

conditional_expr(r) ::= logical_expr(expr).                                     { r = expr; }
conditional_expr(r) ::= logical_expr(cond) QM logical_expr(left) COLON conditional_expr(right).
                                                                                { fprintf(spkout, "e%d = f.exprCond(e%d, e%d, e%d)\n", r = ++E, cond, left, right); }

%left OR.
%left AND.

logical_expr(r) ::= binary_expr(expr).                                          { r = expr; }
logical_expr(r) ::= logical_expr(left) OR logical_expr(right).                  { fprintf(spkout, "e%d = f.exprOr(e%d, e%d)\n", r = ++E, left, right); }
logical_expr(r) ::= logical_expr(left) AND logical_expr(right).                 { fprintf(spkout, "e%d = f.exprAnd(e%d, e%d)\n", r = ++E, left, right); }

%left ID NI EQ NE.
%left GT GE LT LE.
%left BOR.
%left BXOR.
%left AMP.
%left LSHIFT RSHIFT.
%left PLUS MINUS.
%left TIMES DIVIDE MOD.
%left DOT_STAR.

binary_expr(r) ::= unary_expr(expr).                                            { r = expr; }
binary_expr(r) ::= binary_expr(left) ID binary_expr(right).                     { fprintf(spkout, "e%d = f.exprId(e%d, e%d)\n", r = ++E, left, right); }
binary_expr(r) ::= binary_expr(left) NI binary_expr(right).                     { fprintf(spkout, "e%d = f.exprNI(e%d, e%d)\n", r = ++E, left, right); }
binary_expr(r) ::= binary_expr(left) EQ binary_expr(right).                     { fprintf(spkout, "e%d = f.exprBinary(f.operEq,     e%d, e%d)\n", r = ++E, left, right); }
binary_expr(r) ::= binary_expr(left) NE binary_expr(right).                     { fprintf(spkout, "e%d = f.exprBinary(f.operNE,     e%d, e%d)\n", r = ++E, left, right); }
binary_expr(r) ::= binary_expr(left) GT binary_expr(right).                     { fprintf(spkout, "e%d = f.exprBinary(f.operGT,     e%d, e%d)\n", r = ++E, left, right); }
binary_expr(r) ::= binary_expr(left) GE binary_expr(right).                     { fprintf(spkout, "e%d = f.exprBinary(f.operGE,     e%d, e%d)\n", r = ++E, left, right); }
binary_expr(r) ::= binary_expr(left) LT binary_expr(right).                     { fprintf(spkout, "e%d = f.exprBinary(f.operLT,     e%d, e%d)\n", r = ++E, left, right); }
binary_expr(r) ::= binary_expr(left) LE binary_expr(right).                     { fprintf(spkout, "e%d = f.exprBinary(f.operLE,     e%d, e%d)\n", r = ++E, left, right); }
binary_expr(r) ::= binary_expr(left) BOR binary_expr(right).                    { fprintf(spkout, "e%d = f.exprBinary(f.operBOr,    e%d, e%d)\n", r = ++E, left, right); }
binary_expr(r) ::= binary_expr(left) BXOR binary_expr(right).                   { fprintf(spkout, "e%d = f.exprBinary(f.operBXOr,   e%d, e%d)\n", r = ++E, left, right); }
binary_expr(r) ::= binary_expr(left) AMP binary_expr(right).                    { fprintf(spkout, "e%d = f.exprBinary(f.operBAnd,   e%d, e%d)\n", r = ++E, left, right); }
binary_expr(r) ::= binary_expr(left) LSHIFT binary_expr(right).                 { fprintf(spkout, "e%d = f.exprBinary(f.operLShift, e%d, e%d)\n", r = ++E, left, right); }
binary_expr(r) ::= binary_expr(left) RSHIFT binary_expr(right).                 { fprintf(spkout, "e%d = f.exprBinary(f.operRShift, e%d, e%d)\n", r = ++E, left, right); }
binary_expr(r) ::= binary_expr(left) PLUS binary_expr(right).                   { fprintf(spkout, "e%d = f.exprBinary(f.operAdd,    e%d, e%d)\n", r = ++E, left, right); }
binary_expr(r) ::= binary_expr(left) MINUS binary_expr(right).                  { fprintf(spkout, "e%d = f.exprBinary(f.operSub,    e%d, e%d)\n", r = ++E, left, right); }
binary_expr(r) ::= binary_expr(left) TIMES binary_expr(right).                  { fprintf(spkout, "e%d = f.exprBinary(f.operMul,    e%d, e%d)\n", r = ++E, left, right); }
binary_expr(r) ::= binary_expr(left) DIVIDE binary_expr(right).                 { fprintf(spkout, "e%d = f.exprBinary(f.operDiv,    e%d, e%d)\n", r = ++E, left, right); }
binary_expr(r) ::= binary_expr(left) MOD binary_expr(right).                    { fprintf(spkout, "e%d = f.exprBinary(f.operMod,    e%d, e%d)\n", r = ++E, left, right); }
binary_expr(r) ::= binary_expr(left) DOT_STAR binary_expr(right).               { fprintf(spkout, "e%d = f.exprAttrVar(e%d, e%d)\n", r = ++E, left, right); }

unary_expr(r) ::= postfix_expr(expr).                                           { r = expr; }
unary_expr(r) ::= INC   unary_expr(expr).                                       { fprintf(spkout, "e%d = f.exprPreOp(f.operSucc, e%d)\n", r = ++E, expr); }
unary_expr(r) ::= DEC   unary_expr(expr).                                       { fprintf(spkout, "e%d = f.exprPreOp(f.operPred, e%d)\n", r = ++E, expr); }
unary_expr(r) ::= PLUS  unary_expr(expr).                                       { fprintf(spkout, "e%d = f.exprUnary(f.operPos,  e%d)\n", r = ++E, expr); }
unary_expr(r) ::= MINUS unary_expr(expr).                                       { fprintf(spkout, "e%d = f.exprUnary(f.operNeg,  e%d)\n", r = ++E, expr); }
unary_expr(r) ::= TIMES unary_expr(expr).                                       { fprintf(spkout, "e%d = f.exprUnary(f.operInd,  e%d)\n", r = ++E, expr); }
unary_expr(r) ::= BNEG  unary_expr(expr).                                       { fprintf(spkout, "e%d = f.exprUnary(f.operBNeg, e%d)\n", r = ++E, expr); }
unary_expr(r) ::= LNEG  unary_expr(expr).                                       { fprintf(spkout, "e%d = f.exprUnary(f.operLNeg, e%d)\n", r = ++E, expr); }

postfix_expr(r) ::= primary_expr(expr).                                         { r = expr; }
postfix_expr(r) ::= postfix_expr(func) LBRACK argument_list(args) RBRACK.       { fprintf(spkout, "e%d = f.exprCall(f.operIndex, e%d, tmp%d[0], tmp%d[1])\n", r = ++E, func, args, args); }
postfix_expr(r) ::= postfix_expr(func) LPAREN RPAREN.                           { fprintf(spkout, "e%d = f.exprCall(f.operApply, e%d, [],       None    )\n", r = ++E, func            ); }
postfix_expr(r) ::= postfix_expr(func) LPAREN argument_list(args) RPAREN.       { fprintf(spkout, "e%d = f.exprCall(f.operApply, e%d, tmp%d[0], tmp%d[1])\n", r = ++E, func, args, args); }
postfix_expr(r) ::= postfix_expr(obj) DOT IDENTIFIER(attr).                     { fprintf(spkout, "e%d = f.exprAttr(e%d, t%d)\n", r = ++E, obj,  attr); }
postfix_expr(r) ::= postfix_expr(obj) DOT SPECIFIER(attr).                      { fprintf(spkout, "e%d = f.exprAttr(e%d, t%d)\n", r = ++E, obj,  attr); }
postfix_expr(r) ::= postfix_expr(obj) DOT CLASS(attr).                          { fprintf(spkout, "e%d = f.exprAttr(e%d, t%d)\n", r = ++E, obj,  attr); }
postfix_expr(r) ::= SPECIFIER(name) DOT IDENTIFIER(attr).                       { fprintf(spkout, "e%d = f.exprAttr(t%d, t%d)\n", r = ++E, name, attr); }
postfix_expr(r) ::= SPECIFIER(name) DOT CLASS(attr).                            { fprintf(spkout, "e%d = f.exprAttr(t%d, t%d)\n", r = ++E, name, attr); }
postfix_expr(r) ::= postfix_expr(expr) INC.                                     { fprintf(spkout, "e%d = f.exprPostOp(f.operSucc, e%d)\n", r = ++E, expr); }
postfix_expr(r) ::= postfix_expr(expr) DEC.                                     { fprintf(spkout, "e%d = f.exprPostOp(f.operPred, e%d)\n", r = ++E, expr); }

primary_expr(r) ::= name(expr).                                                 { r = expr; }
primary_expr(r) ::= LITERAL_SYMBOL(token).                                      { fprintf(spkout, "e%d = f.exprLiteral(t%d)\n", r = ++E, token); }
primary_expr(r) ::= LITERAL_INT(token).                                         { fprintf(spkout, "e%d = f.exprLiteral(t%d)\n", r = ++E, token); }
primary_expr(r) ::= LITERAL_FLOAT(token).                                       { fprintf(spkout, "e%d = f.exprLiteral(t%d)\n", r = ++E, token); }
primary_expr(r) ::= LITERAL_CHAR(token).                                        { fprintf(spkout, "e%d = f.exprLiteral(t%d)\n", r = ++E, token); }
primary_expr(r) ::= literal_string(expr).                                       { r = expr; }
primary_expr(r) ::= LPAREN expr(expr) RPAREN.                                   { r = expr; }
primary_expr(r) ::= block(expr).                                                { r = expr; }
//primary_expr(r) ::= LCURLY expr(expr) RCURLY.                                   { fprintf(spkout, "e%d = f.exprCompound(e%d)\n", r = ++E, expr); }

name(r) ::= IDENTIFIER(name).                                                   { fprintf(spkout, "e%d = f.exprName(t%d)\n", r = ++E, name); }
name(r) ::= LPAREN SPECIFIER(name) RPAREN.                                      { fprintf(spkout, "e%d = f.exprName(t%d)\n", r = ++E, name); }

literal_string(r) ::= LITERAL_STR(token).                                       { fprintf(spkout, "e%d = f.exprLiteral(t%d)\n", r = ++E, token); }
literal_string(r) ::= literal_string(expr) LITERAL_STR(token).                  { r = expr; fprintf(spkout, "e%d.concat(t%d)\n", r, token); }

block(r) ::= LBRACK statement_list_expr(sle) RBRACK.                            { fprintf(spkout, "e%d = f.exprBlock([], tmp%d[0], tmp%d[1])\n", r = ++E, sle, sle); }
block(r) ::= LBRACK block_argument_list(args) BOR statement_list_expr(sle) RBRACK.
                                                                                { fprintf(spkout, "e%d = f.exprBlock(l%d, tmp%d[0], tmp%d[1])\n", r = ++E, args, sle, sle); }

block_argument_list(r) ::= COLON block_arg(arg).                                { fprintf(spkout, "l%d = [e%d]\n", r = ++L, arg); }
block_argument_list(r) ::= block_argument_list(args) COLON block_arg(arg).      { r = args; fprintf(spkout, "l%d.append(e%d)\n", r, arg); }

block_arg(r) ::= unary_expr(arg).                                               { r = arg; }
block_arg(r) ::= decl_spec_list(declSpecList) unary_expr(arg).                  { r = arg; fprintf(spkout, "e%d.declSpecs = l%d\n", r, declSpecList); }

argument_list(r) ::= fixed_arg_list(f).                                         { fprintf(spkout, "tmp%d = (l%d, None)\n", r = ++TMP, f   ); }
argument_list(r) ::= fixed_arg_list(f) COMMA ELLIPSIS assignment_expr(v).       { fprintf(spkout, "tmp%d = (l%d,  e%d)\n", r = ++TMP, f, v); }
argument_list(r) ::= ELLIPSIS assignment_expr(v).                               { fprintf(spkout, "tmp%d = ([],   e%d)\n", r = ++TMP,    v); }

fixed_arg_list(r) ::= arg(arg).                                                 { fprintf(spkout, "l%d = [e%d]\n", r = ++L, arg); }
fixed_arg_list(r) ::= fixed_arg_list(args) COMMA arg(arg).                      { r = args; fprintf(spkout, "l%d.append(e%d)\n", r, arg); }

arg(r) ::= colon_expr(arg).                                                     { r = arg; }
arg(r) ::= decl_spec_list(declSpecList) colon_expr(arg).                        { r = arg; fprintf(spkout, "e%d.declSpecs = l%d\n", r, declSpecList); }


%syntax_error {
    fprintf(spkerr, "n.syntaxError(%d)\n", Lexer_get_lineno(lexer));
}

%parse_accept {
    fprintf(spkerr, "accepted = True\n");
}

%parse_failure {
    fprintf(spkerr, "accepted = False\n");
}
