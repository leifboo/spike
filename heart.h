
#ifndef __heart_h__
#define __heart_h__


#include "module.h"


typedef struct Heart {
    Module base;


    /*
     * class variables
     */

    struct Behavior *Array;
    struct Behavior *Behavior;
    struct Behavior *BlockContext;
    struct Behavior *Boolean;
    struct Behavior *Char;
    struct Behavior *Class;
    struct Behavior *Context;
    struct Behavior *False;
    struct Behavior *Fiber;
    struct Behavior *FileStream;
    struct Behavior *Float;
    struct Behavior *IdentityDictionary;
    struct Behavior *Integer;
    struct Behavior *Interpreter;
    struct Behavior *Message;
    struct Behavior *Metaclass;
    struct Behavior *Method;
    struct Behavior *MethodContext;
    struct Behavior *Module;
    struct Behavior *Null;
    struct Behavior *Object;
    struct Behavior *ProcessorScheduler;
    struct Behavior *String;
    struct Behavior *Symbol;
    struct Behavior *Thunk;
    struct Behavior *True;
    struct Behavior *Uninit;
    struct Behavior *VariableObject;
    struct Behavior *Void;
    
    struct Behavior *Parser;
    
    struct Behavior *XExpr;
    struct Behavior *XStmt;
    struct Behavior *XSymbolNode;
    struct Behavior *XSTEntry;
    struct Behavior *XContextClass;
    struct Behavior *XScope;
    struct Behavior *XSymbolTable;
    struct Behavior *XNotifier;


    /*
     * global variables
     */
    
    struct Interpreter *theInterpreter;
    struct Unknown *xfalse, *xtrue;
    struct Unknown *null, *uninit, *xvoid;
    
    /* sometimes std* are macros */
    struct FileStream *xstdin, *xstdout, *xstderr;


} Heart;


extern ModuleTmpl ModulemoduleTmpl;

extern Module *heart;


#define GLOBAL(g) (((Heart *)heart)->g)
#define LITERAL(l) (Module_LITERALS(heart)[l])

#define CLASS(c) GLOBAL(c)

#define HEART_CLASS_TMPL(n, s) CLASS_TMPL(n, s, Heart)


#endif /* __heart_h__ */
