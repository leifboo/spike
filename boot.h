
#ifndef __spk_boot_h__
#define __spk_boot_h__


#include <stddef.h>


struct SpkBehavior;
struct SpkClassTmpl;
struct SpkObject;


typedef struct SpkClassTmpl *SpkClassBootRec;

typedef struct SpkObjectBootRec {
    size_t classVarOffset;
    size_t varOffset;
} SpkObjectBootRec;

typedef struct SpkVarBootRec {
    size_t classVarOffset;
    size_t varOffset;
    const char *name;
} SpkVarBootRec;


int Spk_Boot(void);


extern SpkClassBootRec Spk_classBootRec[];
extern SpkObjectBootRec Spk_objectBootRec[];
extern SpkVarBootRec Spk_globalVarBootRec[];


#endif /* __spk_boot_h__ */
