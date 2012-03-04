
#ifndef __io_h__
#define __io_h__


#include "obj.h"
#include <stdio.h>


typedef struct FileStream FileStream;


extern struct ClassTmpl ClassFileStreamTmpl;


int IO_Boot(void);

FILE *FileStream_AsCFileStream(FileStream *);


#endif /* __io_h__ */
