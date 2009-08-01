
#include "boot.h"

#include "behavior.h"
#include "char.h"
#include "class.h"
#include "float.h"
#include "host.h"
#include "interp.h"
#include "metaclass.h"
#include "module.h"
#include "native.h"
#include "obj.h"
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


#define CLASS_VAR(c) Spk_Class ## c
#define CLASS_TMPL(c) Spk_Class ## c ## Tmpl


#define CLASS(c, s) {&CLASS_VAR(c), &CLASS_TMPL(c), &CLASS_VAR(s)}

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
#ifdef MALTIPY
    /**** bridge ****/
    CLASS(PythonObject, Object /*NULL_CLASS*/),
#else /* !MALTIPY */
    /***CLASS(VariableObject, Object),*/
    /******/CLASS(String,  VariableObject),
    /******/CLASS(Array,   VariableObject),
    /**/CLASS(Boolean,   Object),
    /******/CLASS(False, Boolean),
    /******/CLASS(True,  Boolean),
    /**/CLASS(IdentityDictionary, Object),
    /**/CLASS(Symbol,     Object),
    /**/CLASS(Integer,    Object),
    /**/CLASS(Float,      Object),
    /**/CLASS(FileStream, Object),
#endif /* !MALTIPY */
    {0, 0}
};


#define OBJECT(c, v) {(SpkBehavior **)&CLASS_VAR(c),(SpkObject **)&v}

SpkObjectBootRec Spk_objectBootRec[] = {
#ifndef MALTIPY
    OBJECT(False,  Spk_false),
    OBJECT(True,   Spk_true),
#endif /* !MALTIPY */
    OBJECT(Null,   Spk_null),
    OBJECT(Uninit, Spk_uninit),
    OBJECT(Void,   Spk_void),
    {0, 0}
};


#define VAR(c, v, n) {(SpkBehavior **)&CLASS_VAR(c),(SpkObject **)&v, n}

SpkVarBootRec Spk_globalVarBootRec[] = {
#ifndef MALTIPY
    VAR(FileStream, Spk_stdin,  "stdin"),
    VAR(FileStream, Spk_stdout, "stdout"),
    VAR(FileStream, Spk_stderr, "stderr"),
#endif /* !MALTIPY */
};


static void initCoreClasses(void) {
    SpkMetaclass
        *ClassObjectClass, *ClassBehaviorClass,
        *ClassClassClass, *ClassMetaclassClass;
    SpkMethodNamespace namespace;
    size_t i;
    
    /* Cf. figure 16.4 on p. 271 of the Blue Book.  (Spike has no
       class 'ClassDescription'.) */
    
    /*
     * Create the (meta)class objects an establish their "instance of"
     * relationships.
     */
    
    /* Create the core classes. */
    Spk_ClassObject    = (SpkBehavior *)SpkObjMem_Alloc(Spk_ClassClassTmpl.thisClass.instVarOffset);
    Spk_ClassBehavior  = (SpkBehavior *)SpkObjMem_Alloc(Spk_ClassClassTmpl.thisClass.instVarOffset);
    Spk_ClassClass     = (SpkBehavior *)SpkObjMem_Alloc(Spk_ClassClassTmpl.thisClass.instVarOffset);
    Spk_ClassMetaclass = (SpkBehavior *)SpkObjMem_Alloc(Spk_ClassClassTmpl.thisClass.instVarOffset);
    
    /* Create the core metaclasses. */
    ClassObjectClass    = (SpkMetaclass *)SpkObjMem_Alloc(Spk_ClassMetaclassTmpl.thisClass.instVarOffset);
    ClassBehaviorClass  = (SpkMetaclass *)SpkObjMem_Alloc(Spk_ClassMetaclassTmpl.thisClass.instVarOffset);
    ClassClassClass     = (SpkMetaclass *)SpkObjMem_Alloc(Spk_ClassMetaclassTmpl.thisClass.instVarOffset);
    ClassMetaclassClass = (SpkMetaclass *)SpkObjMem_Alloc(Spk_ClassMetaclassTmpl.thisClass.instVarOffset);
    
    /* Each class is an instance of the corresponding metaclass. */
    Spk_ClassObject->base.klass    = (SpkBehavior *)ClassObjectClass;     Spk_INCREF(ClassObjectClass);
    Spk_ClassBehavior->base.klass  = (SpkBehavior *)ClassBehaviorClass;   Spk_INCREF(ClassBehaviorClass);
    Spk_ClassClass->base.klass     = (SpkBehavior *)ClassClassClass;      Spk_INCREF(ClassClassClass);
    Spk_ClassMetaclass->base.klass = (SpkBehavior *)ClassMetaclassClass;  Spk_INCREF(ClassMetaclassClass);
    
    /* Each metaclass is an instance of 'Metaclass'. */
    ClassObjectClass->base.base.klass    = Spk_ClassMetaclass;  Spk_INCREF(Spk_ClassMetaclass);
    ClassBehaviorClass->base.base.klass  = Spk_ClassMetaclass;  Spk_INCREF(Spk_ClassMetaclass);
    ClassClassClass->base.base.klass     = Spk_ClassMetaclass;  Spk_INCREF(Spk_ClassMetaclass);
    ClassMetaclassClass->base.base.klass = Spk_ClassMetaclass;  Spk_INCREF(Spk_ClassMetaclass);
    
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
    Spk_ClassObject->superclass = 0;
    Spk_ClassObject->module = 0;
    for (namespace = 0; namespace < Spk_NUM_METHOD_NAMESPACES; ++namespace) {
        Spk_ClassObject->ns[namespace].methodDict = SpkHost_NewSymbolDict();
        for (i = 0; i < Spk_NUM_OPER; ++i) {
            Spk_ClassObject->ns[namespace].operTable[i] = 0;
        }
        for (i = 0; i < Spk_NUM_CALL_OPER; ++i) {
            Spk_ClassObject->ns[namespace].operCallTable[i] = 0;
        }
    }
    Spk_ClassObject->zero = Spk_ClassObjectTmpl.thisClass.zero;
    Spk_ClassObject->dealloc = Spk_ClassObjectTmpl.thisClass.dealloc;
    Spk_ClassObject->next = 0;
    Spk_ClassObject->nextInScope = 0;
    Spk_ClassObject->nestedClassList.first = 0;
    Spk_ClassObject->nestedClassList.last = 0;
    Spk_ClassObject->instVarOffset = Spk_ClassObjectTmpl.thisClass.instVarOffset;
    Spk_ClassObject->instVarBaseIndex = 0;
    Spk_ClassObject->instVarCount = 0;
    Spk_ClassObject->instanceSize = Spk_ClassObject->instVarOffset;
    Spk_ClassObject->itemSize = Spk_ClassObjectTmpl.thisClass.itemSize;
    ((SpkClass *)Spk_ClassObject)->name = SpkHost_SymbolFromString(Spk_ClassObjectTmpl.name);
    
    /**/SpkClass_InitFromTemplate((SpkClass *)Spk_ClassBehavior, &Spk_ClassBehaviorTmpl, Spk_ClassObject, 0);
    /******/SpkClass_InitFromTemplate((SpkClass *)Spk_ClassClass, &Spk_ClassClassTmpl, Spk_ClassBehavior, 0);
    /******/SpkClass_InitFromTemplate((SpkClass *)Spk_ClassMetaclass, &Spk_ClassMetaclassTmpl, Spk_ClassBehavior, 0);
    
    /* The metaclass of 'Object' is a subclass of 'Class'.  The rest
       of the metaclass hierarchy mirrors the class hierarchy. */
    SpkBehavior_Init((SpkBehavior *)ClassObjectClass, Spk_ClassClass, 0, 0);
    /**/SpkBehavior_Init((SpkBehavior *)ClassBehaviorClass, (SpkBehavior *)ClassObjectClass, 0, 0);
    /******/SpkBehavior_Init((SpkBehavior *)ClassClassClass, (SpkBehavior *)ClassBehaviorClass, 0, 0);
    /******/SpkBehavior_Init((SpkBehavior *)ClassMetaclassClass, (SpkBehavior *)ClassBehaviorClass, 0, 0);
    
    /* Each metaclass has a reference to is sole instance. */
    ClassObjectClass->thisClass = (SpkClass *)Spk_ClassObject;        Spk_INCREF(Spk_ClassObject);
    ClassBehaviorClass->thisClass = (SpkClass *)Spk_ClassBehavior;    Spk_INCREF(Spk_ClassBehavior);
    ClassClassClass->thisClass = (SpkClass *)Spk_ClassClass;          Spk_INCREF(Spk_ClassClass);
    ClassMetaclassClass->thisClass = (SpkClass *)Spk_ClassMetaclass;  Spk_INCREF(Spk_ClassMetaclass);
    
    /*
     * Create class 'Module' and its first instance, the built-in
     * module.
     */
    
    Spk_ClassModule = (SpkBehavior *)SpkClass_EmptyFromTemplate(&Spk_ClassModuleTmpl, Spk_ClassObject, 0);
    Spk_builtInModule = (SpkModule *)SpkObject_New(Spk_ClassModule);
    Spk_builtInModule->literals = 0;
    Spk_builtInModule->firstClass = 0;
    
    /*
     * Patch the 'module' field of existing classes.
     */
    Spk_ClassObject->module    = Spk_builtInModule;  Spk_INCREF(Spk_builtInModule);
    Spk_ClassBehavior->module  = Spk_builtInModule;  Spk_INCREF(Spk_builtInModule);
    Spk_ClassClass->module     = Spk_builtInModule;  Spk_INCREF(Spk_builtInModule);
    Spk_ClassMetaclass->module = Spk_builtInModule;  Spk_INCREF(Spk_builtInModule);
    
    ClassObjectClass->base.module    = Spk_builtInModule;  Spk_INCREF(Spk_builtInModule);
    ClassBehaviorClass->base.module  = Spk_builtInModule;  Spk_INCREF(Spk_builtInModule);
    ClassClassClass->base.module     = Spk_builtInModule;  Spk_INCREF(Spk_builtInModule);
    ClassMetaclassClass->base.module = Spk_builtInModule;  Spk_INCREF(Spk_builtInModule);
    
    Spk_ClassModule->module    = Spk_builtInModule;  Spk_INCREF(Spk_builtInModule);
    
    /*
     * Create the remaining classes needed to fully describe a class.
     */
    
    /**/Spk_ClassVariableObject = (SpkBehavior *)SpkClass_EmptyFromTemplate(&Spk_ClassVariableObjectTmpl, Spk_ClassObject, Spk_builtInModule);
    /******/Spk_ClassMethod = (SpkBehavior *)SpkClass_EmptyFromTemplate(&Spk_ClassMethodTmpl, Spk_ClassVariableObject, Spk_builtInModule);
    /**********/Spk_ClassNativeAccessor = (SpkBehavior *)SpkClass_EmptyFromTemplate(&Spk_ClassNativeAccessorTmpl, Spk_ClassMethod, Spk_builtInModule);
    
    /*
     * The class template machinery is now operational.  Populate the
     * existing classes with methods.
     */
    
    SpkBehavior_AddMethodsFromTemplate(Spk_ClassObject,    &Spk_ClassObjectTmpl.thisClass);
    SpkBehavior_AddMethodsFromTemplate(Spk_ClassBehavior,  &Spk_ClassBehaviorTmpl.thisClass);
    SpkBehavior_AddMethodsFromTemplate(Spk_ClassClass,     &Spk_ClassClassTmpl.thisClass);
    SpkBehavior_AddMethodsFromTemplate(Spk_ClassMetaclass, &Spk_ClassMetaclassTmpl.thisClass);
    
    SpkBehavior_AddMethodsFromTemplate((SpkBehavior *)ClassObjectClass,    &Spk_ClassObjectTmpl.metaclass);
    SpkBehavior_AddMethodsFromTemplate((SpkBehavior *)ClassBehaviorClass,  &Spk_ClassBehaviorTmpl.metaclass);
    SpkBehavior_AddMethodsFromTemplate((SpkBehavior *)ClassClassClass,     &Spk_ClassClassTmpl.metaclass);
    SpkBehavior_AddMethodsFromTemplate((SpkBehavior *)ClassMetaclassClass, &Spk_ClassMetaclassTmpl.metaclass);
    
    SpkBehavior_AddMethodsFromTemplate(Spk_ClassModule,         &Spk_ClassModuleTmpl.thisClass);
    SpkBehavior_AddMethodsFromTemplate(Spk_ClassVariableObject, &Spk_ClassVariableObjectTmpl.thisClass);
    SpkBehavior_AddMethodsFromTemplate(Spk_ClassMethod,         &Spk_ClassMethodTmpl.thisClass);
    SpkBehavior_AddMethodsFromTemplate(Spk_ClassNativeAccessor, &Spk_ClassNativeAccessorTmpl.thisClass);
    
    SpkBehavior_AddMethodsFromTemplate(Spk_ClassModule->base.klass,         &Spk_ClassModuleTmpl.metaclass);
    SpkBehavior_AddMethodsFromTemplate(Spk_ClassVariableObject->base.klass, &Spk_ClassVariableObjectTmpl.metaclass);
    SpkBehavior_AddMethodsFromTemplate(Spk_ClassMethod->base.klass,         &Spk_ClassMethodTmpl.metaclass);
    SpkBehavior_AddMethodsFromTemplate(Spk_ClassNativeAccessor->base.klass, &Spk_ClassNativeAccessorTmpl.metaclass);
    
    /*
     * The End
     */
    
    Spk_DECREF(ClassObjectClass);
    Spk_DECREF(ClassBehaviorClass);
    Spk_DECREF(ClassClassClass);
    Spk_DECREF(ClassMetaclassClass);
}


static void initBuiltInClasses(void) {
    SpkClassBootRec *r;
    
    for (r = Spk_classBootRec; r->var; ++r) {
        *r->var = (SpkBehavior *)SpkClass_FromTemplate(r->tmpl, *r->superclass, Spk_builtInModule);
    }
}


static void initGlobalObjects(void) {
    SpkObjectBootRec *r;
    SpkObject *obj;
    
    for (r = Spk_objectBootRec; r->var; ++r) {
        obj = SpkObject_New(*r->klass);
        assert(!Spk_CAST(VariableObject, obj));
        *r->var = obj;
    }
}


static void initGlobalVars(void) {
    SpkVarBootRec *r;
    SpkObject *obj;
    
    for (r = Spk_globalVarBootRec; r->var; ++r) {
        obj = SpkObject_New(*r->klass);
        assert(!Spk_CAST(VariableObject, obj));
        *r->var = obj;
    }
    
#ifndef MALTIPY
    /* init I/O */
    Spk_stdin->stream = stdin;
    Spk_stdout->stream = stdout;
    Spk_stderr->stream = stderr;
#endif /* !MALTIPY */
}


void Spk_Bootstrap(void) {
    initCoreClasses();
    initBuiltInClasses();
    initGlobalObjects();
    initGlobalVars();
    
#ifdef MALTIPY
    /* XXX: Do this cleanly! */
    Spk_DECREF(Spk_ClassPythonObject->superclass);
    Spk_ClassPythonObject->superclass = 0;
#endif /* MALTIPY */
}
