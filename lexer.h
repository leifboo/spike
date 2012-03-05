
#ifndef __lexer_h__
#define __lexer_h__


#include "obj.h"
#include <stdio.h>


struct SymbolTable;


typedef struct Token {
    int id;
    struct Unknown *value;
    unsigned int lineNo;
} Token;

#ifndef YY_TYPEDEF_YY_SCANNER_T
#define YY_TYPEDEF_YY_SCANNER_T
typedef void *yyscan_t;
#endif

#ifndef YY_TYPEDEF_YY_BUFFER_STATE
#define YY_TYPEDEF_YY_BUFFER_STATE
typedef struct yy_buffer_state *YY_BUFFER_STATE;
#endif


int Lexer_lex_init(yyscan_t *);
void Lexer_restart(FILE *, yyscan_t);
YY_BUFFER_STATE Lexer__scan_string(const char *, yyscan_t);
int Lexer_get_column(yyscan_t);
void Lexer_set_lineno(int, yyscan_t);
void Lexer_set_column(int, yyscan_t);
int Lexer_lex(yyscan_t);
int Lexer_lex_destroy(yyscan_t);

int Lexer_GetNextToken(Token *, yyscan_t, Unknown *);


#endif /* __lexer_h__ */
