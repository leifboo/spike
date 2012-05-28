
%name Parser_Parse
%token_type {int}
%token_prefix TOKEN_

%include {
    #include <assert.h>
    static int S, E, L, TMP;
}

start ::= statement_list(stmtList).                                             { printf("root = l%d\n", stmtList); }

statement_list(r) ::= statement(stmt).                                          { printf("l%d = [s%d]\n", r = ++L, stmt); }
statement_list(r) ::= statement_list(stmtList) statement(stmt).                 { r = stmtList; printf("l%d.append(s%d)\n", r, stmt); }

statement_list_expr(r) ::= statement_list(stmtList) expr(expr).                 { printf("tmp%d = (l%d, e%d)\n", r = ++TMP, stmtList, expr); } 
statement_list_expr(r) ::= statement_list(stmtList).                            { printf("tmp%d = (l%d, [])\n", r = ++TMP, stmtList       ); } 
statement_list_expr(r) ::= expr(expr).                                          { printf("tmp%d = ([],  e%d)\n", r = ++TMP,           expr); } 

statement(r) ::= open_statement(stmt).                                          { r = stmt; }
statement(r) ::= closed_statement(stmt).                                        { r = stmt; }
//statement(r) ::= IDENTIFIER COLON statement(stmt).                              { r = stmt; }

open_statement(r) ::= IF LPAREN expr(expr) RPAREN statement(ifTrue).            { printf("s%d = f.stmtIfElse(e%d, s%d, None)\n", r = ++S, expr, ifTrue); }
open_statement(r) ::= IF LPAREN expr(expr) RPAREN closed_statement(ifTrue)
                      ELSE open_statement(ifFalse).                             { printf("s%d = f.stmtIfElse(e%d, s%d, s%d)\n", r = ++S, expr, ifTrue, ifFalse); }
open_statement(r) ::= WHILE LPAREN expr(expr) RPAREN statement(body).           { printf("s%d = f.stmtWhile(e%d, s%d)\n", r = ++S, expr, body); }
open_statement(r) ::= FOR LPAREN             SEMI             SEMI             RPAREN statement(body).
                                                                                { printf("s%d = f.stmtFor(None, None, None, s%d)\n", r = ++S,                      body); }
open_statement(r) ::= FOR LPAREN             SEMI             SEMI expr(expr3) RPAREN statement(body).
                                                                                { printf("s%d = f.stmtFor(None, None, e%d,  s%d)\n", r = ++S,               expr3, body); }
open_statement(r) ::= FOR LPAREN             SEMI expr(expr2) SEMI             RPAREN statement(body).
                                                                                { printf("s%d = f.stmtFor(None, e%d,  None, s%d)\n", r = ++S,        expr2,        body); }
open_statement(r) ::= FOR LPAREN             SEMI expr(expr2) SEMI expr(expr3) RPAREN statement(body).
                                                                                { printf("s%d = f.stmtFor(None, e%d,  e%d,  s%d)\n", r = ++S,        expr2, expr3, body); }
open_statement(r) ::= FOR LPAREN expr(expr1) SEMI             SEMI             RPAREN statement(body).
                                                                                { printf("s%d = f.stmtFor(e%d,  None, None, s%d)\n", r = ++S, expr1,               body); }
open_statement(r) ::= FOR LPAREN expr(expr1) SEMI             SEMI expr(expr3) RPAREN statement(body).
                                                                                { printf("s%d = f.stmtFor(e%d,  None, e%d,  s%d)\n", r = ++S, expr1,        expr3, body); }
open_statement(r) ::= FOR LPAREN expr(expr1) SEMI expr(expr2) SEMI             RPAREN statement(body).
                                                                                { printf("s%d = f.stmtFor(e%d,  e%d,  None, s%d)\n", r = ++S, expr1, expr2,        body); }
open_statement(r) ::= FOR LPAREN expr(expr1) SEMI expr(expr2) SEMI expr(expr3) RPAREN statement(body).
                                                                                { printf("s%d = f.stmtFor(e%d,  e%d,  e%d,  s%d)\n", r = ++S, expr1, expr2, expr3, body); }

closed_statement(r) ::= IF LPAREN expr(expr) RPAREN closed_statement(ifTrue)
                        ELSE closed_statement(ifFalse).                         { printf("s%d = f.stmtIfElse(e%d, s%d, s%d)\n", r = ++S, expr, ifTrue, ifFalse); }
closed_statement(r) ::= SEMI.                                                   { printf("s%d = f.stmtExpr(None)\n", r = ++S); }
closed_statement(r) ::= expr(expr) SEMI.                                        { printf("s%d = f.stmtExprOrDefVar(e%d)\n", r = ++S, expr); }
closed_statement(r) ::= compound_statement(stmt).                               { r = stmt; }
closed_statement(r) ::= DO statement(body) WHILE LPAREN expr(expr) RPAREN SEMI. { printf("s%d = f.stmtDoWhile(s%d, e%d)\n", r = ++S, body, expr); }
closed_statement(r) ::= CONTINUE SEMI.                                          { printf("s%d = f.stmtContinue()\n", r = ++S); }
closed_statement(r) ::= BREAK SEMI.                                             { printf("s%d = f.stmtBreak()\n", r = ++S); }
closed_statement(r) ::= RETURN            SEMI.                                 { printf("s%d = f.stmtReturn(None)\n", r = ++S); }
closed_statement(r) ::= RETURN expr(expr) SEMI.                                 { printf("s%d = f.stmtReturn(e%d)\n", r = ++S, expr); }
closed_statement(r) ::= YIELD            SEMI.                                  { printf("s%d = f.stmtYield(None)\n", r = ++S); }
closed_statement(r) ::= YIELD expr(expr) SEMI.                                  { printf("s%d = f.stmtYield(e%d)\n", r = ++S, expr); }
closed_statement(r) ::= expr(expr) compound_statement(stmt).                    { printf("s%d = f.stmtDefMethod(e%d, s%d)\n", r = ++S, expr, stmt); }
closed_statement(r) ::= CLASS IDENTIFIER(name) class_body(body).                { printf("s%d = f.stmtDefClass(t%d, None, tmp%d[0], tmp%d[1])\n", r = ++S, name, body, body); }
closed_statement(r) ::= CLASS SPECIFIER(name) class_body(body).                 { printf("s%d = f.stmtDefClass(t%d, None, tmp%d[0], tmp%d[1])\n", r = ++S, name, body, body); }
closed_statement(r) ::= CLASS IDENTIFIER(name) COLON IDENTIFIER(super) class_body(body).
                                                                                { printf("s%d = f.stmtDefClass(t%d, t%d,  tmp%d[0], tmp%d[1])\n", r = ++S, name, super, body, body); }
closed_statement(r) ::= CLASS IDENTIFIER(name) COLON SPECIFIER(super) class_body(body).
                                                                                { printf("s%d = f.stmtDefClass(t%d, t%d,  tmp%d[0], tmp%d[1])\n", r = ++S, name, super, body, body); }
closed_statement(r) ::= CLASS SPECIFIER(name) COLON IDENTIFIER(super) class_body(body).
                                                                                { printf("s%d = f.stmtDefClass(t%d, t%d,  tmp%d[0], tmp%d[1])\n", r = ++S, name, super, body, body); }
closed_statement(r) ::= CLASS SPECIFIER(name) COLON SPECIFIER(super) class_body(body).
                                                                                { printf("s%d = f.stmtDefClass(t%d, t%d,  tmp%d[0], tmp%d[1])\n", r = ++S, name, super, body, body); }

class_body(r) ::= compound_statement(body).                                     { printf("tmp%d = (s%d, None)\n", r = ++TMP, body); }
class_body(r) ::= compound_statement(body) META compound_statement(metaBody).   { printf("tmp%d = (s%d,  s%d)\n", r = ++TMP, body, metaBody); } 

compound_statement(r) ::= LCURLY                          RCURLY.               { printf("s%d = f.stmtCompound([])\n", r = ++S); }
compound_statement(r) ::= LCURLY statement_list(stmtList) RCURLY.               { printf("s%d = f.stmtCompound(l%d)\n", r = ++S, stmtList); }

expr(r) ::= comma_expr(expr).                                                   { r = expr; }
expr(r) ::= decl_spec_list(declSpecList) comma_expr(expr).                      { r = expr; printf("e%d.declSpecs = l%d\n", r, declSpecList); }

decl_spec_list(r) ::= decl_spec(declSpec).                                      { printf("l%d = [e%d]\n", r = ++L, declSpec); }
decl_spec_list(r) ::= decl_spec_list(declSpecList) decl_spec(declSpec).         { r = declSpecList; printf("l%d.append(e%d)\n", r, declSpec); }

decl_spec(r) ::= SPECIFIER(name).                                               { printf("e%d = f.exprName(t%d)\n", r = ++E, name); }

comma_expr(r) ::= colon_expr(expr).                                             { r = expr; }
comma_expr(r) ::= comma_expr(left) COMMA colon_expr(right).                     { printf("e%d = e%d.comma(e%d)\n", r = ++E, left, right); }

colon_expr(r) ::= assignment_expr(expr).                                        { r = expr; }
//colon_expr(r) ::= colon_expr COLON assignment_expr(expr).                       { r = expr; }

assignment_expr(r) ::= keyword_expr(expr).                                      { r = expr; }
assignment_expr(r) ::= unary_expr(left) ASSIGN        assignment_expr(right).   { printf("e%d = f.exprAssign(f.operEq,     e%d, e%d)\n", r = ++E, left, right); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_TIMES  assignment_expr(right).   { printf("e%d = f.exprAssign(f.operMul,    e%d, e%d)\n", r = ++E, left, right); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_DIVIDE assignment_expr(right).   { printf("e%d = f.exprAssign(f.operDiv,    e%d, e%d)\n", r = ++E, left, right); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_MOD    assignment_expr(right).   { printf("e%d = f.exprAssign(f.operMod,    e%d, e%d)\n", r = ++E, left, right); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_PLUS   assignment_expr(right).   { printf("e%d = f.exprAssign(f.operAdd,    e%d, e%d)\n", r = ++E, left, right); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_MINUS  assignment_expr(right).   { printf("e%d = f.exprAssign(f.operSub,    e%d, e%d)\n", r = ++E, left, right); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_LSHIFT assignment_expr(right).   { printf("e%d = f.exprAssign(f.operLShift, e%d, e%d)\n", r = ++E, left, right); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_RSHIFT assignment_expr(right).   { printf("e%d = f.exprAssign(f.operRShift, e%d, e%d)\n", r = ++E, left, right); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_BAND   assignment_expr(right).   { printf("e%d = f.exprAssign(f.operBAnd,   e%d, e%d)\n", r = ++E, left, right); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_BXOR   assignment_expr(right).   { printf("e%d = f.exprAssign(f.operBXOr,   e%d, e%d)\n", r = ++E, left, right); }
assignment_expr(r) ::= unary_expr(left) ASSIGN_BOR    assignment_expr(right).   { printf("e%d = f.exprAssign(f.operBOr,    e%d, e%d)\n", r = ++E, left, right); }

keyword_expr(r) ::= conditional_expr(expr).                                     { r = expr; }
keyword_expr(r) ::= conditional_expr(receiver) keyword_message(message).        { printf("e%d = f.exprKeyword(e%d, *tmp%d)\n", r = ++E, receiver, message); }

keyword_message(r) ::= keyword_list(kwList).                                    { printf("tmp%d = (None, tmp%d[0], tmp%d[1])\n", r = ++TMP,     kwList, kwList); }
keyword_message(r) ::= keyword(kw).                                             { printf("tmp%d = (t%d,  None,     None    )\n", r = ++TMP, kw                ); }
keyword_message(r) ::= keyword(kw) keyword_list(kwList).                        { printf("tmp%d = (t%d,  tmp%d[0], tmp%d[1])\n", r = ++TMP, kw, kwList, kwList); }

keyword_list(r) ::= keyword(kw) COLON conditional_expr(arg).                    { printf("tmp%d = ([t%d], [e%d])\n", r = ++TMP, kw, arg); }
keyword_list(r) ::= keyword_list(kwList) keyword(kw) COLON conditional_expr(arg).
                                                                                { r = kwList; printf("tmp%d[0].append(t%d); tmp%d[1].append(e%d)\n", r, kw, r, arg); }

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
                                                                                { printf("e%d = f.exprCond(e%d, e%d, e%d)\n", r = ++E, cond, left, right); }

%left OR.
%left AND.

logical_expr(r) ::= binary_expr(expr).                                          { r = expr; }
logical_expr(r) ::= logical_expr(left) OR logical_expr(right).                  { printf("e%d = f.exprOr(e%d, e%d)\n", r = ++E, left, right); }
logical_expr(r) ::= logical_expr(left) AND logical_expr(right).                 { printf("e%d = f.exprAnd(e%d, e%d)\n", r = ++E, left, right); }

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
binary_expr(r) ::= binary_expr(left) ID binary_expr(right).                     { printf("e%d = f.exprId(e%d, e%d)\n", r = ++E, left, right); }
binary_expr(r) ::= binary_expr(left) NI binary_expr(right).                     { printf("e%d = f.exprNI(e%d, e%d)\n", r = ++E, left, right); }
binary_expr(r) ::= binary_expr(left) EQ binary_expr(right).                     { printf("e%d = f.exprBinary(f.operEq,     e%d, e%d)\n", r = ++E, left, right); }
binary_expr(r) ::= binary_expr(left) NE binary_expr(right).                     { printf("e%d = f.exprBinary(f.operNE,     e%d, e%d)\n", r = ++E, left, right); }
binary_expr(r) ::= binary_expr(left) GT binary_expr(right).                     { printf("e%d = f.exprBinary(f.operGT,     e%d, e%d)\n", r = ++E, left, right); }
binary_expr(r) ::= binary_expr(left) GE binary_expr(right).                     { printf("e%d = f.exprBinary(f.operGE,     e%d, e%d)\n", r = ++E, left, right); }
binary_expr(r) ::= binary_expr(left) LT binary_expr(right).                     { printf("e%d = f.exprBinary(f.operLT,     e%d, e%d)\n", r = ++E, left, right); }
binary_expr(r) ::= binary_expr(left) LE binary_expr(right).                     { printf("e%d = f.exprBinary(f.operLE,     e%d, e%d)\n", r = ++E, left, right); }
binary_expr(r) ::= binary_expr(left) BOR binary_expr(right).                    { printf("e%d = f.exprBinary(f.operBOr,    e%d, e%d)\n", r = ++E, left, right); }
binary_expr(r) ::= binary_expr(left) BXOR binary_expr(right).                   { printf("e%d = f.exprBinary(f.operBXOr,   e%d, e%d)\n", r = ++E, left, right); }
binary_expr(r) ::= binary_expr(left) AMP binary_expr(right).                    { printf("e%d = f.exprBinary(f.operBAnd,   e%d, e%d)\n", r = ++E, left, right); }
binary_expr(r) ::= binary_expr(left) LSHIFT binary_expr(right).                 { printf("e%d = f.exprBinary(f.operLShift, e%d, e%d)\n", r = ++E, left, right); }
binary_expr(r) ::= binary_expr(left) RSHIFT binary_expr(right).                 { printf("e%d = f.exprBinary(f.operRShift, e%d, e%d)\n", r = ++E, left, right); }
binary_expr(r) ::= binary_expr(left) PLUS binary_expr(right).                   { printf("e%d = f.exprBinary(f.operAdd,    e%d, e%d)\n", r = ++E, left, right); }
binary_expr(r) ::= binary_expr(left) MINUS binary_expr(right).                  { printf("e%d = f.exprBinary(f.operSub,    e%d, e%d)\n", r = ++E, left, right); }
binary_expr(r) ::= binary_expr(left) TIMES binary_expr(right).                  { printf("e%d = f.exprBinary(f.operMul,    e%d, e%d)\n", r = ++E, left, right); }
binary_expr(r) ::= binary_expr(left) DIVIDE binary_expr(right).                 { printf("e%d = f.exprBinary(f.operDiv,    e%d, e%d)\n", r = ++E, left, right); }
binary_expr(r) ::= binary_expr(left) MOD binary_expr(right).                    { printf("e%d = f.exprBinary(f.operMod,    e%d, e%d)\n", r = ++E, left, right); }
binary_expr(r) ::= binary_expr(left) DOT_STAR binary_expr(right).               { printf("e%d = f.exprAttrVar(e%d, e%d)\n", r = ++E, left, right); }

unary_expr(r) ::= postfix_expr(expr).                                           { r = expr; }
unary_expr(r) ::= INC   unary_expr(expr).                                       { printf("e%d = f.exprPreOp(f.operSucc, e%d)\n", r = ++E, expr); }
unary_expr(r) ::= DEC   unary_expr(expr).                                       { printf("e%d = f.exprPreOp(f.operPred, e%d)\n", r = ++E, expr); }
unary_expr(r) ::= PLUS  unary_expr(expr).                                       { printf("e%d = f.exprUnary(f.operPos,  e%d)\n", r = ++E, expr); }
unary_expr(r) ::= MINUS unary_expr(expr).                                       { printf("e%d = f.exprUnary(f.operNeg,  e%d)\n", r = ++E, expr); }
unary_expr(r) ::= TIMES unary_expr(expr).                                       { printf("e%d = f.exprUnary(f.operInd,  e%d)\n", r = ++E, expr); }
unary_expr(r) ::= BNEG  unary_expr(expr).                                       { printf("e%d = f.exprUnary(f.operBNeg, e%d)\n", r = ++E, expr); }
unary_expr(r) ::= LNEG  unary_expr(expr).                                       { printf("e%d = f.exprUnary(f.operLNeg, e%d)\n", r = ++E, expr); }

postfix_expr(r) ::= primary_expr(expr).                                         { r = expr; }
postfix_expr(r) ::= postfix_expr(func) LBRACK argument_list(args) RBRACK.       { printf("e%d = f.exprCall(f.operIndex, e%d, tmp%d[0], tmp%d[1])\n", r = ++E, func, args, args); }
postfix_expr(r) ::= postfix_expr(func) LPAREN RPAREN.                           { printf("e%d = f.exprCall(f.operApply, e%d, [],       [],     )\n", r = ++E, func            ); }
postfix_expr(r) ::= postfix_expr(func) LPAREN argument_list(args) RPAREN.       { printf("e%d = f.exprCall(f.operApply, e%d, tmp%d[0], tmp%d[1])\n", r = ++E, func, args, args); }
postfix_expr(r) ::= postfix_expr(obj) DOT IDENTIFIER(attr).                     { printf("e%d = f.exprAttr(e%d, t%d)\n", r = ++E, obj,  attr); }
postfix_expr(r) ::= postfix_expr(obj) DOT SPECIFIER(attr).                      { printf("e%d = f.exprAttr(e%d, t%d)\n", r = ++E, obj,  attr); }
postfix_expr(r) ::= postfix_expr(obj) DOT CLASS(attr).                          { printf("e%d = f.exprAttr(e%d, t%d)\n", r = ++E, obj,  attr); }
postfix_expr(r) ::= SPECIFIER(name) DOT IDENTIFIER(attr).                       { printf("e%d = f.exprAttr(t%d, t%d)\n", r = ++E, name, attr); }
postfix_expr(r) ::= SPECIFIER(name) DOT CLASS(attr).                            { printf("e%d = f.exprAttr(t%d, t%d)\n", r = ++E, name, attr); }
postfix_expr(r) ::= postfix_expr(expr) INC.                                     { printf("e%d = f.exprPostOp(f.operSucc, e%d)\n", r = ++E, expr); }
postfix_expr(r) ::= postfix_expr(expr) DEC.                                     { printf("e%d = f.exprPostOp(f.operPred, e%d)\n", r = ++E, expr); }

primary_expr(r) ::= IDENTIFIER(name).                                           { printf("e%d = f.exprName(t%d)\n", r = ++E, name); }
primary_expr(r) ::= LITERAL_SYMBOL(token).                                      { printf("e%d = f.exprLiteral(t%d)\n", r = ++E, token); }
primary_expr(r) ::= LITERAL_INT(token).                                         { printf("e%d = f.exprLiteral(t%d)\n", r = ++E, token); }
primary_expr(r) ::= LITERAL_FLOAT(token).                                       { printf("e%d = f.exprLiteral(t%d)\n", r = ++E, token); }
primary_expr(r) ::= LITERAL_CHAR(token).                                        { printf("e%d = f.exprLiteral(t%d)\n", r = ++E, token); }
primary_expr(r) ::= literal_string(expr).                                       { r = expr; }
primary_expr(r) ::= LPAREN expr(expr) RPAREN.                                   { r = expr; }
primary_expr(r) ::= LPAREN SPECIFIER(name) RPAREN.                              { printf("e%d = f.exprName(t%d)\n", r = ++E, name); }
primary_expr(r) ::= block(expr).                                                { r = expr; }
//primary_expr(r) ::= LCURLY expr(expr) RCURLY.                                   { printf("e%d = f.exprCompound(e%d)\n", r = ++E, expr); }

literal_string(r) ::= LITERAL_STR(token).                                       { printf("e%d = f.exprLiteral(t%d)\n", r = ++E, token); }
literal_string(r) ::= literal_string(expr) LITERAL_STR(token).                  { r = expr; printf("e%d.concat(t%d)\n", r, token); }

block(r) ::= LBRACK statement_list_expr(sle) RBRACK.                            { printf("e%d = f.exprBlock(None, tmp%d[0], tmp%d[1])\n", r = ++E, sle, sle); }
block(r) ::= LBRACK block_argument_list(args) BOR statement_list_expr(sle) RBRACK.
                                                                                { printf("e%d = f.exprBlock(l%d, tmp%d[0], tmp%d[1])\n", r = ++E, args, sle, sle); }

block_argument_list(r) ::= COLON block_arg(arg).                                { printf("l%d = [e%d]\n", r = ++L, arg); }
block_argument_list(r) ::= block_argument_list(args) COLON block_arg(arg).      { r = args; printf("l%d.append(e%d)\n", r, arg); }

block_arg(r) ::= unary_expr(arg).                                               { r = arg; }
block_arg(r) ::= decl_spec_list(declSpecList) unary_expr(arg).                  { r = arg; printf("e%d.declSpecs = l%d\n", r, declSpecList); }

argument_list(r) ::= fixed_arg_list(f).                                         { printf("tmp%d = (l%d,  [])\n", r = ++TMP, f   ); }
argument_list(r) ::= fixed_arg_list(f) COMMA ELLIPSIS assignment_expr(v).       { printf("tmp%d = (l%d, e%d)\n", r = ++TMP, f, v); }
argument_list(r) ::= ELLIPSIS assignment_expr(v).                               { printf("tmp%d = ([],  e%d)\n", r = ++TMP,    v); }

fixed_arg_list(r) ::= arg(arg).                                                 { printf("l%d = [e%d]\n", r = ++L, arg); }
fixed_arg_list(r) ::= fixed_arg_list(args) COMMA arg(arg).                      { r = args; printf("l%d.append(e%d)\n", r, arg); }

arg(r) ::= colon_expr(arg).                                                     { r = arg; }
arg(r) ::= decl_spec_list(declSpecList) colon_expr(arg).                        { r = arg; printf("e%d.declSpecs = l%d\n", r, declSpecList); }


%syntax_error {
    fprintf(stderr, "syntax error!\n");
}

%parse_accept {
    /*printf("parsing complete!\n");*/
}

%parse_failure {
    fprintf(stderr,"Giving up.  Parser is hopelessly lost...\n");
}
