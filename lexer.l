
%option reentrant noyywrap
%option prefix="SpkLexer_"

D			[0-9]
L			[a-zA-Z_]
UL			[A-Z]
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
#include "gram.h"
#include "host.h"
#include "lexer.h"
#include "st.h"

static void count(yyscan_t yyscanner);
static void comment(yyscan_t yyscanner);
static void lcomment(yyscan_t yyscanner);
%}

%%
"/*"			{ comment(yyscanner); }
"//"			{ lcomment(yyscanner); }

"arg"			{ count(yyscanner); return Spk_TOKEN_ARG; }
"break"			{ count(yyscanner); return Spk_TOKEN_BREAK; }
"class"			{ count(yyscanner); return Spk_TOKEN_CLASS; }
"continue"		{ count(yyscanner); return Spk_TOKEN_CONTINUE; }
"do"			{ count(yyscanner); return Spk_TOKEN_DO; }
"else"			{ count(yyscanner); return Spk_TOKEN_ELSE; }
"for"			{ count(yyscanner); return Spk_TOKEN_FOR; }
"from"			{ count(yyscanner); return Spk_TOKEN_FROM; }
"if"			{ count(yyscanner); return Spk_TOKEN_IF; }
"import"		{ count(yyscanner); return Spk_TOKEN_IMPORT; }
"meta"			{ count(yyscanner); return Spk_TOKEN_META; }
"raise"			{ count(yyscanner); return Spk_TOKEN_RAISE; }
"return"		{ count(yyscanner); return Spk_TOKEN_RETURN; }
"var"			{ count(yyscanner); return Spk_TOKEN_VAR; }
"while"			{ count(yyscanner); return Spk_TOKEN_WHILE; }
"yield"			{ count(yyscanner); return Spk_TOKEN_YIELD; }

{LL}({L}|{D})*		{ count(yyscanner); return Spk_TOKEN_IDENTIFIER; }
{UL}({L}|{D})*		{ count(yyscanner); return Spk_TOKEN_TYPE_IDENTIFIER; }
${L}({L}|{D})*		{ count(yyscanner); return Spk_TOKEN_SYMBOL; }

0[xX]{H}+{IS}?		{ count(yyscanner); return Spk_TOKEN_INT; }
0{D}+{IS}?		{ count(yyscanner); return Spk_TOKEN_INT; }
{D}+{IS}?		{ count(yyscanner); return Spk_TOKEN_INT; }
{D}+{E}{FS}?		{ count(yyscanner); return Spk_TOKEN_FLOAT; }
{D}*"."{D}+({E})?{FS}?	{ count(yyscanner); return Spk_TOKEN_FLOAT; }
{D}+"."{D}*({E})?{FS}?	{ count(yyscanner); return Spk_TOKEN_FLOAT; }
'(\\.|[^\\'])+'		{ count(yyscanner); return Spk_TOKEN_CHAR; }
\"(\\.|[^\\"])*\"	{ count(yyscanner); return Spk_TOKEN_STR; }

"..."			{ count(yyscanner); return Spk_TOKEN_ELLIPSIS; }
"==="			{ count(yyscanner); return Spk_TOKEN_ID; }
"!=="			{ count(yyscanner); return Spk_TOKEN_NI; }
">>="			{ count(yyscanner); return Spk_TOKEN_ASSIGN_RSHIFT; }
"<<="			{ count(yyscanner); return Spk_TOKEN_ASSIGN_LSHIFT; }
"+="			{ count(yyscanner); return Spk_TOKEN_ASSIGN_PLUS; }
"-="			{ count(yyscanner); return Spk_TOKEN_ASSIGN_MINUS; }
"*="			{ count(yyscanner); return Spk_TOKEN_ASSIGN_TIMES; }
"/="			{ count(yyscanner); return Spk_TOKEN_ASSIGN_DIVIDE; }
"%="			{ count(yyscanner); return Spk_TOKEN_ASSIGN_MOD; }
"&="			{ count(yyscanner); return Spk_TOKEN_ASSIGN_BAND; }
"^="			{ count(yyscanner); return Spk_TOKEN_ASSIGN_BXOR; }
"|="			{ count(yyscanner); return Spk_TOKEN_ASSIGN_BOR; }
">>"			{ count(yyscanner); return Spk_TOKEN_RSHIFT; }
"<<"			{ count(yyscanner); return Spk_TOKEN_LSHIFT; }
"++"			{ count(yyscanner); return Spk_TOKEN_INC; }
"--"			{ count(yyscanner); return Spk_TOKEN_DEC; }
"&&"			{ count(yyscanner); return Spk_TOKEN_AND; }
"||"			{ count(yyscanner); return Spk_TOKEN_OR; }
"<="			{ count(yyscanner); return Spk_TOKEN_LE; }
">="			{ count(yyscanner); return Spk_TOKEN_GE; }
"=="			{ count(yyscanner); return Spk_TOKEN_EQ; }
"!="			{ count(yyscanner); return Spk_TOKEN_NE; }
".*"			{ count(yyscanner); return Spk_TOKEN_DOT_STAR; }
";"			{ count(yyscanner); return Spk_TOKEN_SEMI; }
"{"			{ count(yyscanner); return Spk_TOKEN_LCURLY; }
"}"			{ count(yyscanner); return Spk_TOKEN_RCURLY; }
","			{ count(yyscanner); return Spk_TOKEN_COMMA; }
":"			{ count(yyscanner); return Spk_TOKEN_COLON; }
"="			{ count(yyscanner); return Spk_TOKEN_ASSIGN; }
"["			{ count(yyscanner); return Spk_TOKEN_LBRACK; }
"]"			{ count(yyscanner); return Spk_TOKEN_RBRACK; }
"("			{ count(yyscanner); return Spk_TOKEN_LPAREN; }
")"			{ count(yyscanner); return Spk_TOKEN_RPAREN; }
"."			{ count(yyscanner); return Spk_TOKEN_DOT; }
"&"			{ count(yyscanner); return Spk_TOKEN_AMP; }
"!"			{ count(yyscanner); return Spk_TOKEN_LNEG; }
"~"			{ count(yyscanner); return Spk_TOKEN_BNEG; }
"-"			{ count(yyscanner); return Spk_TOKEN_MINUS; }
"+"			{ count(yyscanner); return Spk_TOKEN_PLUS; }
"*"			{ count(yyscanner); return Spk_TOKEN_TIMES; }
"/"			{ count(yyscanner); return Spk_TOKEN_DIVIDE; }
"%"			{ count(yyscanner); return Spk_TOKEN_MOD; }
"<"			{ count(yyscanner); return Spk_TOKEN_LT; }
">"			{ count(yyscanner); return Spk_TOKEN_GT; }
"^"			{ count(yyscanner); return Spk_TOKEN_BXOR; }
"|"			{ count(yyscanner); return Spk_TOKEN_BOR; }
"?"			{ count(yyscanner); return Spk_TOKEN_QM; }

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

static SpkUnknown *intFromLiteral(char *str, size_t len) {
    return SpkHost_IntegerFromCLong(strtol(str, 0, 0));
}

static SpkUnknown *floatFromLiteral(char *str, size_t len) {
    return SpkHost_FloatFromCDouble(strtod(str, 0));
}

static SpkUnknown *charFromLiteral(char *str, size_t len) {
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
    
    return (SpkUnknown *)SpkChar_FromChar(result);
}

static SpkUnknown *stringFromLiteral(char *str, size_t len) {
    SpkUnknown *result;
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
        result = SpkHost_StringFromCStringAndLength(0, resultLen);
        d = SpkHost_StringAsCString(result);
        goto again;
    }

    return result;
}

int SpkLexer_GetNextToken(SpkToken *token, yyscan_t scanner, SpkSymbolTable *st) {
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
    case Spk_TOKEN_INT:
        token->literalValue = intFromLiteral(yyget_text(scanner), yyget_leng(scanner));
        break;
    case Spk_TOKEN_FLOAT:
        token->literalValue = floatFromLiteral(yyget_text(scanner), yyget_leng(scanner));
        break;
    case Spk_TOKEN_CHAR:
        token->literalValue = charFromLiteral(yyget_text(scanner), yyget_leng(scanner));
        break;
    case Spk_TOKEN_STR:
        token->literalValue = stringFromLiteral(yyget_text(scanner), yyget_leng(scanner));
        break;
    case Spk_TOKEN_IDENTIFIER:
    case Spk_TOKEN_TYPE_IDENTIFIER:
        token->sym = SpkSymbolNode_FromString(st, yyget_text(scanner));
        break;
    case Spk_TOKEN_SYMBOL:
        token->sym = SpkSymbolNode_FromString(st, yyget_text(scanner) + 1);
        break;
    }
    return id;
}
