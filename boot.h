
#ifndef __spk_boot_h__
#define __spk_boot_h__


struct SpkBehavior;
struct SpkClassTmpl;
struct SpkObject;


typedef struct SpkClassBootRec {
    struct SpkBehavior **var;
    struct SpkClassTmpl *tmpl;
    struct SpkBehavior **superclass;
} SpkClassBootRec;

typedef struct SpkObjectBootRec {
    struct SpkBehavior **klass;
    struct SpkObject **var;
} SpkObjectBootRec;

typedef struct SpkVarBootRec {
    struct SpkBehavior **klass;
    struct SpkObject **var;
    const char *name;
} SpkVarBootRec;


int Spk_Boot(void);


extern SpkClassBootRec Spk_classBootRec[];
extern SpkObjectBootRec Spk_objectBootRec[];
extern SpkVarBootRec Spk_globalVarBootRec[];


#endif /* __spk_boot_h__ */
