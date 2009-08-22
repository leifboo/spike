
#ifndef __spk_notifier_h__
#define __spk_notifier_h__


#include "obj.h"

#include <stdio.h>


typedef struct SpkNotifier {
    SpkObject base;
    FILE *stream;
    unsigned int errorTally;
    SpkUnknown *source;
} SpkNotifier;

typedef struct SpkNotifierSubclass {
    SpkNotifier base;
    SpkUnknown *variables[1]; /* stretchy */
} SpkNotifierSubclass;


extern struct SpkBehavior *Spk_ClassNotifier;
extern struct SpkClassTmpl Spk_ClassNotifierTmpl;


#endif /* __spk_notifier_h__ */
