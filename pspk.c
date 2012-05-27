
#include "lexer.h"
#include "parser.h"

#include <stdio.h>
#include <stdlib.h>


static void parse(FILE *yyin) {
    void *parser; yyscan_t lexer;
    char buffer[1024];
    int id, token;
    
    Lexer_lex_init(&lexer);
    Lexer_restart(yyin, lexer);
    (void)fgets(buffer, sizeof(buffer), yyin); /* skip shebang (#!) line */
    Lexer_set_lineno(2, lexer);
    Lexer_set_column(1, lexer);
    
    parser = Parser_ParseAlloc(&malloc);
    
    while ((id = Lexer_GetNextToken(&token, lexer)))
        Parser_Parse(parser, id, token);
    if (id != -1)
        Parser_Parse(parser, 0, token);
    
    Parser_ParseFree(parser, &free);
    Lexer_lex_destroy(lexer);
}


int main(int argc, char **argv) {
    int i;
    char *pathname;
    FILE *stream;
    
    if (argc < 2) {
        fprintf(stderr, "Spike Parser\n");
        fprintf(stderr, "usage: %s FILE ...\n", argv[0]);
        return 1;
    }
    
    for (i = 1; i < argc; ++i) {
        pathname = argv[i];
        stream = fopen(pathname, "r");
        if (!stream) {
            fprintf(stderr, "%s: cannot open '%s'\n",
                    argv[0], pathname);
            return 1;
        }
        
        parse(stream);
        
        fclose(stream);
    }
    
    return 0;
}
