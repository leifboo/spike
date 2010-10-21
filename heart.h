
#ifndef __spk_heart_h__
#define __spk_heart_h__


#include "module.h"


typedef struct SpkHeart {
    SpkModule base;


    /*
     * class variables
     */

    struct SpkBehavior *Array;
    struct SpkBehavior *Behavior;
    struct SpkBehavior *BlockContext;
    struct SpkBehavior *Boolean;
    struct SpkBehavior *Char;
    struct SpkBehavior *Class;
    struct SpkBehavior *Context;
    struct SpkBehavior *False;
    struct SpkBehavior *Fiber;
    struct SpkBehavior *FileStream;
    struct SpkBehavior *Float;
    struct SpkBehavior *IdentityDictionary;
    struct SpkBehavior *Integer;
    struct SpkBehavior *Interpreter;
    struct SpkBehavior *Message;
    struct SpkBehavior *Metaclass;
    struct SpkBehavior *Method;
    struct SpkBehavior *MethodContext;
    struct SpkBehavior *Module;
    struct SpkBehavior *Null;
    struct SpkBehavior *Object;
    struct SpkBehavior *ProcessorScheduler;
    struct SpkBehavior *String;
    struct SpkBehavior *Symbol;
    struct SpkBehavior *Thunk;
    struct SpkBehavior *True;
    struct SpkBehavior *Uninit;
    struct SpkBehavior *VariableObject;
    struct SpkBehavior *Void;
    
    struct SpkBehavior *Parser;
    
    struct SpkBehavior *XExpr;
    struct SpkBehavior *XStmt;
    struct SpkBehavior *XSymbolNode;
    struct SpkBehavior *XSTEntry;
    struct SpkBehavior *XContextClass;
    struct SpkBehavior *XScope;
    struct SpkBehavior *XSymbolTable;
    struct SpkBehavior *XNotifier;


    /*
     * global variables
     */
    
    struct SpkInterpreter *theInterpreter;
    SpkUnknown *xfalse, *xtrue;
    SpkUnknown *null, *uninit, *xvoid;
    
    /* sometimes std* are macros */
    struct SpkFileStream *xstdin, *xstdout, *xstderr;


} SpkHeart;


extern SpkModuleTmpl Spk_ModulemoduleTmpl;

extern SpkModule *Spk_heart;


#define Spk_GLOBAL(g) (((SpkHeart *)Spk_heart)->g)
#define Spk_LITERAL(l) (SpkModule_LITERALS(Spk_heart)[l])

#define Spk_CLASS(c) Spk_GLOBAL(c)

#define Spk_HEART_CLASS_TMPL(n, s) Spk_CLASS_TMPL(n, s, SpkHeart)


#endif /* __spk_heart_h__ */
