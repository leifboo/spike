
#include "cgen.h"
#include "parser.h"
#include "tree.h"

#include "behavior.h"
#include "bool.h"
#include "class.h"
#include "dict.h"
#include "int.h"
#include "interp.h"
#include "metaclass.h"
#include "module.h"
#include "obj.h"
#include "str.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>


static Behavior *ClassNULL_CLASS;


#define CLASS_VAR(c) Class ## c
#define CLASS_TMPL(c) Class ## c ## Tmpl

#define BOOT_REC(c, v, s, i) {(Behavior **)&CLASS_VAR(c), &CLASS_TMPL(c), (Object **)&v, s, i}

#define METACLASS(c) BOOT_REC(Metaclass, CLASS_VAR(c), (Behavior **)&CLASS_VAR(Behavior), &CLASS_TMPL(c))
#define CLASS(c, s) BOOT_REC(Class, CLASS_VAR(c), (Behavior **)&CLASS_VAR(s), &CLASS_TMPL(c))
#define OBJECT(c, v) BOOT_REC(c, v, 0, 0)


static struct BootRec {
    Behavior **klass;
    SpkClassTmpl *klassTmpl;
    Object **var;
    Behavior **superclass;
    SpkClassTmpl *init;
} bootRec[] = {
    CLASS(Object, NULL_CLASS),
    /**/CLASS(Behavior, Class),
    /******/METACLASS(Metaclass),
    /******/METACLASS(Class),
    /**/CLASS(Module, Object),
    /**/CLASS(Boolean, Object),
    /******/CLASS(False, Boolean),
    /******/CLASS(True, Boolean),
    /**/CLASS(IdentityDictionary, Object),
    /**/CLASS(Symbol,  Object),
    /**/CLASS(Message, Object),
    /**/CLASS(Thunk,   Object),
    /**/CLASS(Null,    Object),
    /**/CLASS(Uninit,  Object),
    /**/CLASS(Void,    Object),
    OBJECT(False,  Spk_false),
    OBJECT(True,   Spk_true),
    OBJECT(Module, builtInModule),
    OBJECT(Null,   Spk_null),
    OBJECT(Uninit, Spk_uninit),
    OBJECT(Void,   Spk_void),
    {0, 0}
};


static void bootstrap() {
    struct BootRec *r;
    Oper operator;
    
    /* alloc */
    for (r = bootRec; r->var; ++r) {
        *r->var = (Object *)malloc(r->klassTmpl->instanceSize);
        (*r->var)->refCount = 0;
    }
    /* init 'klass' */
    for (r = bootRec; r->var; ++r) {
        (*r->var)->klass = *r->klass;
    }
    
    /* init classes */
    for (operator = 0; operator < NUM_OPER; ++operator) {
        specialSelectors[operator].messageSelector = SpkSymbol_get(specialSelectors[operator].messageSelectorStr);
    }
    for (r = bootRec; r->var; ++r) {
        if (r->init) {
            if ((*r->var)->klass == (Behavior *)ClassClass) {
                SpkClass_initFromTemplate((Class *)*r->var, r->init, *r->superclass, builtInModule);
            } else {
                assert((*r->var)->klass == (Behavior *)ClassMetaclass);
                SpkBehavior_initFromTemplate((Behavior *)*r->var, r->init, *r->superclass, builtInModule);
            }
        }
    }
}


int main(int argc, char **argv) {
    int i, showHelp, error, disassemble;
    char *arg, *sourceFilename ;
    Stmt *tree;
    Module *module;
    Object *entry, *result;
    unsigned int dataSize;
    
    sourceFilename = 0;
    showHelp = error = disassemble = 0;
    for (i = 1; i < argc; ++i) {
        arg = argv[i];
        if (*arg != '-') {
            sourceFilename = arg;
            break;
        }
        ++arg;
        if (*arg == '-') {
            ++arg;
            if (strcmp(arg, "help") == 0) {
                showHelp = 1;
                continue;
            }
            if (strcmp(arg, "disassemble") == 0) {
                disassemble = 1;
                continue;
            }
            fprintf(stderr, "%s: unrecognized option %s\n", argv[0], argv[i]);
            error = 1;
        } else {
            switch (*arg) {
            case '?':
            case 'h':
                showHelp = 1;
                break;
            default:
                fprintf(stderr, "%s: unrecognized option %s\n", argv[0], argv[i]);
                error = 1;
                break;
            }
        }
    }
    
    if (error) {
        return 1;
    }
    if (showHelp) {
        printf("-?, -h, --help    Display this help and exit.\n");
        printf("--disassemble     Display code disassembly and exit.\n");
        return 0;
    }
    
    if (!sourceFilename) {
        printf("Yip!\n");
        return 1;
    }
    
    /* XXX: program arguments */
    
    bootstrap();
    
    SpkClassInteger_init();
    SpkClassChar_init();
    SpkClassString_init();
    
    tree = SpkParser_ParseFile(sourceFilename);
    if (!tree) {
        return 1;
    }
    
    if (SpkStaticChecker_Check(tree, &dataSize) == -1) {
        return 1;
    }
    module = SpkCodeGen_generateCode(tree, dataSize);
    
    if (disassemble) {
        SpkDisassembler_disassembleModule(module, stdout);
        return 0;
    }
    
    entry = SpkModule_lookupSymbol(module, SpkSymbol_get("main"));
    if (!entry) {
        fprintf(stderr, "unresolved symbol 'main'\n");
        return 1;
    }
    
    result = SpkInterpreter_start(entry);
    
    if (!result) {
        return 1;
    }
    if (result->klass == ClassInteger) {
        return SpkInteger_asLong((Integer *)result);
    }
    return 0;
}
