
%option reentrant noyywrap
%option prefix="Lexer_"

D			[0-9]
L			[a-zA-Z_]
H			[a-fA-F0-9]
E			[Ee][+-]?{D}+
FS			(f|F|l|L)
IS			(u|U|l|L)*

%{
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include "char.h"
#include "gram.h"
#include "heart.h"
#include "host.h"
#include "lexer.h"
#include "rodata.h"
#include "st.h"
#include "sym.h"

static void count(yyscan_t yyscanner);
static void comment(yyscan_t yyscanner);
static void lcomment(yyscan_t yyscanner);
%}

%%
"/*"			{ comment(yyscanner); }
"//"			{ lcomment(yyscanner); }

"break"			{ count(yyscanner); return TOKEN_BREAK; }
"class"			{ count(yyscanner); return TOKEN_CLASS; }
"continue"		{ count(yyscanner); return TOKEN_CONTINUE; }
"do"			{ count(yyscanner); return TOKEN_DO; }
"else"			{ count(yyscanner); return TOKEN_ELSE; }
"for"			{ count(yyscanner); return TOKEN_FOR; }
"if"			{ count(yyscanner); return TOKEN_IF; }
"meta"			{ count(yyscanner); return TOKEN_META; }
"return"		{ count(yyscanner); return TOKEN_RETURN; }
"while"			{ count(yyscanner); return TOKEN_WHILE; }
"yield"			{ count(yyscanner); return TOKEN_YIELD; }

{L}({L}|{D})*		{ count(yyscanner); return TOKEN_IDENTIFIER; }
${L}({L}|{D})*		{ count(yyscanner); return TOKEN_LITERAL_SYMBOL; }

0[xX]{H}+{IS}?		{ count(yyscanner); return TOKEN_LITERAL_INT; }
0{D}+{IS}?		{ count(yyscanner); return TOKEN_LITERAL_INT; }
{D}+{IS}?		{ count(yyscanner); return TOKEN_LITERAL_INT; }
{D}+{E}{FS}?		{ count(yyscanner); return TOKEN_LITERAL_FLOAT; }
{D}*"."{D}+({E})?{FS}?	{ count(yyscanner); return TOKEN_LITERAL_FLOAT; }
{D}+"."{D}*({E})?{FS}?	{ count(yyscanner); return TOKEN_LITERAL_FLOAT; }
'(\\.|[^\\'])+'		{ count(yyscanner); return TOKEN_LITERAL_CHAR; }
\"(\\.|[^\\"])*\"	{ count(yyscanner); return TOKEN_LITERAL_STR; }

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


#ifndef __cplusplus
#define yyinput input
#endif


static void comment(yyscan_t yyscanner)
{
    char c;
    int lineno, column;

    lineno = yyget_lineno(yyscanner);
    column = yyget_column(yyscanner);
    
    column += 2;
    c = yyinput(yyscanner);
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
            c = yyinput(yyscanner);
        }
        column++;
        c = yyinput(yyscanner);
    } while (c != '/');
    column++;

    yyset_lineno(lineno, yyscanner);
    yyset_column(column, yyscanner);
}

static void lcomment(yyscan_t yyscanner)
{
    char c;

    c = yyinput(yyscanner);
    while (c && c != '\n')
        c = yyinput(yyscanner);
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

static Unknown *intFromLiteral(char *str, size_t len) {
    return Host_IntegerFromCLong(strtol(str, 0, 0));
}

static Unknown *floatFromLiteral(char *str, size_t len) {
    return Host_FloatFromCDouble(strtod(str, 0));
}

static Unknown *charFromLiteral(char *str, size_t len) {
    char *s;
    char c, result;
    
    /* strip quotes */
    s = str + 1; len -= 2;
    
    while (len--) {
        c = *s++;
        /* Note: a trailing '\' is syntactically impossible. */
        if (len && c == '\\') {
            c = *s++;
            --len;
            
            /* XXX: numeric escape codes */
            switch (c) {
            case 'a': c = '\a'; break;
            case 'b': c = '\b'; break;
            case 'f': c = '\f'; break;
            case 'n': c = '\n'; break;
            case 'r': c = '\r'; break;
            case 't': c = '\t'; break;
            case 'v': c = '\v'; break;
                
            case '\\':
            case '\'':
            case '"':
                break;
                
            default:
                /* XXX: error */
                break;
            }
            result = c;
        } else {
            result = c;
        }
    }
    
    return (Unknown *)Char_FromCChar(result);
}

static Unknown *stringFromLiteral(char *str, size_t len) {
    Unknown *result;
    char *d, *s;
    char c;
    size_t resultLen, left;
    
    len -= 2; /* strip quotes */
    
    result = 0;
    d = 0;
    resultLen = 0;
    
 again:
    left = len;
    s = str + 1; /* skip leading quote */
    
    while (left--) {
        c = *s++;
        /* Note: a trailing '\' is syntactically impossible. */
        if (left && c == '\\') {
            c = *s++;
            --left;
            
            /* XXX: numeric escape codes */
            switch (c) {
            case 'a': c = '\a'; break;
            case 'b': c = '\b'; break;
            case 'f': c = '\f'; break;
            case 'n': c = '\n'; break;
            case 'r': c = '\r'; break;
            case 't': c = '\t'; break;
            case 'v': c = '\v'; break;
                
            case '\\':
            case '\'':
            case '"':
                break;
                
            default:
                /* XXX: error */
                break;
            }
            if (d)
                *d++ = c;
            else
                ++resultLen;
        } else {
            if (d)
                *d++ = c;
            else
                ++resultLen;
        }
    }
    
    if (!result) {
        result = Host_StringFromCStringAndLength(0, resultLen);
        d = Host_StringAsCString(result);
        goto again;
    }

    return result;
}

static Unknown *symbolNodeFromCString(Unknown *st, const char *str) {
    Symbol *sym = Symbol_FromCString(str);
    return Send(GLOBAL(theInterpreter), st, symbolNodeForSymbol, sym, 0);
}

static int _isSpec(Unknown *symbolNode) {
    Unknown *bool; int result;
    bool = Send(GLOBAL(theInterpreter), symbolNode, isSpec, 0);
    result = (bool == one || bool == GLOBAL(xtrue));
    DECREF(bool);
    return result;
}

int Lexer_GetNextToken(Token *token, yyscan_t scanner, Unknown *st) {
    int id;
    struct yyguts_t * yyg = (struct yyguts_t*)scanner;
    
    id = yylex(scanner);
    token->id = id;
    token->value = 0;
    token->lineNo = yylineno;
    if (id <= 0) {
        return 0;
    }
    switch (id) {
    case TOKEN_LITERAL_INT:
        token->value = intFromLiteral(yyget_text(scanner), yyget_leng(scanner));
        break;
    case TOKEN_LITERAL_FLOAT:
        token->value = floatFromLiteral(yyget_text(scanner), yyget_leng(scanner));
        break;
    case TOKEN_LITERAL_CHAR:
        token->value = charFromLiteral(yyget_text(scanner), yyget_leng(scanner));
        break;
    case TOKEN_LITERAL_STR:
        token->value = stringFromLiteral(yyget_text(scanner), yyget_leng(scanner));
        break;
    case TOKEN_LITERAL_SYMBOL:
        token->value = (Unknown *)Symbol_FromCString(yyget_text(scanner) + 1);
        break;
    case TOKEN_IDENTIFIER:
        token->value = symbolNodeFromCString(st, yyget_text(scanner));
        if (_isSpec(token->value)) {
            token->id = TOKEN_SPECIFIER;
        }
        break;
    }
    return id;
}
