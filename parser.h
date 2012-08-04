
#ifndef __parser_h__
#define __parser_h__


#include "lexer.h"

#include <stddef.h>
#include <stdio.h>


void Parser_Parse(void *yyp, int yymajor, int yyminor, yyscan_t scanner);
void *Parser_ParseAlloc(void *(*mallocProc)(size_t));
void Parser_ParseFree(void *p, void (*freeProc)(void*));
void Parser_ParseTrace(FILE *TraceFILE, char *zTracePrompt);


#endif /* __parser_h__ */
