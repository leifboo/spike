
#include "boot.h"

#include "behavior.h"
#include "char.h"
#include "class.h"
#include "float.h"
#include "heart.h"
#include "host.h"
#include "interp.h"
#include "metaclass.h"
#include "module.h"
#include "native.h"
#include "notifier.h"
#include "obj.h"
#include "parser.h"
#include "rodata.h"
#include "st.h"
#include "tree.h"

#ifdef MALTIPY
#include "bridge.h"
#else /* !MALTIPY */
#include "array.h"
#include "bool.h"
#include "dict.h"
#include "float.h"
#include "int.h"
#include "io.h"
#include "str.h"
#include "sym.h"
#endif /* !MALTIPY */

#include <assert.h>
#include <stdio.h>


#define CLASS_TMPL(c) Spk_Class ## c ## Tmpl


#define CLASS(c, s) &CLASS_TMPL(c)

SpkClassBootRec Spk_classBootRec[] = {
    /***CLASS(VariableObject, Object),*/
    /******/CLASS(Context, VariableObject),
    /**********/CLASS(MethodContext, Context),
    /**********/CLASS(BlockContext,  Context),
    /**/CLASS(Message, Object),
    /**/CLASS(Thunk,   Object),
    /**/CLASS(Interpreter,         Object),
    /**/CLASS(ProcessorScheduler,  Object),
    /**/CLASS(Fiber,               Object),
    /**/CLASS(Null,    Object),
    /**/CLASS(Uninit,  Object),
    /**/CLASS(Void,    Object),
    /**/CLASS(Char,    Object),
    /**** compiler ****/
    /**/CLASS(Expr,         Object),
    /**/CLASS(Stmt,         Object),
    /**/CLASS(SymbolNode,   Object),
    /**/CLASS(STEntry,      Object),
    /**/CLASS(ContextClass, Object),
    /**/CLASS(Scope,        Object),
    /**/CLASS(SymbolTable,  Object),
    /**/CLASS(Parser,       Object),
    /**/CLASS(Notifier,     Object),
#ifdef MALTIPY
    /**** bridge ****/
    CLASS(PythonObject, Object /*NULL_CLASS*/),
#else /* !MALTIPY */
    /***CLASS(VariableObject, Object),*/
    /******/CLASS(String,  VariableObject),
    /**/CLASS(Integer,    Object),
    /**/CLASS(Float,      Object),
    /**/CLASS(FileStream, Object),
#endif /* !MALTIPY */
    0
};


#define OBJECT(c, v) { offsetof(SpkHeart, c), offsetof(SpkHeart, v) }

SpkObjectBootRec Spk_objectBootRec[] = {
    OBJECT(Null,   null),
    OBJECT(Uninit, uninit),
    OBJECT(Void,   xvoid),
    {0, 0}
};


#define VAR(c, v, n) { offsetof(SpkHeart, c), offsetof(SpkHeart, v), n }

SpkVarBootRec Spk_globalVarBootRec[] = {
#ifndef MALTIPY
    VAR(FileStream, xstdin,  "stdin"),
    VAR(FileStream, xstdout, "stdout"),
    VAR(FileStream, xstderr, "stderr"),
#endif /* !MALTIPY */
    {0, 0}
};


typedef struct SpkHeartSubclass {
    SpkHeart base;
    SpkUnknown *variables[1]; /* stretchy */
} SpkHeartSubclass;

SpkBehaviorTmpl Spk_ModuleHeartTmpl = {
    /*accessors*/ 0,
    /*methods*/ 0,
    /*lvalueMethods*/ 0,
    offsetof(SpkHeartSubclass, variables)
};


static void initCoreClasses(void) {
    SpkBehavior
        *Object, *Behavior,
        *Class, *Metaclass;
    SpkMetaclass
        *ObjectClass, *BehaviorClass,
        *ClassClass, *MetaclassClass;
    SpkBehavior *Module, *Heart;
#ifndef MALTIPY
    SpkBehavior
        *IdentityDictionary, *Symbol;
    SpkMetaclass
        *IdentityDictionaryClass, *SymbolClass;
#endif /* !MALTIPY */
    SpkMethodNamespace ns;
    size_t i;
    
    /* Cf. figure 16.4 on p. 271 of the Blue Book.  (Spike has no
       class 'ClassDescription'.) */
    
    /*
     * Create the (meta)class objects an establish their "instance of"
     * relationships.
     */
    
    /* Create the core classes. */
    Object    = (SpkBehavior *)SpkObjMem_Alloc(Spk_ClassClassTmpl.thisClass.instVarOffset);
    Behavior  = (SpkBehavior *)SpkObjMem_Alloc(Spk_ClassClassTmpl.thisClass.instVarOffset);
    Class     = (SpkBehavior *)SpkObjMem_Alloc(Spk_ClassClassTmpl.thisClass.instVarOffset);
    Metaclass = (SpkBehavior *)SpkObjMem_Alloc(Spk_ClassClassTmpl.thisClass.instVarOffset);
    
    /* Create the core metaclasses. */
    ObjectClass    = (SpkMetaclass *)SpkObjMem_Alloc(Spk_ClassMetaclassTmpl.thisClass.instVarOffset);
    BehaviorClass  = (SpkMetaclass *)SpkObjMem_Alloc(Spk_ClassMetaclassTmpl.thisClass.instVarOffset);
    ClassClass     = (SpkMetaclass *)SpkObjMem_Alloc(Spk_ClassMetaclassTmpl.thisClass.instVarOffset);
    MetaclassClass = (SpkMetaclass *)SpkObjMem_Alloc(Spk_ClassMetaclassTmpl.thisClass.instVarOffset);
    
    /* Each class is an instance of the corresponding metaclass. */
    Object->base.klass    = (SpkBehavior *)ObjectClass;     Spk_INCREF(ObjectClass);
    Behavior->base.klass  = (SpkBehavior *)BehaviorClass;   Spk_INCREF(BehaviorClass);
    Class->base.klass     = (SpkBehavior *)ClassClass;      Spk_INCREF(ClassClass);
    Metaclass->base.klass = (SpkBehavior *)MetaclassClass;  Spk_INCREF(MetaclassClass);
    
    /* Each metaclass is an instance of 'Metaclass'. */
    ObjectClass->base.base.klass    = Metaclass;  Spk_INCREF(Metaclass);
    BehaviorClass->base.base.klass  = Metaclass;  Spk_INCREF(Metaclass);
    ClassClass->base.base.klass     = Metaclass;  Spk_INCREF(Metaclass);
    MetaclassClass->base.base.klass = Metaclass;  Spk_INCREF(Metaclass);
    
#ifndef MALTIPY
    /* Create IdentityDictionary and Symbol. */
    IdentityDictionary = (SpkBehavior *)SpkObjMem_Alloc(Spk_ClassClassTmpl.thisClass.instVarOffset);
    Symbol             = (SpkBehavior *)SpkObjMem_Alloc(Spk_ClassClassTmpl.thisClass.instVarOffset);
    
    IdentityDictionaryClass = (SpkMetaclass *)SpkObjMem_Alloc(Spk_ClassMetaclassTmpl.thisClass.instVarOffset);
    SymbolClass             = (SpkMetaclass *)SpkObjMem_Alloc(Spk_ClassMetaclassTmpl.thisClass.instVarOffset);
    
    IdentityDictionary->base.klass = (SpkBehavior *)IdentityDictionaryClass;  Spk_INCREF(IdentityDictionaryClass);
    Symbol->base.klass             = (SpkBehavior *)SymbolClass;              Spk_INCREF(SymbolClass);
    
    IdentityDictionaryClass->base.base.klass = Metaclass;  Spk_INCREF(Metaclass);
    SymbolClass->base.base.klass             = Metaclass;  Spk_INCREF(Metaclass);
#endif /* !MALTIPY */
    
    /*
     * Establish the core class hierarchy:
     *
     *     Object
     *         Behavior
     *             Class
     *             Metaclass
     *
     * Initialize the 'SpkBehavior' struct for each class object.
     */
    
    /* 'Object' is the only class in the system with no superclass.
       It must be handled as a special case. */
    Object->superclass = 0;
    Object->module = 0;
    for (i = 0; i < Spk_NUM_OPER; ++i) {
        Object->operTable[i] = 0;
    }
    for (i = 0; i < Spk_NUM_CALL_OPER; ++i) {
        Object->operCallTable[i] = 0;
    }
    Object->assignInd = 0;
    Object->assignIndex = 0;
    Object->zero = Spk_ClassObjectTmpl.thisClass.zero;
    Object->dealloc = Spk_ClassObjectTmpl.thisClass.dealloc;
    Object->traverse = *Spk_ClassObjectTmpl.thisClass.traverse;
    Object->next = 0;
    Object->nextInScope = 0;
    Object->nestedClassList.first = 0;
    Object->nestedClassList.last = 0;
    Object->instVarOffset = Spk_ClassObjectTmpl.thisClass.instVarOffset;
    Object->instVarBaseIndex = 0;
    Object->instVarCount = 0;
    Object->instanceSize = Object->instVarOffset;
    Object->itemSize = Spk_ClassObjectTmpl.thisClass.itemSize;
#ifndef MALTIPY
    /* Before continuing, partially initialize IdentityDictionary and Symbol. */
    IdentityDictionary->superclass = Object;
    IdentityDictionary->zero = Object->zero;
    IdentityDictionary->instVarOffset = Spk_ClassIdentityDictionaryTmpl.thisClass.instVarOffset;
    IdentityDictionary->instVarBaseIndex = Object->instVarBaseIndex + Object->instVarCount;
    IdentityDictionary->instVarCount = 0;
    IdentityDictionary->instanceSize = Spk_ClassIdentityDictionaryTmpl.thisClass.instVarOffset;
    IdentityDictionary->itemSize = Object->itemSize;
    Symbol->zero = Object->zero;
    if (Spk_ClassIdentityDictionaryTmpl.thisClass.zero) {
        IdentityDictionary->zero = Spk_ClassIdentityDictionaryTmpl.thisClass.zero;
    }
    if (Spk_ClassSymbolTmpl.thisClass.zero) {
        Symbol->zero = Spk_ClassSymbolTmpl.thisClass.zero;
    }
#endif /* !MALTIPY */
    /* XXX: These two cannot be module-ized. */
    Spk_ClassSymbol = Symbol;
    Spk_ClassIdentityDictionary = IdentityDictionary;
    for (ns = 0; ns < Spk_NUM_METHOD_NAMESPACES; ++ns) {
        Object->methodDict[ns] = SpkHost_NewSymbolDict();
    }
    ((SpkClass *)Object)->name = SpkHost_SymbolFromCString(Spk_ClassObjectTmpl.name);
    
    /* Initialize the remaining core classes. */
    /**/SpkClass_InitFromTemplate((SpkClass *)Behavior, &Spk_ClassBehaviorTmpl, Object, 0);
    /******/SpkClass_InitFromTemplate((SpkClass *)Class, &Spk_ClassClassTmpl, Behavior, 0);
    /******/SpkClass_InitFromTemplate((SpkClass *)Metaclass, &Spk_ClassMetaclassTmpl, Behavior, 0);
#ifndef MALTIPY
    /**/SpkClass_InitFromTemplate((SpkClass *)IdentityDictionary, &Spk_ClassIdentityDictionaryTmpl, Object, 0);
    /**/SpkClass_InitFromTemplate((SpkClass *)Symbol, &Spk_ClassSymbolTmpl, Object, 0);
#endif /* !MALTIPY */
    
    /* The metaclass of 'Object' is a subclass of 'Class'.  The rest
       of the metaclass hierarchy mirrors the class hierarchy. */
    SpkBehavior_Init((SpkBehavior *)ObjectClass, Class, 0, 0);
    /**/SpkBehavior_Init((SpkBehavior *)BehaviorClass, (SpkBehavior *)ObjectClass, 0, 0);
    /******/SpkBehavior_Init((SpkBehavior *)ClassClass, (SpkBehavior *)BehaviorClass, 0, 0);
    /******/SpkBehavior_Init((SpkBehavior *)MetaclassClass, (SpkBehavior *)BehaviorClass, 0, 0);
#ifndef MALTIPY
    /**/SpkBehavior_Init((SpkBehavior *)IdentityDictionaryClass, (SpkBehavior *)ObjectClass, 0, 0);
    /**/SpkBehavior_Init((SpkBehavior *)SymbolClass, (SpkBehavior *)ObjectClass, 0, 0);
#endif /* !MALTIPY */
    
    /* Each metaclass has a reference to is sole instance. */
    ObjectClass->thisClass = (SpkClass *)Object;        Spk_INCREF(Object);
    BehaviorClass->thisClass = (SpkClass *)Behavior;    Spk_INCREF(Behavior);
    ClassClass->thisClass = (SpkClass *)Class;          Spk_INCREF(Class);
    MetaclassClass->thisClass = (SpkClass *)Metaclass;  Spk_INCREF(Metaclass);
#ifndef MALTIPY
    IdentityDictionaryClass->thisClass = (SpkClass *)IdentityDictionary; Spk_INCREF(IdentityDictionary);
    SymbolClass->thisClass = (SpkClass *)Symbol;                         Spk_INCREF(Symbol);
#endif /* !MALTIPY */
    
    /*
     * Create class 'Module' and its first subclass and instance:
     * the built-in "heart" module.
     */
    Module = (SpkBehavior *)SpkClass_EmptyFromTemplate(&Spk_ClassModuleTmpl, Object, Metaclass, 0);
    
    Heart = (SpkBehavior *)SpkObject_New(Behavior);
    SpkBehavior_InitFromTemplate(Heart, &Spk_ModuleHeartTmpl, Module, 0);
    Spk_heart = (SpkModule *)SpkModule_New(Heart);
    
    /*
     * Initialize the class variables for existing classes.
     */
    Spk_CLASS(Object) = Object;
    Spk_CLASS(Behavior) = Behavior;
    Spk_CLASS(Class) = Class;
    Spk_CLASS(Metaclass) = Metaclass;
    Spk_CLASS(Module) = Module;
#ifndef MALTIPY
    Spk_CLASS(IdentityDictionary) = IdentityDictionary;
    Spk_CLASS(Symbol) = Symbol;
#endif /* !MALTIPY */
    
    /*
     * Patch the 'module' field of existing classes.
     */
    Spk_CLASS(Object)->module    = Spk_heart;  Spk_INCREF(Spk_heart);
    Spk_CLASS(Behavior)->module  = Spk_heart;  Spk_INCREF(Spk_heart);
    Spk_CLASS(Class)->module     = Spk_heart;  Spk_INCREF(Spk_heart);
    Spk_CLASS(Metaclass)->module = Spk_heart;  Spk_INCREF(Spk_heart);
    
    ObjectClass->base.module    = Spk_heart;  Spk_INCREF(Spk_heart);
    BehaviorClass->base.module  = Spk_heart;  Spk_INCREF(Spk_heart);
    ClassClass->base.module     = Spk_heart;  Spk_INCREF(Spk_heart);
    MetaclassClass->base.module = Spk_heart;  Spk_INCREF(Spk_heart);
    
#ifndef MALTIPY
    Spk_CLASS(IdentityDictionary)->module = Spk_heart;  Spk_INCREF(Spk_heart);
    Spk_CLASS(Symbol)->module             = Spk_heart;  Spk_INCREF(Spk_heart);
    
    IdentityDictionaryClass->base.module = Spk_heart;  Spk_INCREF(Spk_heart);
    SymbolClass->base.module             = Spk_heart;  Spk_INCREF(Spk_heart);
#endif /* !MALTIPY */
    
    Spk_CLASS(Module)->module    = Spk_heart;  Spk_INCREF(Spk_heart);
    Heart->module                = Spk_heart;  Spk_INCREF(Spk_heart);
    
    /*
     * Create the remaining classes needed to fully describe a class.
     */
    
    /**/Spk_CLASS(VariableObject) = (SpkBehavior *)SpkClass_EmptyFromTemplate(&Spk_ClassVariableObjectTmpl, Spk_CLASS(Object), Metaclass, Spk_heart);
    /******/Spk_CLASS(Method) = (SpkBehavior *)SpkClass_EmptyFromTemplate(&Spk_ClassMethodTmpl, Spk_CLASS(VariableObject), Metaclass, Spk_heart);
    
#ifndef MALTIPY
    /* for building selectors */
    /******/Spk_CLASS(Array) = (SpkBehavior *)SpkClass_EmptyFromTemplate(&Spk_ClassArrayTmpl, Spk_CLASS(VariableObject), Metaclass, Spk_heart);
    
    /* Stand-in until initGlobalObjects() is called. */
    Spk_GLOBAL(null)   = (SpkUnknown *)Spk_CLASS(Object);
    Spk_GLOBAL(uninit) = (SpkUnknown *)Spk_CLASS(Object);
    Spk_GLOBAL(xvoid)   = (SpkUnknown *)Spk_CLASS(Object);
#endif /* !MALTIPY */
    
    /*
     * The class tmpl machinery is now operational.  Populate the
     * existing classes with methods.
     */
    
    SpkBehavior_AddMethodsFromTemplate(Spk_CLASS(Object),    &Spk_ClassObjectTmpl.thisClass);
    SpkBehavior_AddMethodsFromTemplate(Spk_CLASS(Behavior),  &Spk_ClassBehaviorTmpl.thisClass);
    SpkBehavior_AddMethodsFromTemplate(Spk_CLASS(Class),     &Spk_ClassClassTmpl.thisClass);
    SpkBehavior_AddMethodsFromTemplate(Spk_CLASS(Metaclass), &Spk_ClassMetaclassTmpl.thisClass);
    
    SpkBehavior_AddMethodsFromTemplate((SpkBehavior *)ObjectClass,    &Spk_ClassObjectTmpl.metaclass);
    SpkBehavior_AddMethodsFromTemplate((SpkBehavior *)BehaviorClass,  &Spk_ClassBehaviorTmpl.metaclass);
    SpkBehavior_AddMethodsFromTemplate((SpkBehavior *)ClassClass,     &Spk_ClassClassTmpl.metaclass);
    SpkBehavior_AddMethodsFromTemplate((SpkBehavior *)MetaclassClass, &Spk_ClassMetaclassTmpl.metaclass);
    
    SpkBehavior_AddMethodsFromTemplate(Spk_CLASS(Module),         &Spk_ClassModuleTmpl.thisClass);
    SpkBehavior_AddMethodsFromTemplate(Spk_CLASS(VariableObject), &Spk_ClassVariableObjectTmpl.thisClass);
    SpkBehavior_AddMethodsFromTemplate(Spk_CLASS(Method),         &Spk_ClassMethodTmpl.thisClass);
    
    SpkBehavior_AddMethodsFromTemplate(Spk_CLASS(Module)->base.klass,         &Spk_ClassModuleTmpl.metaclass);
    SpkBehavior_AddMethodsFromTemplate(Spk_CLASS(VariableObject)->base.klass, &Spk_ClassVariableObjectTmpl.metaclass);
    SpkBehavior_AddMethodsFromTemplate(Spk_CLASS(Method)->base.klass,         &Spk_ClassMethodTmpl.metaclass);
    
#ifndef MALTIPY
    SpkBehavior_AddMethodsFromTemplate(Spk_CLASS(IdentityDictionary), &Spk_ClassIdentityDictionaryTmpl.thisClass);
    SpkBehavior_AddMethodsFromTemplate(Spk_CLASS(Symbol),             &Spk_ClassSymbolTmpl.thisClass);
    SpkBehavior_AddMethodsFromTemplate(Spk_CLASS(Array),              &Spk_ClassArrayTmpl.thisClass);
    
    SpkBehavior_AddMethodsFromTemplate((SpkBehavior *)IdentityDictionaryClass, &Spk_ClassIdentityDictionaryTmpl.metaclass);
    SpkBehavior_AddMethodsFromTemplate((SpkBehavior *)SymbolClass,             &Spk_ClassSymbolTmpl.metaclass);
    SpkBehavior_AddMethodsFromTemplate(Spk_CLASS(Array)->base.klass,           &Spk_ClassArrayTmpl.metaclass);
#endif /* !MALTIPY */
    
    /*
     * The End
     */
    
    Spk_DECREF(ObjectClass);
    Spk_DECREF(BehaviorClass);
    Spk_DECREF(ClassClass);
    Spk_DECREF(MetaclassClass);
#ifndef MALTIPY
    Spk_DECREF(IdentityDictionaryClass);
    Spk_DECREF(SymbolClass);
#endif /* !MALTIPY */
}


static void initBuiltInClasses(void) {
    SpkClassBootRec *r;
    SpkBehavior **classVar, **superclassVar;
    SpkClassTmpl *t;
    
    for (r = Spk_classBootRec; *r; ++r) {
        t = *r;
        classVar = (SpkBehavior **)((char *)Spk_heart + t->classVarOffset);
        superclassVar = (SpkBehavior **)((char *)Spk_heart + t->superclassVarOffset);
        *classVar = (SpkBehavior *)SpkClass_FromTemplate(t, *superclassVar, Spk_heart);
    }
}


static void initGlobalObjects(void) {
    SpkObjectBootRec *r;
    SpkBehavior **classVar;
    SpkObject **var, *obj;
    
    for (r = Spk_objectBootRec; r->varOffset; ++r) {
        classVar = (SpkBehavior **)((char *)Spk_heart + r->classVarOffset);
        var = (SpkObject **)((char *)Spk_heart + r->varOffset);
        obj = SpkObject_New(*classVar);
        assert(!Spk_CAST(VariableObject, obj));
        *var = obj;
    }
}


static void initGlobalVars(void) {
    SpkVarBootRec *r;
    SpkBehavior **classVar;
    SpkObject **var, *obj;
    
    for (r = Spk_globalVarBootRec; r->varOffset; ++r) {
        classVar = (SpkBehavior **)((char *)Spk_heart + r->classVarOffset);
        var = (SpkObject **)((char *)Spk_heart + r->varOffset);
        obj = SpkObject_New(*classVar);
        assert(!Spk_CAST(VariableObject, obj));
        *var = obj;
    }
}


int Spk_Boot(void) {
    SpkHost_Init();
    initCoreClasses();
    if (Spk_InitSymbols() < 0)
        return 0;
    initBuiltInClasses();
    if (Spk_InitReadOnlyData() < 0)
        return 0;
    
    /* XXX: There is an order-of-init problem that prevents core
       classes from defining operators.  As a work-around, simply
       re-initialize the affected classes.  */
    SpkBehavior_AddMethodsFromTemplate(Spk_CLASS(Object), &Spk_ClassObjectTmpl.thisClass);
#ifndef MALTIPY
    SpkBehavior_AddMethodsFromTemplate(Spk_CLASS(Array), &Spk_ClassArrayTmpl.thisClass);
#endif /* !MALTIPY */
    
    initGlobalObjects();
    initGlobalVars();
    
    SpkInterpreter_Boot();
    
    Spk_GLOBAL(theInterpreter) = SpkInterpreter_New();
    
#ifdef MALTIPY
    /* XXX: Do this cleanly! */
    Spk_DECREF(Spk_CLASS(PythonObject)->superclass);
    Spk_CLASS(PythonObject)->superclass = 0;
#else
    /* Create true, false, True, False, and Boolean. */
    if (!SpkBoolean_Boot())
        return 0;
    /* init I/O */
    if (!SpkIO_Boot())
        return 0;
#endif /* MALTIPY */
    
    return 1;
}
