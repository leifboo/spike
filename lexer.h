
#ifndef __lexer_h__
#define __lexer_h__


#include <stdio.h>


typedef struct Token {
    int id;
    struct SymbolNode *sym;
    union {
        long intValue;
        struct Float *floatValue;
        struct Char *charValue;
        struct VariableObject *strValue;
    } lit;
    unsigned int lineNo;
} Token;

#ifndef YY_TYPEDEF_YY_SCANNER_T
#define YY_TYPEDEF_YY_SCANNER_T
typedef void *yyscan_t;
#endif


int SpkLexer_lex_init(yyscan_t *);
void SpkLexer_restart(FILE *, yyscan_t);
void SpkLexer_set_lineno(int, yyscan_t);
int SpkLexer_lex(yyscan_t);
int SpkLexer_lex_destroy(yyscan_t);

int SpkLexer_GetNextToken(Token *, yyscan_t);


#endif /* __lexer_h__ */
