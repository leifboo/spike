
#include "bool.h"

#include "class.h"
#include "compiler.h"
#include "heart.h"
#include "interp.h"
#include "module.h"
#include "rodata.h"


SpkUnknown *Spk_false, *Spk_true;


static const char *source =
"\n"
"class Boolean {\n"
"    !self    { self subclassResponsibility; }\n"
"    ~self    { self subclassResponsibility; }\n"
"    self & x { self subclassResponsibility; }\n"
"    self ^ x { self subclassResponsibility; }\n"
"    self | x { self subclassResponsibility; }\n"
"}\n"
"\n"
"class False : Boolean {\n"
"    !self    { return true; }\n"
"    ~self    { return true; }\n"
"    self & x { return false; }\n"
"    self ^ x { return x; }\n"
"    self | x { return x; }\n"
"}\n"
"\n"
"class True : Boolean {\n"
"    !self    { return false; }\n"
"    ~self    { return false; }\n"
"    self & x { return x; }\n"
"    self ^ x { return !x; }\n"
"    self | x { return true; }\n"
"}\n"
"\n"
"(Boolean) { return (Boolean); }\n"
"(False)   { return (False); }\n"
"(True)    { return (True); }\n"
"\n";


int SpkBoolean_Boot(void) {
    SpkModule *module;
    SpkUnknown *tmp;
    
    module = SpkCompiler_CompileString(source);
    if (!module)
        return 0;
    
    /* Get the classes. */
    tmp = Spk_Attr(theInterpreter, (SpkUnknown *)module, Spk_Boolean);
    if (!tmp)
        return 0;
    Spk_CLASS(Boolean) = Spk_CAST(Behavior, tmp);
    
    tmp = Spk_Attr(theInterpreter, (SpkUnknown *)module, Spk_False);
    if (!tmp)
        return 0;
    Spk_CLASS(False) = Spk_CAST(Behavior, tmp);
    
    tmp = Spk_Attr(theInterpreter, (SpkUnknown *)module, Spk_True);
    if (!tmp)
        return 0;
    Spk_CLASS(True) = Spk_CAST(Behavior, tmp);
    
    if (!Spk_CLASS(Boolean) ||
        !Spk_CLASS(False) ||
        !Spk_CLASS(True))
        return 0;
    
    /* Create 'true' and 'false'. */
    Spk_false = Spk_CallAttr(theInterpreter,
                             (SpkUnknown *)Spk_CLASS(False), Spk_new, 0);
    Spk_true = Spk_CallAttr(theInterpreter,
                            (SpkUnknown *)Spk_CLASS(True), Spk_new, 0);
    
    return 1;
}
