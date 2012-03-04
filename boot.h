
#ifndef __boot_h__
#define __boot_h__


#include <stddef.h>


struct Behavior;
struct ClassTmpl;
struct Object;


typedef struct ClassTmpl *ClassBootRec;

typedef struct ObjectBootRec {
    size_t classVarOffset;
    size_t varOffset;
} ObjectBootRec;

typedef struct VarBootRec {
    size_t classVarOffset;
    size_t varOffset;
    const char *name;
} VarBootRec;


int Boot(void);


extern ClassBootRec essentialClassBootRec[];
extern ClassBootRec classBootRec[];
extern ObjectBootRec objectBootRec[];
extern VarBootRec globalVarBootRec[];


#endif /* __boot_h__ */
