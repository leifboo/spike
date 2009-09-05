
#include "bool.h"

#include "class.h"
#include "compiler.h"
#include "interp.h"
#include "module.h"
#include "rodata.h"


SpkUnknown *Spk_false, *Spk_true;
SpkBehavior *Spk_ClassBoolean, *Spk_ClassFalse, *Spk_ClassTrue;


static const char *source =
""
"class Boolean {"
"    !self    { self subclassResponsibility; }"
"    ~self    { self subclassResponsibility; }"
"    self & x { self subclassResponsibility; }"
"    self ^ x { self subclassResponsibility; }"
"    self | x { self subclassResponsibility; }"
"}"
""
"class False : Boolean {"
"    !self    { return true; }"
"    ~self    { return true; }"
"    self & x { return false; }"
"    self ^ x { return x; }"
"    self | x { return x; }"
"}"
""
"class True : Boolean {"
"    !self    { return false; }"
"    ~self    { return false; }"
"    self & x { return x; }"
"    self ^ x { return !x; }"
"    self | x { return true; }"
"}"
""
"(Boolean) { return (Boolean); }"
"(False)   { return (False); }"
"(True)    { return (True); }"
"";


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
    Spk_ClassBoolean = Spk_CAST(Behavior, tmp);
    
    tmp = Spk_Attr(theInterpreter, (SpkUnknown *)module, Spk_False);
    if (!tmp)
        return 0;
    Spk_ClassFalse = Spk_CAST(Behavior, tmp);
    
    tmp = Spk_Attr(theInterpreter, (SpkUnknown *)module, Spk_True);
    if (!tmp)
        return 0;
    Spk_ClassTrue = Spk_CAST(Behavior, tmp);
    
    if (!Spk_ClassBoolean ||
        !Spk_ClassFalse ||
        !Spk_ClassTrue)
        return 0;
    
    /* Create 'true' and 'false'. */
    Spk_false = Spk_CallAttr(theInterpreter,
                             (SpkUnknown *)Spk_ClassFalse, Spk_new, 0);
    Spk_true = Spk_CallAttr(theInterpreter,
                            (SpkUnknown *)Spk_ClassTrue, Spk_new, 0);
    
    return 1;
}
