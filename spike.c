
#include "cgen.h"
#include "parser.h"
#include "tree.h"

#include "array.h"
#include "behavior.h"
#include "bool.h"
#include "char.h"
#include "class.h"
#include "dict.h"
#include "float.h"
#include "int.h"
#include "interp.h"
#include "metaclass.h"
#include "module.h"
#include "native.h"
#include "obj.h"
#include "str.h"
#include "sym.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static Behavior *Spk_ClassNULL_CLASS;


#define CLASS_VAR(c) Spk_Class ## c
#define CLASS_TMPL(c) Spk_Class ## c ## Tmpl

#define BOOT_REC(c, v, s, i) {(Behavior **)&CLASS_VAR(c), &CLASS_TMPL(c), (Object **)&v, s, i}

#define METACLASS(c) BOOT_REC(Metaclass, CLASS_VAR(c), (Behavior **)&CLASS_VAR(Behavior), &CLASS_TMPL(c))
#define CLASS(c, s) BOOT_REC(Class, CLASS_VAR(c), (Behavior **)&CLASS_VAR(s), &CLASS_TMPL(c))
#define OBJECT(c, v) BOOT_REC(c, v, 0, 0)


static BootRec bootRec[] = {
    CLASS(Object, NULL_CLASS),
    /**/CLASS(Behavior, Object),
    /******/METACLASS(Metaclass),
    /******/METACLASS(Class),
    /**/CLASS(VariableObject, Object),
    /******/CLASS(Method,  VariableObject),
    /**********/CLASS(NativeAccessor,  Method),
    /******/CLASS(Context, VariableObject),
    /******/CLASS(Module,  VariableObject),
    /******/CLASS(String,  VariableObject),
    /******/CLASS(Array,   VariableObject),
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
    /**/CLASS(Integer, Object),
    /**/CLASS(Float,   Object),
    /**/CLASS(Char,    Object),
    OBJECT(False,  Spk_false),
    OBJECT(True,   Spk_true),
    OBJECT(Module, Spk_builtInModule),
    OBJECT(Null,   Spk_null),
    OBJECT(Uninit, Spk_uninit),
    OBJECT(Void,   Spk_void),
    {0, 0}
};


static void bootstrap() {
    BootRec *r;
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
        Spk_operSelectors[operator].messageSelector = SpkSymbol_get(Spk_operSelectors[operator].messageSelectorStr);
    }
    for (operator = 0; operator < NUM_CALL_OPER; ++operator) {
        Spk_operCallSelectors[operator].messageSelector = SpkSymbol_get(Spk_operCallSelectors[operator].messageSelectorStr);
    }
    for (r = bootRec; r->var; ++r) {
        if (r->init) {
            /* it is too early to use Spk_CAST */
            if ((*r->var)->klass == (Behavior *)Spk_ClassClass) {
                SpkClass_initFromTemplate((Class *)*r->var, r->init, *r->superclass, Spk_builtInModule);
            } else {
                assert((*r->var)->klass == (Behavior *)Spk_ClassMetaclass);
                SpkBehavior_initFromTemplate((Behavior *)*r->var, r->init, *r->superclass, Spk_builtInModule);
            }
        }
    }
    
    /* init methods */
    for (r = bootRec; r->var; ++r) {
        if (r->init) {
            Class *c;
            if ((c = Spk_CAST(Class, *r->var))) {
                SpkClass_addMethodsFromTemplate(c, r->init);
            } else {
                assert((*r->var)->klass == (Behavior *)Spk_ClassMetaclass);
                SpkBehavior_addMethodsFromTemplate((Behavior *)*r->var, r->init);
            }
        }
    }
}


static Stmt *wrap(Stmt *stmtList) {
    /* Wrap the top-level statement list in a module (class)
       definition.  XXX: Where does this code really belong? */
    Stmt *compoundStmt, *moduleDef;
    
    compoundStmt = (Stmt *)malloc(sizeof(Stmt));
    memset(compoundStmt, 0, sizeof(Stmt));
    compoundStmt->kind = STMT_COMPOUND;
    compoundStmt->top = stmtList;

    moduleDef = (Stmt *)malloc(sizeof(Stmt));
    memset(moduleDef, 0, sizeof(Stmt));
    moduleDef->kind = STMT_DEF_CLASS;
    moduleDef->top = compoundStmt;
    
    return moduleDef;
}


int main(int argc, char **argv) {
    int i, showHelp, error, disassemble;
    char *arg, *sourceFilename;
    Stmt *tree;
    Module *module;
    Object *result; Integer *resultInt;
    unsigned int dataSize;
    StmtList predefList, rootClassList;
    
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
    
    tree = SpkParser_ParseFile(sourceFilename);
    if (!tree) {
        return 1;
    }
    
    if (SpkStaticChecker_Check(tree, bootRec, &dataSize, &predefList, &rootClassList) == -1) {
        return 1;
    }
    tree = wrap(tree);
    module = SpkCodeGen_generateCode(tree, dataSize, predefList.first, rootClassList.first);
    
    if (disassemble) {
        SpkDisassembler_disassembleModule(module, stdout);
        return 0;
    }
    
    result = SpkInterpreter_start((Object *)module, SpkSymbol_get("main"), SpkArray_new(0));
    
    if (!result) {
        return 1;
    }
    resultInt = Spk_CAST(Integer, result);
    if (resultInt) {
        return SpkInteger_asLong(resultInt);
    }
    return 0;
}
