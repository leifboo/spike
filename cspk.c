
#include "boot.h"
#include "cxxgen.h"
#include "heart.h"
#include "host.h"
#include "interp.h"
#include "native.h"
#include "notifier.h"
#include "parser.h"
#include "rodata.h"
#include "scheck.h"
#include "st.h"
#include "tree.h"
#include "x86gen.h"

#include "float.h"
#include "int.h"
#include "io.h"
#include "str.h"

#include <stdio.h>


extern char *kwSep[2];


#define CLASS_TMPL_DEF(c) Class ## c ## Tmpl
#define CBR(c, s) &CLASS_TMPL_DEF(c)

ClassBootRec classBootRec[] = {
    /***CBR(VariableObject, Object),*/
    /******/CBR(String,  VariableObject),
    /**/CBR(Integer,    Object),
    /**/CBR(Float,      Object),
    /**/CBR(FileStream, Object),
    0
};


ModuleTmpl ModulemoduleTmpl = { { "heart" } };

struct Module *heart;


int main(int argc, char **argv) {
    int i;
    char *pathname;
    const char *outputFilename = "generatedcxx.cc";
    FILE *stream, *out;
    
    Unknown *notifier, *tmp;
    SymbolTable *st;
    Stmt *tree, **treeTail, *s;
    
    if (argc < 2) {
        fprintf(stderr, "Spike Compiler\n");
        fprintf(stderr, "usage: %s FILE ...\n", argv[0]);
        return 1;
    }
    
#if 1 /* x86 */
    /* XXX: Hack! */
    kwSep[0] = "$$";
    kwSep[1] = "$";
    declareBuiltIn = 0; /* as in xcspk */
    declareObject = 0;
#endif
    
    if (!Boot())
        return 1;
    
    notifier = Send(GLOBAL(theInterpreter),
                        (Unknown *)CLASS(XNotifier),
                        new,
                        GLOBAL(xstderr),
                        0);
    if (!notifier)
        return 1;
    
    st = SymbolTable_New();
    if (!st)
        return 1;
    tmp = StaticChecker_DeclareBuiltIn(st, notifier);
    if (!tmp)
        return 1;
    
    tree = 0;
    treeTail = &tree;
    for (i = 1; i < argc; ++i) {
        pathname = argv[i];
        stream = fopen(pathname, "r");
        if (!stream) {
            fprintf(stderr, "%s: cannot open '%s'\n",
                    argv[0], pathname);
            return 1;
        }
    
        *treeTail = Parser_ParseFileStream(stream, st);
        if (!treeTail)
            return 1;
        
        tmp = Host_StringFromCString(pathname);
        Parser_Source(treeTail, tmp);
        
        for (s = *treeTail; s->next; s = s->next)
            ;
        treeTail = &s->next;
    }
    if (!tree)
        return 1;
    tree = Parser_NewModuleDef(tree);
    if (!tree)
        return 1;
    
    tmp = StaticChecker_Check(tree, st, notifier);
    if (!tmp)
        return 1;
    
    tmp = Send(GLOBAL(theInterpreter), notifier, failOnError, 0);
    if (!tmp)
        return 1;
    
    out = fopen(outputFilename, "w");
    if (!out) {
        fprintf(stderr, "%s: cannot open '%s'\n", argv[0], outputFilename);
        return 1;
    }
#if 0
    CxxCodeGen_GenerateCode(tree, stdout /*out*/ );
#else
    X86CodeGen_GenerateCode(tree, stdout /*out*/ );
#endif
    fclose(out);
    
    return 0;
}
