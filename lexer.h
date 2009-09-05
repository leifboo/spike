
#ifndef __spk_lexer_h__
#define __spk_lexer_h__


#include "obj.h"
#include <stdio.h>


struct SpkSymbolTable;


typedef struct SpkToken {
    int id;
    struct SpkSymbolNode *sym;
    SpkUnknown *literalValue;
    unsigned int lineNo;
} SpkToken;

#ifndef YY_TYPEDEF_YY_SCANNER_T
#define YY_TYPEDEF_YY_SCANNER_T
typedef void *yyscan_t;
#endif

#ifndef YY_TYPEDEF_YY_BUFFER_STATE
#define YY_TYPEDEF_YY_BUFFER_STATE
typedef struct yy_buffer_state *YY_BUFFER_STATE;
#endif


int SpkLexer_lex_init(yyscan_t *);
void SpkLexer_restart(FILE *, yyscan_t);
YY_BUFFER_STATE SpkLexer__scan_string(const char *, yyscan_t);
void SpkLexer_set_lineno(int, yyscan_t);
int SpkLexer_lex(yyscan_t);
int SpkLexer_lex_destroy(yyscan_t);

int SpkLexer_GetNextToken(SpkToken *, yyscan_t, struct SpkSymbolTable *);


#endif /* __spk_lexer_h__ */
