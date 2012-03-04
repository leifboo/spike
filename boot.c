
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

#include "array.h"
#include "dict.h"
#include "io.h"
#include "sym.h"

#include <assert.h>
#include <stdio.h>


#define CLASS_TMPL_DEF(c) Class ## c ## Tmpl
#define CBR(c, s) &CLASS_TMPL_DEF(c)

ClassBootRec essentialClassBootRec[] = {
    /***CBR(VariableObject, Object),*/
    /******/CBR(Context, VariableObject),
    /**********/CBR(MethodContext, Context),
    /**********/CBR(BlockContext,  Context),
    /**/CBR(Message, Object),
    /**/CBR(Thunk,   Object),
    /**/CBR(Interpreter,         Object),
    /**/CBR(ProcessorScheduler,  Object),
    /**/CBR(Fiber,               Object),
    /**/CBR(Null,    Object),
    /**/CBR(Uninit,  Object),
    /**/CBR(Void,    Object),
    /**/CBR(Char,    Object),
    /**** compiler ****/
    /**/CBR(XExpr,         Object),
    /**/CBR(XStmt,         Object),
    /**/CBR(XSymbolNode,   Object),
    /**/CBR(XSTEntry,      Object),
    /**/CBR(XContextClass, Object),
    /**/CBR(XScope,        Object),
    /**/CBR(XSymbolTable,  Object),
    /**/CBR(Parser,        Object),
    /**/CBR(XNotifier,     Object),
    0
};


#define OBJECT(c, v) { offsetof(Heart, c), offsetof(Heart, v) }

ObjectBootRec objectBootRec[] = {
    OBJECT(Null,   null),
    OBJECT(Uninit, uninit),
    OBJECT(Void,   xvoid),
    {0, 0}
};


#define VAR(c, v, n) { offsetof(Heart, c), offsetof(Heart, v), n }

VarBootRec globalVarBootRec[] = {
    VAR(FileStream, xstdin,  "stdin"),
    VAR(FileStream, xstdout, "stdout"),
    VAR(FileStream, xstderr, "stderr"),
    {0, 0}
};


typedef struct HeartSubclass {
    Heart base;
    Unknown *variables[1]; /* stretchy */
} HeartSubclass;

Heart scaffold;


static void initCoreClasses(void) {
    Behavior
        *_Object, *_Behavior,
        *_Class, *_Metaclass;
    Metaclass
        *_ObjectClass, *_BehaviorClass,
        *_ClassClass, *_MetaclassClass;
    Behavior *_Module, *_Heart;
    Behavior
        *_IdentityDictionary, *_Symbol;
    Metaclass
        *_IdentityDictionaryClass, *_SymbolClass;
    MethodNamespace ns;
    size_t i;
    
    /* Cf. figure 16.4 on p. 271 of the Blue Book.  (Spike has no
       class 'ClassDescription'.) */
    
    /*
     * Create the (meta)class objects an establish their "instance of"
     * relationships.
     */
    
    /* Create the core classes. */
    _Object    = (Behavior *)ObjMem_Alloc(ClassClassTmpl.thisClass.instVarOffset);
    _Behavior  = (Behavior *)ObjMem_Alloc(ClassClassTmpl.thisClass.instVarOffset);
    _Class     = (Behavior *)ObjMem_Alloc(ClassClassTmpl.thisClass.instVarOffset);
    _Metaclass = (Behavior *)ObjMem_Alloc(ClassClassTmpl.thisClass.instVarOffset);
    
    /* Create the core metaclasses. */
    _ObjectClass    = (Metaclass *)ObjMem_Alloc(ClassMetaclassTmpl.thisClass.instVarOffset);
    _BehaviorClass  = (Metaclass *)ObjMem_Alloc(ClassMetaclassTmpl.thisClass.instVarOffset);
    _ClassClass     = (Metaclass *)ObjMem_Alloc(ClassMetaclassTmpl.thisClass.instVarOffset);
    _MetaclassClass = (Metaclass *)ObjMem_Alloc(ClassMetaclassTmpl.thisClass.instVarOffset);
    
    /* Each class is an instance of the corresponding metaclass. */
    _Object->base.klass    = (Behavior *)_ObjectClass;     INCREF(_ObjectClass);
    _Behavior->base.klass  = (Behavior *)_BehaviorClass;   INCREF(_BehaviorClass);
    _Class->base.klass     = (Behavior *)_ClassClass;      INCREF(_ClassClass);
    _Metaclass->base.klass = (Behavior *)_MetaclassClass;  INCREF(_MetaclassClass);
    
    /* Each metaclass is an instance of 'Metaclass'. */
    _ObjectClass->base.base.klass    = _Metaclass;  INCREF(_Metaclass);
    _BehaviorClass->base.base.klass  = _Metaclass;  INCREF(_Metaclass);
    _ClassClass->base.base.klass     = _Metaclass;  INCREF(_Metaclass);
    _MetaclassClass->base.base.klass = _Metaclass;  INCREF(_Metaclass);
    
    /* Create IdentityDictionary and Symbol. */
    _IdentityDictionary = (Behavior *)ObjMem_Alloc(ClassClassTmpl.thisClass.instVarOffset);
    _Symbol             = (Behavior *)ObjMem_Alloc(ClassClassTmpl.thisClass.instVarOffset);
    
    _IdentityDictionaryClass = (Metaclass *)ObjMem_Alloc(ClassMetaclassTmpl.thisClass.instVarOffset);
    _SymbolClass             = (Metaclass *)ObjMem_Alloc(ClassMetaclassTmpl.thisClass.instVarOffset);
    
    _IdentityDictionary->base.klass = (Behavior *)_IdentityDictionaryClass;  INCREF(_IdentityDictionaryClass);
    _Symbol->base.klass             = (Behavior *)_SymbolClass;              INCREF(_SymbolClass);
    
    _IdentityDictionaryClass->base.base.klass = _Metaclass;  INCREF(_Metaclass);
    _SymbolClass->base.base.klass             = _Metaclass;  INCREF(_Metaclass);
    
    /*
     * Establish the core class hierarchy:
     *
     *     Object
     *         Behavior
     *             Class
     *             Metaclass
     *
     * Initialize the 'Behavior' struct for each class object.
     */
    
    /* 'Object' is the only class in the system with no superclass.
       It must be handled as a special case. */
    _Object->superclass = 0;
    _Object->module = 0;
    for (i = 0; i < NUM_OPER; ++i) {
        _Object->operTable[i] = 0;
    }
    for (i = 0; i < NUM_CALL_OPER; ++i) {
        _Object->operCallTable[i] = 0;
    }
    _Object->assignInd = 0;
    _Object->assignIndex = 0;
    _Object->zero = ClassObjectTmpl.thisClass.zero;
    _Object->dealloc = ClassObjectTmpl.thisClass.dealloc;
    _Object->instVarOffset = ClassObjectTmpl.thisClass.instVarOffset;
    _Object->instVarBaseIndex = 0;
    _Object->instVarCount = 0;
    _Object->instanceSize = _Object->instVarOffset;
    _Object->itemSize = ClassObjectTmpl.thisClass.itemSize;

    /* Before continuing, partially initialize IdentityDictionary and Symbol. */
    _IdentityDictionary->superclass = _Object;
    _IdentityDictionary->zero = _Object->zero;
    _IdentityDictionary->instVarOffset = ClassIdentityDictionaryTmpl.thisClass.instVarOffset;
    _IdentityDictionary->instVarBaseIndex = _Object->instVarBaseIndex + _Object->instVarCount;
    _IdentityDictionary->instVarCount = 0;
    _IdentityDictionary->instanceSize = ClassIdentityDictionaryTmpl.thisClass.instVarOffset;
    _IdentityDictionary->itemSize = _Object->itemSize;
    _Symbol->zero = _Object->zero;
    if (ClassIdentityDictionaryTmpl.thisClass.zero) {
        _IdentityDictionary->zero = ClassIdentityDictionaryTmpl.thisClass.zero;
    }
    if (ClassSymbolTmpl.thisClass.zero) {
        _Symbol->zero = ClassSymbolTmpl.thisClass.zero;
    }

    /*
     * Certain 'heart' variables are accessed very early;
     * before we proceed, set up some scaffolding so that we don't crash.
     */
    heart = (Module *)&scaffold;
    CLASS(Behavior) = _Behavior;
    CLASS(Class) = _Class;
    CLASS(IdentityDictionary) = _IdentityDictionary;
    CLASS(Symbol) = _Symbol;
    GLOBAL(uninit) = (Unknown *)_Object;

    /* Finish initializing 'Object'. */
    for (ns = 0; ns < NUM_METHOD_NAMESPACES; ++ns) {
        _Object->methodDict[ns] = Host_NewSymbolDict();
    }
    ((Class *)_Object)->name = Host_SymbolFromCString(ClassObjectTmpl.name);
    
    /* Initialize the remaining core classes. */
    /**/Class_InitFromTemplate((Class *)_Behavior, &ClassBehaviorTmpl, _Object, 0);
    /******/Class_InitFromTemplate((Class *)_Class, &ClassClassTmpl, _Behavior, 0);
    /******/Class_InitFromTemplate((Class *)_Metaclass, &ClassMetaclassTmpl, _Behavior, 0);
    /**/Class_InitFromTemplate((Class *)_IdentityDictionary, &ClassIdentityDictionaryTmpl, _Object, 0);
    /**/Class_InitFromTemplate((Class *)_Symbol, &ClassSymbolTmpl, _Object, 0);
    
    /* The metaclass of 'Object' is a subclass of 'Class'.  The rest
       of the metaclass hierarchy mirrors the class hierarchy. */
    Behavior_Init((Behavior *)_ObjectClass, _Class, 0, 0);
    /**/Behavior_Init((Behavior *)_BehaviorClass, (Behavior *)_ObjectClass, 0, 0);
    /******/Behavior_Init((Behavior *)_ClassClass, (Behavior *)_BehaviorClass, 0, 0);
    /******/Behavior_Init((Behavior *)_MetaclassClass, (Behavior *)_BehaviorClass, 0, 0);
    /**/Behavior_Init((Behavior *)_IdentityDictionaryClass, (Behavior *)_ObjectClass, 0, 0);
    /**/Behavior_Init((Behavior *)_SymbolClass, (Behavior *)_ObjectClass, 0, 0);
    
    /* Each metaclass has a reference to is sole instance. */
    _ObjectClass->thisClass = (Class *)_Object;        INCWREF(_Object);
    _BehaviorClass->thisClass = (Class *)_Behavior;    INCWREF(_Behavior);
    _ClassClass->thisClass = (Class *)_Class;          INCWREF(_Class);
    _MetaclassClass->thisClass = (Class *)_Metaclass;  INCWREF(_Metaclass);
    _IdentityDictionaryClass->thisClass = (Class *)_IdentityDictionary; INCWREF(_IdentityDictionary);
    _SymbolClass->thisClass = (Class *)_Symbol;                         INCWREF(_Symbol);
    
    /*
     * Create class 'Module' and its first subclass and instance:
     * the built-in "heart" module.
     */
    _Module = (Behavior *)Class_EmptyFromTemplate(&ClassModuleTmpl, _Object, _Metaclass, 0);
    CLASS(Module) = _Module;
    
    ModulemoduleTmpl.moduleClass.thisClass.instVarOffset = offsetof(HeartSubclass, variables);
    _Heart = (Behavior *)Class_EmptyFromTemplate(&ModulemoduleTmpl.moduleClass, _Module, _Metaclass, 0);
    heart = (Module *)Object_New(_Heart);
    
    /*
     * Initialize the class variables for existing classes.
     */
    CLASS(Object) = _Object;
    CLASS(Behavior) = _Behavior;
    CLASS(Class) = _Class;
    CLASS(Metaclass) = _Metaclass;
    CLASS(Module) = _Module;
    CLASS(IdentityDictionary) = _IdentityDictionary;
    CLASS(Symbol) = _Symbol;
    
    /*
     * Patch the 'module' field of existing classes.
     */
    CLASS(Object)->module    = heart;  INCREF(heart);
    CLASS(Behavior)->module  = heart;  INCREF(heart);
    CLASS(Class)->module     = heart;  INCREF(heart);
    CLASS(Metaclass)->module = heart;  INCREF(heart);
    
    _ObjectClass->base.module    = heart;  INCREF(heart);
    _BehaviorClass->base.module  = heart;  INCREF(heart);
    _ClassClass->base.module     = heart;  INCREF(heart);
    _MetaclassClass->base.module = heart;  INCREF(heart);
    
    CLASS(IdentityDictionary)->module = heart;  INCREF(heart);
    CLASS(Symbol)->module             = heart;  INCREF(heart);
    
    _IdentityDictionaryClass->base.module = heart;  INCREF(heart);
    _SymbolClass->base.module             = heart;  INCREF(heart);
    
    CLASS(Module)->module    = heart;  INCREF(heart);
    _Heart->module                = heart;  INCREF(heart);
    
    /*
     * Create the remaining classes needed to fully describe a class.
     */
    
    /**/CLASS(VariableObject) = (Behavior *)Class_EmptyFromTemplate(&ClassVariableObjectTmpl, CLASS(Object), _Metaclass, heart);
    /******/CLASS(Method) = (Behavior *)Class_EmptyFromTemplate(&ClassMethodTmpl, CLASS(VariableObject), _Metaclass, heart);
    
    /* for building selectors */
    /******/CLASS(Array) = (Behavior *)Class_EmptyFromTemplate(&ClassArrayTmpl, CLASS(VariableObject), _Metaclass, heart);
    
    /* Stand-in until initGlobalObjects() is called. */
    GLOBAL(null)   = (Unknown *)CLASS(Object);
    GLOBAL(uninit) = (Unknown *)CLASS(Object);
    GLOBAL(xvoid)   = (Unknown *)CLASS(Object);
    
    /*
     * The class tmpl machinery is now operational.  Populate the
     * existing classes with methods.
     */
    
    Behavior_AddMethodsFromTemplate(CLASS(Object),    &ClassObjectTmpl.thisClass);
    Behavior_AddMethodsFromTemplate(CLASS(Behavior),  &ClassBehaviorTmpl.thisClass);
    Behavior_AddMethodsFromTemplate(CLASS(Class),     &ClassClassTmpl.thisClass);
    Behavior_AddMethodsFromTemplate(CLASS(Metaclass), &ClassMetaclassTmpl.thisClass);
    
    Behavior_AddMethodsFromTemplate((Behavior *)_ObjectClass,    &ClassObjectTmpl.metaclass);
    Behavior_AddMethodsFromTemplate((Behavior *)_BehaviorClass,  &ClassBehaviorTmpl.metaclass);
    Behavior_AddMethodsFromTemplate((Behavior *)_ClassClass,     &ClassClassTmpl.metaclass);
    Behavior_AddMethodsFromTemplate((Behavior *)_MetaclassClass, &ClassMetaclassTmpl.metaclass);
    
    Behavior_AddMethodsFromTemplate(CLASS(Module),         &ClassModuleTmpl.thisClass);
    Behavior_AddMethodsFromTemplate(CLASS(VariableObject), &ClassVariableObjectTmpl.thisClass);
    Behavior_AddMethodsFromTemplate(CLASS(Method),         &ClassMethodTmpl.thisClass);
    
    Behavior_AddMethodsFromTemplate(CLASS(Module)->base.klass,         &ClassModuleTmpl.metaclass);
    Behavior_AddMethodsFromTemplate(CLASS(VariableObject)->base.klass, &ClassVariableObjectTmpl.metaclass);
    Behavior_AddMethodsFromTemplate(CLASS(Method)->base.klass,         &ClassMethodTmpl.metaclass);
    
    Behavior_AddMethodsFromTemplate(CLASS(IdentityDictionary), &ClassIdentityDictionaryTmpl.thisClass);
    Behavior_AddMethodsFromTemplate(CLASS(Symbol),             &ClassSymbolTmpl.thisClass);
    Behavior_AddMethodsFromTemplate(CLASS(Array),              &ClassArrayTmpl.thisClass);
    
    Behavior_AddMethodsFromTemplate((Behavior *)_IdentityDictionaryClass, &ClassIdentityDictionaryTmpl.metaclass);
    Behavior_AddMethodsFromTemplate((Behavior *)_SymbolClass,             &ClassSymbolTmpl.metaclass);
    Behavior_AddMethodsFromTemplate(CLASS(Array)->base.klass,           &ClassArrayTmpl.metaclass);
    
    /*
     * The End
     */
    
    DECREF(_ObjectClass);
    DECREF(_BehaviorClass);
    DECREF(_ClassClass);
    DECREF(_MetaclassClass);
    DECREF(_IdentityDictionaryClass);
    DECREF(_SymbolClass);
}


static void initBuiltInClasses(void) {
    ClassBootRec *r;
    Behavior **classVar, **superclassVar;
    ClassTmpl *t;
    
    for (r = essentialClassBootRec; *r; ++r) {
        t = *r;
        classVar = (Behavior **)((char *)heart + t->classVarOffset);
        superclassVar = (Behavior **)((char *)heart + t->superclassVarOffset);
        *classVar = (Behavior *)Class_FromTemplate(t, *superclassVar, heart);
    }
    for (r = classBootRec; *r; ++r) {
        t = *r;
        classVar = (Behavior **)((char *)heart + t->classVarOffset);
        superclassVar = (Behavior **)((char *)heart + t->superclassVarOffset);
        *classVar = (Behavior *)Class_FromTemplate(t, *superclassVar, heart);
    }
}


static void initGlobalObjects(void) {
    ObjectBootRec *r;
    Behavior **classVar;
    Object **var, *obj;
    
    for (r = objectBootRec; r->varOffset; ++r) {
        classVar = (Behavior **)((char *)heart + r->classVarOffset);
        var = (Object **)((char *)heart + r->varOffset);
        obj = Object_New(*classVar);
        assert(!CAST(VariableObject, obj));
        *var = obj;
    }
}


static void initGlobalVars(void) {
    VarBootRec *r;
    Behavior **classVar;
    Object **var, *obj;
    
    for (r = globalVarBootRec; r->varOffset; ++r) {
        classVar = (Behavior **)((char *)heart + r->classVarOffset);
        var = (Object **)((char *)heart + r->varOffset);
        obj = Object_New(*classVar);
        assert(!CAST(VariableObject, obj));
        *var = obj;
    }
}


int Boot(void) {
    initCoreClasses();
    if (InitSymbols() < 0)
        return 0;
    Host_Init();
    
    CLASS(False) = CLASS(True) = 0; /* XXX: Heart_zero */
    
    initBuiltInClasses();
    if (InitReadOnlyData() < 0)
        return 0;
    Module_InitLiteralsFromTemplate(heart->base.klass, &ModulemoduleTmpl);
    
    /* XXX: There is an order-of-init problem that prevents core
       classes from defining operators.  As a work-around, simply
       re-initialize the affected classes.  */
    Behavior_AddMethodsFromTemplate(CLASS(Object), &ClassObjectTmpl.thisClass);
    Behavior_AddMethodsFromTemplate(CLASS(Array), &ClassArrayTmpl.thisClass);
    Behavior_AddMethodsFromTemplate(CLASS(IdentityDictionary), &ClassIdentityDictionaryTmpl.thisClass);
    
    initGlobalObjects();
    initGlobalVars();
    
    Interpreter_Boot();
    
    GLOBAL(theInterpreter) = Interpreter_New();
    
    if (CLASS(False) && CLASS(True)) {
        /* create true and false */
        GLOBAL(xfalse) = (Unknown *)Object_New(CLASS(False));
        GLOBAL(xtrue) = (Unknown *)Object_New(CLASS(True));
    }
    
    /* init I/O */
    if (!IO_Boot())
        return 0;
    
    return 1;
}


static void releaseBuiltInClasses(void) {
    ClassBootRec *r;
    Behavior **classVar;
    ClassTmpl *t;
    
    for (r = classBootRec; *r; ++r) {
        t = *r;
        classVar = (Behavior **)((char *)heart + t->classVarOffset);
        CLEAR(*classVar);
    }
    for (r = essentialClassBootRec; *r; ++r) {
        t = *r;
        classVar = (Behavior **)((char *)heart + t->classVarOffset);
        CLEAR(*classVar);
    }
}


static void releaseGlobalObjects(void) {
    ObjectBootRec *r;
    Object **var;
    
    for (r = objectBootRec; r->varOffset; ++r) {
        var = (Object **)((char *)heart + r->varOffset);
        CLEAR(*var);
    }
}


static void releaseGlobalVars(void) {
    VarBootRec *r;
    Object **var;
    
    for (r = globalVarBootRec; r->varOffset; ++r) {
        var = (Object **)((char *)heart + r->varOffset);
        CLEAR(*var);
    }
}


static void releaseCoreClasses(void) {
    CLEAR(CLASS(Array));
    CLEAR(CLASS(VariableObject));
    
    CLEAR(CLASS(Symbol));
    CLEAR(CLASS(IdentityDictionary));
    CLEAR(CLASS(Module));
    CLEAR(CLASS(Metaclass));
    CLEAR(CLASS(Class));
    CLEAR(CLASS(Behavior));
    CLEAR(CLASS(Object));
    
    CLEAR(CLASS(Method));
}


void Shutdown(void) {
    CLEAR(GLOBAL(xstdin));
    CLEAR(GLOBAL(xstdout));
    CLEAR(GLOBAL(xstderr));
    
    CLEAR(GLOBAL(xtrue));
    CLEAR(GLOBAL(xfalse));
    
    CLEAR(GLOBAL(theInterpreter));
    
    releaseGlobalVars();
    releaseGlobalObjects();
    
    ReleaseReadOnlyData();
    
    releaseBuiltInClasses();
    
    ReleaseSymbols();
    
    releaseCoreClasses();
}
