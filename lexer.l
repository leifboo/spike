
%option reentrant noyywrap
%option prefix="SpkLexer_"

D			[0-9]
L			[a-zA-Z_]
UL			[A-Z_]
LL			[a-z_]
H			[a-fA-F0-9]
E			[Ee][+-]?{D}+
FS			(f|F|l|L)
IS			(u|U|l|L)*

%{
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include "char.h"
#include "float.h"
#include "gram.h"
#include "lexer.h"
#include "st.h"
#include "str.h"
#include "sym.h"

static void count(yyscan_t yyscanner);
static void comment(yyscan_t yyscanner);
static void lcomment(yyscan_t yyscanner);
%}

%%
"/*"			{ comment(yyscanner); }
"//"			{ lcomment(yyscanner); }

"class"			{ count(yyscanner); return TOKEN_CLASS; }
"break"			{ count(yyscanner); return TOKEN_BREAK; }
"continue"		{ count(yyscanner); return TOKEN_CONTINUE; }
"do"			{ count(yyscanner); return TOKEN_DO; }
"else"			{ count(yyscanner); return TOKEN_ELSE; }
"for"			{ count(yyscanner); return TOKEN_FOR; }
"foreach"		{ count(yyscanner); return TOKEN_FOREACH; }
"if"			{ count(yyscanner); return TOKEN_IF; }
"return"		{ count(yyscanner); return TOKEN_RETURN; }
"var"			{ count(yyscanner); return TOKEN_VAR; }
"while"			{ count(yyscanner); return TOKEN_WHILE; }

{LL}({L}|{D})*		{ count(yyscanner); return TOKEN_IDENTIFIER; }
{UL}({L}|{D})*		{ count(yyscanner); return TOKEN_TYPE_IDENTIFIER; }
$({L}|{D})*		{ count(yyscanner); return TOKEN_SYMBOL; }

0[xX]{H}+{IS}?		{ count(yyscanner); return TOKEN_INT; }
0{D}+{IS}?		{ count(yyscanner); return TOKEN_INT; }
{D}+{IS}?		{ count(yyscanner); return TOKEN_INT; }
{D}+{E}{FS}?		{ count(yyscanner); return TOKEN_FLOAT; }
{D}*"."{D}+({E})?{FS}?	{ count(yyscanner); return TOKEN_FLOAT; }
{D}+"."{D}*({E})?{FS}?	{ count(yyscanner); return TOKEN_FLOAT; }
'(\\.|[^\\'])+'		{ count(yyscanner); return TOKEN_CHAR; }
\"(\\.|[^\\"])*\"	{ count(yyscanner); return TOKEN_STR; }

"..."			{ count(yyscanner); return TOKEN_ELLIPSIS; }
"==="			{ count(yyscanner); return TOKEN_ID; }
"!=="			{ count(yyscanner); return TOKEN_NI; }
">>="			{ count(yyscanner); return TOKEN_ASSIGN_RSHIFT; }
"<<="			{ count(yyscanner); return TOKEN_ASSIGN_LSHIFT; }
"+="			{ count(yyscanner); return TOKEN_ASSIGN_PLUS; }
"-="			{ count(yyscanner); return TOKEN_ASSIGN_MINUS; }
"*="			{ count(yyscanner); return TOKEN_ASSIGN_TIMES; }
"/="			{ count(yyscanner); return TOKEN_ASSIGN_DIVIDE; }
"%="			{ count(yyscanner); return TOKEN_ASSIGN_MOD; }
"&="			{ count(yyscanner); return TOKEN_ASSIGN_BAND; }
"^="			{ count(yyscanner); return TOKEN_ASSIGN_BXOR; }
"|="			{ count(yyscanner); return TOKEN_ASSIGN_BOR; }
">>"			{ count(yyscanner); return TOKEN_RSHIFT; }
"<<"			{ count(yyscanner); return TOKEN_LSHIFT; }
"++"			{ count(yyscanner); return TOKEN_INC; }
"--"			{ count(yyscanner); return TOKEN_DEC; }
"&&"			{ count(yyscanner); return TOKEN_AND; }
"||"			{ count(yyscanner); return TOKEN_OR; }
"<="			{ count(yyscanner); return TOKEN_LE; }
">="			{ count(yyscanner); return TOKEN_GE; }
"=="			{ count(yyscanner); return TOKEN_EQ; }
"!="			{ count(yyscanner); return TOKEN_NE; }
".*"			{ count(yyscanner); return TOKEN_DOT_STAR; }
";"			{ count(yyscanner); return TOKEN_SEMI; }
"{"			{ count(yyscanner); return TOKEN_LCURLY; }
"}"			{ count(yyscanner); return TOKEN_RCURLY; }
","			{ count(yyscanner); return TOKEN_COMMA; }
":"			{ count(yyscanner); return TOKEN_COLON; }
"="			{ count(yyscanner); return TOKEN_ASSIGN; }
"["			{ count(yyscanner); return TOKEN_LBRACK; }
"]"			{ count(yyscanner); return TOKEN_RBRACK; }
"("			{ count(yyscanner); return TOKEN_LPAREN; }
")"			{ count(yyscanner); return TOKEN_RPAREN; }
"."			{ count(yyscanner); return TOKEN_DOT; }
"&"			{ count(yyscanner); return TOKEN_AMP; }
"!"			{ count(yyscanner); return TOKEN_LNEG; }
"~"			{ count(yyscanner); return TOKEN_BNEG; }
"-"			{ count(yyscanner); return TOKEN_MINUS; }
"+"			{ count(yyscanner); return TOKEN_PLUS; }
"*"			{ count(yyscanner); return TOKEN_TIMES; }
"/"			{ count(yyscanner); return TOKEN_DIVIDE; }
"%"			{ count(yyscanner); return TOKEN_MOD; }
"<"			{ count(yyscanner); return TOKEN_LT; }
">"			{ count(yyscanner); return TOKEN_GT; }
"^"			{ count(yyscanner); return TOKEN_BXOR; }
"|"			{ count(yyscanner); return TOKEN_BOR; }
"?"			{ count(yyscanner); return TOKEN_QM; }

[ \t\v\n\f]		{ count(yyscanner); }
.			{ fprintf(stderr, "bad character in source: '%s'\n", yyget_text(yyscanner)); return -1; }

%%

static void comment(yyscan_t yyscanner)
{
    char c;
    int lineno, column;

    lineno = yyget_lineno(yyscanner);
    column = yyget_column(yyscanner);
    
    column += 2;
    c = input(yyscanner);
    do {
        while (c != '*') {
            if (!c) {
                return;
            } else if (c == '\n') {
                lineno++;
                column = 1;
            } else if (c == '\t') {
                column += 4 - (column % 4);
            } else {
                column++;
            }
            c = input(yyscanner);
        }
        column++;
        c = input(yyscanner);
    } while (c != '/');
    column++;

    yyset_lineno(lineno, yyscanner);
    yyset_column(column, yyscanner);
}

static void lcomment(yyscan_t yyscanner)
{
    char c;

    c = input(yyscanner);
    while (c && c != '\n')
        c = input(yyscanner);
    yyset_lineno(yyget_lineno(yyscanner) + 1, yyscanner);
    yyset_column(1, yyscanner);
}

static void count(yyscan_t yyscanner)
{
    int i;
    char *text;
    int lineno, column;

    text = yyget_text(yyscanner);
    lineno = yyget_lineno(yyscanner);
    column = yyget_column(yyscanner);

#if 0
    yylloc.first_line = lineno;
    yylloc.first_column = column;
#endif

    for (i = 0; text[i] != '\0'; i++)
        if (text[i] == '\n') {
            lineno++;
            column = 1;
        } else if (text[i] == '\t') {
            column += 4 - (column % 4);
        } else {
            column++;
        }

#if 0
    yylloc.last_line = lineno;
    yylloc.last_column = column - 1;
#endif
    yyset_lineno(lineno, yyscanner);
    yyset_column(column, yyscanner);
}

int SpkLexer_GetNextToken(Token *token, yyscan_t scanner) {
    int id;
    struct yyguts_t * yyg = (struct yyguts_t*)scanner;
    
    id = yylex(scanner);
    token->id = id;
    token->sym = 0;
    token->lineNo = yylineno;
    if (id <= 0) {
        return 0;
    }
    switch (id) {
    case TOKEN_INT:
        token->lit.intValue = strtol(yyget_text(scanner), 0, 0);
        break;
    case TOKEN_FLOAT:
        token->lit.floatValue = SpkFloat_fromLiteral(yyget_text(scanner), yyget_leng(scanner));
        break;
    case TOKEN_CHAR:
        token->lit.charValue = SpkChar_fromLiteral(yyget_text(scanner), yyget_leng(scanner));
        break;
    case TOKEN_STR:
        token->lit.strValue = SpkString_fromLiteral(yyget_text(scanner), yyget_leng(scanner));
        break;
    case TOKEN_IDENTIFIER:
    case TOKEN_TYPE_IDENTIFIER:
    case TOKEN_CLASS:
        token->sym = SpkSymbolNode_Get(SpkSymbol_get(yyget_text(scanner)));
        break;
    case TOKEN_SYMBOL:
        token->sym = SpkSymbolNode_Get(SpkSymbol_get(yyget_text(scanner) + 1));
        break;
    }
    return id;
}
