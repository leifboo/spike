
#ifndef __spk_io_h__
#define __spk_io_h__


#include "obj.h"
#include <stdio.h>


typedef struct SpkFileStream {
    SpkObject base;
    FILE *stream;
} SpkFileStream;

typedef struct SpkFileStreamSubclass {
    SpkFileStream base;
    SpkUnknown *variables[1]; /* stretchy */
} SpkFileStreamSubclass;


extern struct SpkBehavior *Spk_ClassFileStream;
extern struct SpkClassTmpl Spk_ClassFileStreamTmpl;
extern SpkFileStream *Spk_stdin, *Spk_stdout, *Spk_stderr;


#endif /* __spk_io_h__ */
