
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

#include "float.h"
#include "int.h"
#include "io.h"
#include "str.h"

#include <stdio.h>


#define CLASS_TMPL(c) Spk_Class ## c ## Tmpl
#define CLASS(c, s) &CLASS_TMPL(c)

SpkClassBootRec Spk_classBootRec[] = {
    /***CLASS(VariableObject, Object),*/
    /******/CLASS(String,  VariableObject),
    /**/CLASS(Integer,    Object),
    /**/CLASS(Float,      Object),
    /**/CLASS(FileStream, Object),
    0
};


SpkModuleTmpl Spk_ModulemoduleTmpl;

struct SpkModule *Spk_heart;


int main(int argc, char **argv) {
    int i;
    char *pathname;
    const char *outputFilename = "generatedcxx.cc";
    FILE *stream, *out;
    
    SpkUnknown *notifier, *tmp;
    SpkSymbolTable *st;
    SpkStmt *tree, **treeTail, *s;
    
    if (argc < 2) {
        fprintf(stderr, "Spike Compiler\n");
        fprintf(stderr, "usage: %s FILE ...\n", argv[0]);
        return 1;
    }
    
    if (!Spk_Boot())
        return 1;
    
    notifier = Spk_CallAttr(Spk_GLOBAL(theInterpreter),
                            (SpkUnknown *)Spk_CLASS(Notifier),
                            Spk_new,
                            Spk_GLOBAL(xstderr),
                            0);
    if (!notifier)
        return 1;
    
    st = SpkSymbolTable_New();
    if (!st)
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
    
        *treeTail = SpkParser_ParseFileStream(stream, st);
        if (!treeTail)
            return 1;
        
        tmp = SpkHost_StringFromCString(pathname);
        SpkParser_Source(treeTail, tmp);
        
        for (s = *treeTail; s->next; s = s->next)
            ;
        treeTail = &s->next;
    }
    if (!tree)
        return 1;
    tree = SpkParser_NewModuleDef(tree);
    if (!tree)
        return 1;
    
    tmp = SpkStaticChecker_Check(tree, st, notifier);
    if (!tmp)
        return 1;
    
    tmp = Spk_Keyword(Spk_GLOBAL(theInterpreter), notifier, Spk_failOnError, 0);
    if (!tmp)
        return 1;
    
    out = fopen(outputFilename, "w");
    if (!out) {
        fprintf(stderr, "%s: cannot open '%s'\n", argv[0], outputFilename);
        return 1;
    }
    SpkCxxCodeGen_GenerateCode(tree, stdout /*out*/ );
    fclose(out);
    
    return 0;
}
