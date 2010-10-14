
#include "compiler.h"

#include "cgen.h"
#include "disasm.h"
#include "heart.h"
#include "host.h"
#include "interp.h"
#include "io.h"
#include "module.h"
#include "native.h"
#include "notifier.h"
#include "parser.h"
#include "rodata.h"
#include "scheck.h"
#include "st.h"
#include "tree.h"

#include <string.h>


static SpkUnknown *defaultNotifier(void) {
    return Spk_Send(Spk_GLOBAL(theInterpreter), (SpkUnknown *)Spk_CLASS(Notifier), Spk_new, Spk_GLOBAL(xstderr), 0);
}


static SpkModuleTmpl *compileTree(SpkStmt **pTree, /* destroys reference */
                                  SpkSymbolTable *st,
                                  SpkUnknown *notifier)
{
    SpkStmt *tree;
    SpkUnknown *tmp = 0;
    SpkModuleTmpl *moduleTmpl = 0;
    
    tree = SpkParser_NewModuleDef(*pTree);
    if (!tree) {
        tree = *pTree;
        *pTree = 0;
        goto unwind;
    }
    *pTree = 0;
    
    tmp = SpkStaticChecker_Check(tree, st, notifier);
    if (!tmp)
        goto unwind;
    
    tmp = Spk_Send(Spk_GLOBAL(theInterpreter), notifier, Spk_failOnError, 0);
    if (!tmp)
        goto unwind;
    
    moduleTmpl = SpkCodeGen_GenerateCode(tree);
    
    Spk_DECREF(tmp);
    Spk_DECREF(tree);
    
    return moduleTmpl;
    
 unwind:
    Spk_XDECREF(tmp);
    Spk_DECREF(tree);
    return 0;
}


struct SpkModuleTmpl *SpkCompiler_CompileFileStream(FILE *stream) {
    SpkUnknown *notifier = 0;
    SpkSymbolTable *st = 0;
    SpkStmt *tree = 0;
    SpkModuleTmpl *moduleTmpl = 0;
    SpkUnknown *tmp = 0;
    
    notifier = defaultNotifier();
    if (!notifier)
        goto unwind;
    
    st = SpkSymbolTable_New();
    if (!st)
        goto unwind;
    tmp = SpkStaticChecker_DeclareBuiltIn(st, notifier);
    if (!tmp)
        goto unwind;
    Spk_DECREF(tmp);
    
    tree = SpkParser_ParseFileStream(stream, st);
    if (!tree)
        goto unwind;
    
    moduleTmpl = compileTree(&tree, st, notifier);
    
    Spk_DECREF(notifier);
    Spk_DECREF(st);
    
    return moduleTmpl;
    
 unwind:
    Spk_XDECREF(notifier);
    Spk_XDECREF(st);
    return 0;
}


SpkModuleTmpl *SpkCompiler_CompileFile(const char *pathname) {
    FILE *stream = 0;
    SpkUnknown *notifier = 0;
    SpkSymbolTable *st = 0;
    SpkStmt *tree = 0;
    SpkUnknown *tmp = 0;
    SpkModuleTmpl *moduleTmpl = 0;
    
    stream = fopen(pathname, "r");
    if (!stream) {
        fprintf(stderr, "%s: cannot open '%s'\n",
                /*argv[0]*/ "spike", pathname);
        goto unwind;
    }
    
    notifier = defaultNotifier();
    if (!notifier)
        goto unwind;
    
    st = SpkSymbolTable_New();
    if (!st)
        goto unwind;
    tmp = SpkStaticChecker_DeclareBuiltIn(st, notifier);
    if (!tmp)
        goto unwind;
    Spk_DECREF(tmp);
    
    tree = SpkParser_ParseFileStream(stream, st);
    if (!tree)
        goto unwind;
    
    tmp = SpkHost_StringFromCString(pathname);
    SpkParser_Source(&tree, tmp);
    
    moduleTmpl = compileTree(&tree, st, notifier);
    
    fclose(stream);
    Spk_DECREF(notifier);
    Spk_DECREF(st);
    Spk_DECREF(tmp);
    
    return moduleTmpl;
    
 unwind:
    if (stream)
        fclose(stream);
    Spk_XDECREF(notifier);
    Spk_XDECREF(st);
    Spk_XDECREF(tmp);
    return 0;
}


SpkModuleTmpl *SpkCompiler_CompileString(const char *string) {
    SpkUnknown *notifier = 0;
    SpkSymbolTable *st = 0;
    SpkStmt *tree = 0;
    SpkModuleTmpl *moduleTmpl = 0;
    SpkUnknown *tmp = 0;
    
    notifier = defaultNotifier();
    if (!notifier)
        goto unwind;
    
    st = SpkSymbolTable_New();
    if (!st)
        goto unwind;
    tmp = SpkStaticChecker_DeclareBuiltIn(st, notifier);
    if (!tmp)
        goto unwind;
    Spk_DECREF(tmp);
    
    tree = SpkParser_ParseString(string, st);
    if (!tree)
        goto unwind;
    
    moduleTmpl = compileTree(&tree, st, notifier);
    
    Spk_DECREF(notifier);
    Spk_DECREF(st);
    
    return moduleTmpl;
    
 unwind:
    Spk_XDECREF(notifier);
    Spk_XDECREF(st);
    return 0;
}


SpkModuleTmpl *SpkCompiler_CompileModule(const char *pathname) {
    FILE *moduleStream = 0, *stream = 0;
    SpkUnknown *notifier = 0;
    SpkSymbolTable *st = 0;
    SpkStmt *tree = 0, **treeTail, *s;
    SpkUnknown *tmp = 0;
    SpkModuleTmpl *moduleTmpl = 0;
    char buffer[1024], *b;
    size_t len;
    
    moduleStream = fopen(pathname, "r");
    if (!moduleStream) {
        fprintf(stderr, "%s: cannot open '%s'\n",
                /*argv[0]*/ "spike", pathname);
        goto unwind;
    }
    
    notifier = defaultNotifier();
    if (!notifier)
        goto unwind;
    
    st = SpkSymbolTable_New();
    if (!st)
        goto unwind;
    tmp = SpkStaticChecker_DeclareBuiltIn(st, notifier);
    if (!tmp)
        goto unwind;
    
    treeTail = &tree;
    
    while (b = fgets(buffer, sizeof(buffer), moduleStream)) {
        len = strlen(b);
        if (b[len-1] == '\n')
            b[--len] = '\0';
        
        if (len > 0 && b[0] != '#') {
            
            pathname = b;
            
            stream = fopen(pathname, "r");
            if (!stream) {
                fprintf(stderr, "%s: cannot open '%s'\n",
                        /*argv[0]*/ "spike", pathname);
                goto unwind;
            }
            
            *treeTail = SpkParser_ParseFileStream(stream, st);
            if (!treeTail)
                goto unwind;
            
            fclose(stream);
            stream = 0;
            
            Spk_DECREF(tmp);
            tmp = SpkHost_StringFromCString(pathname);
            SpkParser_Source(treeTail, tmp);
            
            for (s = *treeTail; s->next; s = s->next)
                ;
            treeTail = &s->next;
        }
    }
    
    if (!tree)
        goto unwind;
    
    moduleTmpl = compileTree(&tree, st, notifier);
    
    fclose(moduleStream);
    Spk_DECREF(notifier);
    Spk_DECREF(st);
    Spk_DECREF(tmp);
    
    return moduleTmpl;
    
unwind:
    if (stream)
        fclose(stream);
    if (moduleStream)
        fclose(moduleStream);
    Spk_XDECREF(notifier);
    Spk_XDECREF(st);
    Spk_XDECREF(tmp);
    return 0;
}
