
#include "io.h"

#include "array.h"
#include "bool.h"
#include "char.h"
#include "class.h"
#include "float.h"
#include "heart.h"
#include "int.h"
#include "native.h"
#include "rodata.h"
#include "str.h"

#include <stdlib.h>
#include <string.h>


struct FileStream {
    Object base;
    FILE *stream;
};


/*------------------------------------------------------------------------*/
/* attributes */

static Unknown *FileStream_eof(Unknown *_self, Unknown *arg0, Unknown *arg1) {
    FileStream *self;
    Unknown *result;
    
    self = (FileStream *)_self;
    result = self->stream
             ? (feof(self->stream) ? GLOBAL(xtrue) : GLOBAL(xfalse))
             : GLOBAL(xtrue);
    INCREF(result);
    return result;
}


/*------------------------------------------------------------------------*/
/* methods */

static Unknown *FileStream_close(Unknown *_self, Unknown *arg0, Unknown *arg1) {
    FileStream *self;
    
    self = (FileStream *)_self;
    if (self->stream) {
        fclose(self->stream);
        self->stream = 0;
    }
    INCREF(GLOBAL(xvoid));
    return GLOBAL(xvoid);
}

static Unknown *FileStream_flush(Unknown *_self, Unknown *arg0, Unknown *arg1) {
    FileStream *self;
    
    self = (FileStream *)_self;
    if (self->stream)
        fflush(self->stream);
    INCREF(GLOBAL(xvoid));
    return GLOBAL(xvoid);
}

static Unknown *FileStream_getc(Unknown *_self, Unknown *arg0, Unknown *arg1) {
    FileStream *self;
    int c;
    
    self = (FileStream *)_self;
    if (!self->stream) {
        INCREF(GLOBAL(null));
        return GLOBAL(null);
    }
    c = fgetc(self->stream);
    if (c == EOF) {
        INCREF(GLOBAL(null));
        return GLOBAL(null);
    }
    return (Unknown *)Char_FromCChar((char)c);
}

static Unknown *FileStream_gets(Unknown *_self, Unknown *arg0, Unknown *arg1) {
    FileStream *self;
    Integer *size;
    Unknown *result;
    
    self = (FileStream *)_self;
    size = CAST(Integer, arg0);
    if (!size) {
        Halt(HALT_TYPE_ERROR, "an integer is required");
        return 0;
    }
    if (!self->stream) {
        INCREF(GLOBAL(null));
        return GLOBAL(null);
    }
    result = (Unknown *)String_FromCStream(self->stream, (size_t)Integer_AsCPtrdiff(size));
    if (!result) {
        INCREF(GLOBAL(null));
        return GLOBAL(null);
    }
    return result;
}

static Unknown *FileStream_printf(Unknown *_self, Unknown *arg0, Unknown *arg1) {
    FileStream *self;
    Array *args; size_t nArgs, argIndex;
    Unknown *formatObj = 0; String *formatString; char *format = 0; size_t formatSize;
    char c, convOp, *f, *chunk;
    Unknown *arg = 0; Char *charArg; Integer *intArg; Float *floatArg; String *strArg;
    static const char *convOps = "cdeEfgGinopsuxX%";
    
    self = (FileStream *)_self;
    args = CAST(Array, arg0);
    if (!args) {
        Halt(HALT_TYPE_ERROR, "an array is required");
        goto unwind;
    }
    nArgs = Array_Size(args);
    if (nArgs == 0) {
        Halt(HALT_TYPE_ERROR, "wrong number of arguments");
        goto unwind;
    }
    formatObj = Array_GetItem(args, 0);
    formatString = CAST(String, formatObj);
    if (!formatString) {
        Halt(HALT_TYPE_ERROR, "a string is required");
        goto unwind;
    }
    formatSize = String_Size(formatString);
    format = (char *)malloc(formatSize);
    memcpy(format, String_AsCString(formatString), formatSize);
    
    argIndex = 1;
    f = chunk = format;
    c = *f;
    
    while (c) {
        while (c && c != '%')
            c = *++f;
        if (chunk < f) {
            *f = 0;
            fputs(chunk, self->stream);
            *f = c;
        }
        if (!c) break;
        
        /* found a conversion specification */
        chunk = f;
        do
            c = *++f;
        while (c && !strchr(convOps, c));
        if (!c) {
            Halt(HALT_VALUE_ERROR, "invalid conversion specification");
            goto unwind;
        }
        
        convOp = c;
        c = *++f;
        if (convOp == '%') {
            if (f - chunk != 2) {
                Halt(HALT_VALUE_ERROR, "invalid conversion specification");
                goto unwind;
            }
            fputc('%', self->stream);
            chunk = f;
            continue;
        }
        *f = 0;
        
        /* consume an argument */
        if (argIndex >= nArgs) {
            Halt(HALT_TYPE_ERROR, "too few arguments");
            goto unwind;
        }
        XDECREF(arg);
        arg = Array_GetItem(args, argIndex++);
        
        switch (convOp) {
        case 'c':
            charArg = CAST(Char, arg);
            if (!charArg) {
                Halt(HALT_TYPE_ERROR, "character expected");
                goto unwind;
            }
            fprintf(self->stream, chunk, (int)Char_AsCChar(charArg));
            break;
        case 'd': case 'i': case 'o': case 'u': case 'x':
            intArg = CAST(Integer, arg);
            if (!intArg) {
                Halt(HALT_TYPE_ERROR, "integer expected");
                goto unwind;
            }
            fprintf(self->stream, chunk, Integer_AsCLong(intArg));
            break;
        case 'e': case 'E': case 'f': case 'g': case 'G':
            floatArg = CAST(Float, arg);
            if (!floatArg) {
                Halt(HALT_TYPE_ERROR, "float expected");
                goto unwind;
            }
            fprintf(self->stream, chunk, Float_AsCDouble(floatArg));
            break;
        case 's':
            strArg = CAST(String, arg);
            if (!strArg) {
                Halt(HALT_TYPE_ERROR, "string expected");
                goto unwind;
            }
            fprintf(self->stream, chunk, String_AsCString(strArg));
            break;
        default:
            Halt(HALT_ASSERTION_ERROR, "conversion letter not implemented");
            goto unwind;
        }
        
        *f = c;
        chunk = f;
    }
    
    if (argIndex != nArgs) {
        Halt(HALT_TYPE_ERROR, "too many arguments");
        goto unwind;
    }
    
    free(format);
    DECREF(formatObj);
    XDECREF(arg);
    INCREF(GLOBAL(xvoid));
    return GLOBAL(xvoid);
    
 unwind:
    free(format);
    XDECREF(formatObj);
    XDECREF(arg);
    return 0;
}

static Unknown *FileStream_putc(Unknown *_self, Unknown *arg0, Unknown *arg1) {
    FileStream *self;
    Char *c;
    
    self = (FileStream *)_self;
    c = CAST(Char, arg0);
    if (!c) {
        Halt(HALT_TYPE_ERROR, "a character is required");
        return 0;
    }
    if (self->stream)
        fputc(Char_AsCChar(c), self->stream);
    INCREF(GLOBAL(xvoid));
    return GLOBAL(xvoid);
}

static Unknown *FileStream_puts(Unknown *_self, Unknown *arg0, Unknown *arg1) {
    FileStream *self;
    String *s;
    
    self = (FileStream *)_self;
    s = CAST(String, arg0);
    if (!s) {
        Halt(HALT_TYPE_ERROR, "a string is required");
        return 0;
    }
    if (self->stream)
        fputs(String_AsCString(s), self->stream);
    INCREF(GLOBAL(xvoid));
    return GLOBAL(xvoid);
}

static Unknown *FileStream_reopen(Unknown *_self, Unknown *arg0, Unknown *arg1) {
    FileStream *self;
    String *pathnameString, *modeString;
    char *pathname, *mode;
    Unknown *result;
    
    self = (FileStream *)_self;
    pathnameString = CAST(String, arg0);
    if (!pathnameString) {
        Halt(HALT_TYPE_ERROR, "a string is required");
        return 0;
    }
    modeString = CAST(String, arg1);
    if (!modeString) {
        Halt(HALT_TYPE_ERROR, "a string is required");
        return 0;
    }
    pathname = String_AsCString(pathnameString);
    mode = String_AsCString(modeString);
    if (self->stream)
        self->stream = freopen(pathname, mode, self->stream);
    else
        self->stream = fopen(pathname, mode);
    result = self->stream ? (Unknown *)self : GLOBAL(null);
    INCREF(result);
    return result;
}

static Unknown *FileStream_rewind(Unknown *_self, Unknown *arg0, Unknown *arg1) {
    FileStream *self;
    
    self = (FileStream *)_self;
    if (self->stream)
        rewind(self->stream);
    INCREF(GLOBAL(xvoid));
    return GLOBAL(xvoid);
}


/*------------------------------------------------------------------------*/
/* meta-methods */

static Unknown *ClassFileStream_open(Unknown *self, Unknown *arg0, Unknown *arg1) {
    String *pathnameString, *modeString;
    char *pathname, *mode;
    FILE *stream;
    Unknown *tmp;
    FileStream *newStream;
    
    pathnameString = CAST(String, arg0);
    if (!pathnameString) {
        Halt(HALT_TYPE_ERROR, "a string is required");
        return 0;
    }
    modeString = CAST(String, arg1);
    if (!modeString) {
        Halt(HALT_TYPE_ERROR, "a string is required");
        return 0;
    }
    pathname = String_AsCString(pathnameString);
    mode = String_AsCString(modeString);
    
    stream = fopen(pathname, mode);
    if (!stream) {
        INCREF(GLOBAL(null));
        return GLOBAL(null);
    }
    
    tmp = Send(GLOBAL(theInterpreter), self, new, 0);
    if (!tmp)
        return 0;
    newStream = CAST(FileStream, tmp);
    if (!newStream) {
        Halt(HALT_TYPE_ERROR, "FileStream expected");
        DECREF(tmp);
        return 0;
    }
    
    /* XXX: Note how this initialization is invisible to subclasses.
     * The stream should be an argument to 'init', but Spike -- unlike
     * Objective-C, for example -- doesn't allow C types in method
     * signatures.
     */
    newStream->stream = stream;
    
    return (Unknown *)newStream;
}


/*------------------------------------------------------------------------*/
/* low-level hooks */

static void FileStream_zero(Object *_self) {
    FileStream *self = (FileStream *)_self;
    (*CLASS(FileStream)->superclass->zero)(_self);
    self->stream = 0;
}

static void FileStream_dealloc(Object *_self, Unknown **l) {
    FileStream *self = (FileStream *)_self;
    if (self->stream &&
        /* XXX: clumsy */
        self->stream != stdin &&
        self->stream != stdout &&
        self->stream != stderr)
        fclose(self->stream);
    (*CLASS(FileStream)->superclass->dealloc)(_self, l);
}


/*------------------------------------------------------------------------*/
/* class tmpl */

typedef struct FileStreamSubclass {
    FileStream base;
    Unknown *variables[1]; /* stretchy */
} FileStreamSubclass;

static MethodTmpl methods[] = {
    /* attributes */
    { "eof", NativeCode_ARGS_0, &FileStream_eof },
    /* methods */
    { "close",  NativeCode_ARGS_0, &FileStream_close },
    { "flush",  NativeCode_ARGS_0, &FileStream_flush },
    { "getc",   NativeCode_ARGS_0, &FileStream_getc },
    { "gets",   NativeCode_ARGS_1, &FileStream_gets },
    { "open",   NativeCode_ARGS_2, &FileStream_reopen },
    { "printf", NativeCode_ARGS_ARRAY, &FileStream_printf },
    { "putc",   NativeCode_ARGS_1, &FileStream_putc },
    { "puts",   NativeCode_ARGS_1, &FileStream_puts },
    { "reopen", NativeCode_ARGS_2, &FileStream_reopen },
    { "rewind", NativeCode_ARGS_0, &FileStream_rewind },
    { 0 }
};

static MethodTmpl metaMethods[] = {
    { "open",   NativeCode_ARGS_2, &ClassFileStream_open },
    { 0 }
};

ClassTmpl ClassFileStreamTmpl = {
    HEART_CLASS_TMPL(FileStream, Object), {
        /*accessors*/ 0,
        methods,
        /*lvalueMethods*/ 0,
        offsetof(FileStreamSubclass, variables),
        0,
        &FileStream_zero,
        &FileStream_dealloc
    }, /*meta*/ {
        /*accessors*/ 0,
        metaMethods,
        /*lvalueMethods*/ 0
    }
};


/*------------------------------------------------------------------------*/
/* C API */

int IO_Boot(void) {
    GLOBAL(xstdin)->stream = stdin;
    GLOBAL(xstdout)->stream = stdout;
    GLOBAL(xstderr)->stream = stderr;
    return 1;
}

FILE *FileStream_AsCFileStream(FileStream *self) {
    return self->stream;
}
