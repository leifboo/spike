
#ifndef __io_h__
#define __io_h__


#include "obj.h"
#include <stdio.h>


typedef struct FileStream {
    Object base;
    FILE *stream;
} FileStream;

typedef struct FileStreamSubclass {
    FileStream base;
    Object *variables[1]; /* stretchy */
} FileStreamSubclass;


extern struct Behavior *Spk_ClassFileStream;
extern struct SpkClassTmpl Spk_ClassFileStreamTmpl;
extern FileStream *Spk_stdin, *Spk_stdout, *Spk_stderr;


#endif /* __io_h__ */
