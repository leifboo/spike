
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


static Unknown *defaultNotifier(void) {
    return Send(GLOBAL(theInterpreter), (Unknown *)CLASS(XNotifier), new, GLOBAL(xstderr), 0);
}


static ModuleTmpl *compileTree(Stmt **pTree, /* destroys reference */
                                  SymbolTable *st,
                                  Unknown *notifier)
{
    Stmt *tree;
    Unknown *tmp = 0;
    ModuleTmpl *moduleTmpl = 0;
    
    tree = Parser_NewModuleDef(*pTree);
    if (!tree) {
        tree = *pTree;
        *pTree = 0;
        goto unwind;
    }
    *pTree = 0;
    
    tmp = StaticChecker_Check(tree, st, notifier);
    if (!tmp)
        goto unwind;
    
    tmp = Send(GLOBAL(theInterpreter), notifier, failOnError, 0);
    if (!tmp)
        goto unwind;
    
    moduleTmpl = CodeGen_GenerateCode(tree);
    
    DECREF(tmp);
    DECREF(tree);
    
    return moduleTmpl;
    
 unwind:
    XDECREF(tmp);
    DECREF(tree);
    return 0;
}


struct ModuleTmpl *Compiler_CompileFileStream(FILE *stream) {
    Unknown *notifier = 0;
    SymbolTable *st = 0;
    Stmt *tree = 0;
    ModuleTmpl *moduleTmpl = 0;
    Unknown *tmp = 0;
    
    notifier = defaultNotifier();
    if (!notifier)
        goto unwind;
    
    st = SymbolTable_New();
    if (!st)
        goto unwind;
    tmp = StaticChecker_DeclareBuiltIn(st, notifier);
    if (!tmp)
        goto unwind;
    DECREF(tmp);
    
    tree = Parser_ParseFileStream(stream, st);
    if (!tree)
        goto unwind;
    
    moduleTmpl = compileTree(&tree, st, notifier);
    
    DECREF(notifier);
    DECREF(st);
    
    return moduleTmpl;
    
 unwind:
    XDECREF(notifier);
    XDECREF(st);
    return 0;
}


ModuleTmpl *Compiler_CompileFile(const char *pathname) {
    FILE *stream = 0;
    Unknown *notifier = 0;
    SymbolTable *st = 0;
    Stmt *tree = 0;
    Unknown *tmp = 0;
    ModuleTmpl *moduleTmpl = 0;
    
    stream = fopen(pathname, "r");
    if (!stream) {
        fprintf(stderr, "%s: cannot open '%s'\n",
                /*argv[0]*/ "spike", pathname);
        goto unwind;
    }
    
    notifier = defaultNotifier();
    if (!notifier)
        goto unwind;
    
    st = SymbolTable_New();
    if (!st)
        goto unwind;
    tmp = StaticChecker_DeclareBuiltIn(st, notifier);
    if (!tmp)
        goto unwind;
    DECREF(tmp);
    
    tree = Parser_ParseFileStream(stream, st);
    if (!tree)
        goto unwind;
    
    tmp = Host_StringFromCString(pathname);
    Parser_Source(&tree, tmp);
    
    moduleTmpl = compileTree(&tree, st, notifier);
    
    fclose(stream);
    DECREF(notifier);
    DECREF(st);
    DECREF(tmp);
    
    return moduleTmpl;
    
 unwind:
    if (stream)
        fclose(stream);
    XDECREF(notifier);
    XDECREF(st);
    XDECREF(tmp);
    return 0;
}


ModuleTmpl *Compiler_CompileString(const char *string) {
    Unknown *notifier = 0;
    SymbolTable *st = 0;
    Stmt *tree = 0;
    ModuleTmpl *moduleTmpl = 0;
    Unknown *tmp = 0;
    
    notifier = defaultNotifier();
    if (!notifier)
        goto unwind;
    
    st = SymbolTable_New();
    if (!st)
        goto unwind;
    tmp = StaticChecker_DeclareBuiltIn(st, notifier);
    if (!tmp)
        goto unwind;
    DECREF(tmp);
    
    tree = Parser_ParseString(string, st);
    if (!tree)
        goto unwind;
    
    moduleTmpl = compileTree(&tree, st, notifier);
    
    DECREF(notifier);
    DECREF(st);
    
    return moduleTmpl;
    
 unwind:
    XDECREF(notifier);
    XDECREF(st);
    return 0;
}


ModuleTmpl *Compiler_CompileModule(const char *pathname) {
    FILE *moduleStream = 0, *stream = 0;
    Unknown *notifier = 0;
    SymbolTable *st = 0;
    Stmt *tree = 0, **treeTail, *s;
    Unknown *tmp = 0;
    ModuleTmpl *moduleTmpl = 0;
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
    
    st = SymbolTable_New();
    if (!st)
        goto unwind;
    tmp = StaticChecker_DeclareBuiltIn(st, notifier);
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
            
            *treeTail = Parser_ParseFileStream(stream, st);
            if (!treeTail)
                goto unwind;
            
            fclose(stream);
            stream = 0;
            
            DECREF(tmp);
            tmp = Host_StringFromCString(pathname);
            Parser_Source(treeTail, tmp);
            
            for (s = *treeTail; s->next; s = s->next)
                ;
            treeTail = &s->next;
        }
    }
    
    if (!tree)
        goto unwind;
    
    moduleTmpl = compileTree(&tree, st, notifier);
    
    fclose(moduleStream);
    DECREF(notifier);
    DECREF(st);
    DECREF(tmp);
    
    return moduleTmpl;
    
unwind:
    if (stream)
        fclose(stream);
    if (moduleStream)
        fclose(moduleStream);
    XDECREF(notifier);
    XDECREF(st);
    XDECREF(tmp);
    return 0;
}
