
#include "notifier.h"

#include "class.h"
#include "heart.h"
#include "host.h"
#include "native.h"
#include "rodata.h"
#include "st.h"
#include "tree.h"

#include <stdio.h>


struct SpkNotifier {
    SpkObject base;
    FILE *stream;
    unsigned int errorTally;
    SpkUnknown *source;
};


/*------------------------------------------------------------------------*/
/* methods */

static SpkUnknown *Notifier_init(SpkUnknown *_self, SpkUnknown *stream, SpkUnknown *arg1) {
    SpkNotifier *self;
    
    self = (SpkNotifier *)_self;
    if (!SpkHost_IsFileStream(stream)) {
        Spk_Halt(Spk_HALT_TYPE_ERROR, "a stream is required");
        return 0;
    }
    
    self->stream = SpkHost_FileStreamAsCFileStream(stream);
    self->errorTally = 0;
    
    Spk_INCREF(_self);
    return _self;
}

static SpkUnknown *Notifier_badExpr(SpkUnknown *_self, SpkUnknown *arg0, SpkUnknown *arg1) {
    SpkNotifier *self;
    SpkExpr *expr;
    const char *source, *desc;
    
    self = (SpkNotifier *)_self;
    expr = Spk_CAST(Expr, arg0);
    if (!expr) {
        Spk_Halt(Spk_HALT_TYPE_ERROR, "an expression node is required");
        return 0;
    }
    if (!SpkHost_IsString(arg1)) {
        Spk_Halt(Spk_HALT_TYPE_ERROR, "a string is required");
        return 0;
    }
    desc = SpkHost_StringAsCString(arg1);
    
    source = self->source
             ? SpkHost_StringAsCString(self->source)
             : "<unknown>";
    fprintf(stderr, "%s:%u: %s\n",
            source, expr->lineNo, desc);
    ++self->errorTally;
    
    Spk_INCREF(Spk_void);
    return Spk_void;
}

static SpkUnknown *Notifier_redefinedSymbol(SpkUnknown *_self, SpkUnknown *arg0, SpkUnknown *arg1) {
    SpkNotifier *self;
    SpkExpr *expr;
    SpkSymbolNode *sym;
    const char *source, *name, *format;
    
    self = (SpkNotifier *)_self;
    expr = Spk_CAST(Expr, arg0);
    if (!expr) {
        Spk_Halt(Spk_HALT_TYPE_ERROR, "an expression node is required");
        return 0;
    }
    
    source = self->source
             ? SpkHost_StringAsCString(self->source)
             : "<unknown>";
    sym = expr->sym;
    name = SpkHost_SymbolAsCString(sym->sym);
    format = (sym->entry && sym->entry->scope->context->level == 0)
             ? "%s:%u: cannot redefine built-in name '%s'\n"
             : "%s:%u: symbol '%s' multiply defined\n";
    fprintf(stderr, format,
            source, expr->lineNo,
            name);
    ++self->errorTally;
    
    Spk_INCREF(Spk_void);
    return Spk_void;
}

static SpkUnknown *Notifier_undefinedSymbol(SpkUnknown *_self, SpkUnknown *arg0, SpkUnknown *arg1) {
    SpkNotifier *self;
    SpkExpr *expr;
    const char *source;
    
    self = (SpkNotifier *)_self;
    expr = Spk_CAST(Expr, arg0);
    if (!expr) {
        Spk_Halt(Spk_HALT_TYPE_ERROR, "an expression node is required");
        return 0;
    }
    
    source = self->source
                     ? SpkHost_StringAsCString(self->source)
                     : "<unknown>";
    fprintf(self->stream, "%s:%u: symbol '%s' undefined\n",
            source, expr->lineNo,
            SpkHost_SymbolAsCString(expr->sym->sym));
    ++self->errorTally;
    
    Spk_INCREF(Spk_void);
    return Spk_void;
}

static SpkUnknown *Notifier_failOnError(SpkUnknown *_self, SpkUnknown *arg0, SpkUnknown *arg1) {
    SpkNotifier *self;
    
    self = (SpkNotifier *)_self;
    if (self->errorTally > 0) {
        Spk_Halt(Spk_HALT_ERROR, "errors");
        return 0;
    }
    Spk_INCREF(Spk_void);
    return Spk_void;
}


/*------------------------------------------------------------------------*/
/* meta-methods */

static SpkUnknown *ClassNotifier_new(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    SpkUnknown *newNotifier, *tmp;
    
    newNotifier = Spk_CallAttr(theInterpreter, Spk_SUPER, Spk_new, 0);
    if (!newNotifier)
        return 0;
    tmp = newNotifier;
    newNotifier = Spk_CallAttr(theInterpreter, newNotifier, Spk_init, arg0, 0);
    Spk_DECREF(tmp);
    return newNotifier;
}


/*------------------------------------------------------------------------*/
/* low-level hooks */

static void Notifier_zero(SpkObject *_self) {
    SpkNotifier *self = (SpkNotifier *)_self;
    (*Spk_CLASS(Notifier)->superclass->zero)(_self);
    self->stream = 0;
    self->source = 0;
}


/*------------------------------------------------------------------------*/
/* memory layout of instances */

static void Notifier_traverse_init(SpkObject *self) {
    (*Spk_CLASS(Notifier)->superclass->traverse.init)(self);
}

static SpkUnknown **Notifier_traverse_current(SpkObject *_self) {
    SpkNotifier *self;
    
    self = (SpkNotifier *)_self;
    if (self->source)
        return (SpkUnknown **)&self->source;
    return (*Spk_CLASS(Notifier)->superclass->traverse.current)(_self);
}

static void Notifier_traverse_next(SpkObject *_self) {
    SpkNotifier *self;
    
    self = (SpkNotifier *)_self;
    if (self->source) {
        self->source = 0;
        return;
    }
    (*Spk_CLASS(Notifier)->superclass->traverse.next)(_self);
}


/*------------------------------------------------------------------------*/
/* class template */

typedef struct SpkNotifierSubclass {
    SpkNotifier base;
    SpkUnknown *variables[1]; /* stretchy */
} SpkNotifierSubclass;

static SpkAccessorTmpl accessors[] = {
    { "source", Spk_T_OBJECT, offsetof(SpkNotifier, source),
      SpkAccessor_READ | SpkAccessor_WRITE },
    { 0, 0, 0, 0 }
};

static SpkMethodTmpl methods[] = {
    { "init", SpkNativeCode_METH_ATTR | SpkNativeCode_ARGS_1, &Notifier_init },
    { "badExpr:", SpkNativeCode_ARGS_2, &Notifier_badExpr },
    { "redefinedSymbol:", SpkNativeCode_ARGS_1, &Notifier_redefinedSymbol },
    { "undefinedSymbol:", SpkNativeCode_ARGS_1, &Notifier_undefinedSymbol },
    { "failOnError", SpkNativeCode_ARGS_0, &Notifier_failOnError },
    { 0, 0, 0 }
};

static SpkMethodTmpl metaMethods[] = {
    { "new", SpkNativeCode_METH_ATTR | SpkNativeCode_ARGS_1, &ClassNotifier_new },
    { 0, 0, 0}
};

static SpkTraverse traverse = {
    &Notifier_traverse_init,
    &Notifier_traverse_current,
    &Notifier_traverse_next,
};

SpkClassTmpl Spk_ClassNotifierTmpl = {
    Spk_HEART_CLASS_TMPL(Notifier, Object), {
        accessors,
        methods,
        /*lvalueMethods*/ 0,
        offsetof(SpkNotifierSubclass, variables),
        /*itemSize*/ 0,
        &Notifier_zero,
        /*dealloc*/ 0,
        &traverse
    }, /*meta*/ {
        /*accessors*/ 0,
        metaMethods,
        /*lvalueMethods*/ 0
    }
};
