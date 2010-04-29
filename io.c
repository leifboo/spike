
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


struct SpkFileStream {
    SpkObject base;
    FILE *stream;
};


/*------------------------------------------------------------------------*/
/* attributes */

static SpkUnknown *FileStream_eof(SpkUnknown *_self, SpkUnknown *arg0, SpkUnknown *arg1) {
    SpkFileStream *self;
    SpkUnknown *result;
    
    self = (SpkFileStream *)_self;
    result = self->stream
             ? (feof(self->stream) ? Spk_GLOBAL(xtrue) : Spk_GLOBAL(xfalse))
             : Spk_GLOBAL(xtrue);
    Spk_INCREF(result);
    return result;
}


/*------------------------------------------------------------------------*/
/* methods */

static SpkUnknown *FileStream_close(SpkUnknown *_self, SpkUnknown *arg0, SpkUnknown *arg1) {
    SpkFileStream *self;
    
    self = (SpkFileStream *)_self;
    if (self->stream) {
        fclose(self->stream);
        self->stream = 0;
    }
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
}

static SpkUnknown *FileStream_flush(SpkUnknown *_self, SpkUnknown *arg0, SpkUnknown *arg1) {
    SpkFileStream *self;
    
    self = (SpkFileStream *)_self;
    if (self->stream)
        fflush(self->stream);
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
}

static SpkUnknown *FileStream_getc(SpkUnknown *_self, SpkUnknown *arg0, SpkUnknown *arg1) {
    SpkFileStream *self;
    int c;
    
    self = (SpkFileStream *)_self;
    if (!self->stream) {
        Spk_INCREF(Spk_GLOBAL(null));
        return Spk_GLOBAL(null);
    }
    c = fgetc(self->stream);
    if (c == EOF) {
        Spk_INCREF(Spk_GLOBAL(null));
        return Spk_GLOBAL(null);
    }
    return (SpkUnknown *)SpkChar_FromCChar((char)c);
}

static SpkUnknown *FileStream_gets(SpkUnknown *_self, SpkUnknown *arg0, SpkUnknown *arg1) {
    SpkFileStream *self;
    SpkInteger *size;
    SpkUnknown *result;
    
    self = (SpkFileStream *)_self;
    size = Spk_CAST(Integer, arg0);
    if (!size) {
        Spk_Halt(Spk_HALT_TYPE_ERROR, "an integer is required");
        return 0;
    }
    if (!self->stream) {
        Spk_INCREF(Spk_GLOBAL(null));
        return Spk_GLOBAL(null);
    }
    result = (SpkUnknown *)SpkString_FromCStream(self->stream, (size_t)SpkInteger_AsCPtrdiff(size));
    if (!result) {
        Spk_INCREF(Spk_GLOBAL(null));
        return Spk_GLOBAL(null);
    }
    return result;
}

static SpkUnknown *FileStream_printf(SpkUnknown *_self, SpkUnknown *arg0, SpkUnknown *arg1) {
    SpkFileStream *self;
    SpkArray *args; size_t nArgs, argIndex;
    SpkUnknown *formatObj = 0; SpkString *formatString; char *format = 0; size_t formatSize;
    char c, convOp, *f, *chunk;
    SpkUnknown *arg = 0; SpkChar *charArg; SpkInteger *intArg; SpkFloat *floatArg; SpkString *strArg;
    static const char *convOps = "cdeEfgGinopsuxX%";
    
    self = (SpkFileStream *)_self;
    args = Spk_CAST(Array, arg0);
    if (!args) {
        Spk_Halt(Spk_HALT_TYPE_ERROR, "an array is required");
        goto unwind;
    }
    nArgs = SpkArray_Size(args);
    if (nArgs == 0) {
        Spk_Halt(Spk_HALT_TYPE_ERROR, "wrong number of arguments");
        goto unwind;
    }
    formatObj = SpkArray_GetItem(args, 0);
    formatString = Spk_CAST(String, formatObj);
    if (!formatString) {
        Spk_Halt(Spk_HALT_TYPE_ERROR, "a string is required");
        goto unwind;
    }
    formatSize = SpkString_Size(formatString);
    format = (char *)malloc(formatSize);
    memcpy(format, SpkString_AsCString(formatString), formatSize);
    
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
            Spk_Halt(Spk_HALT_VALUE_ERROR, "invalid conversion specification");
            goto unwind;
        }
        
        convOp = c;
        c = *++f;
        if (convOp == '%') {
            if (f - chunk != 2) {
                Spk_Halt(Spk_HALT_VALUE_ERROR, "invalid conversion specification");
                goto unwind;
            }
            fputc('%', self->stream);
            chunk = f;
            continue;
        }
        *f = 0;
        
        /* consume an argument */
        if (argIndex >= nArgs) {
            Spk_Halt(Spk_HALT_TYPE_ERROR, "too few arguments");
            goto unwind;
        }
        Spk_XDECREF(arg);
        arg = SpkArray_GetItem(args, argIndex++);
        
        switch (convOp) {
        case 'c':
            charArg = Spk_CAST(Char, arg);
            if (!charArg) {
                Spk_Halt(Spk_HALT_TYPE_ERROR, "character expected");
                goto unwind;
            }
            fprintf(self->stream, chunk, (int)SpkChar_AsCChar(charArg));
            break;
        case 'd': case 'i': case 'o': case 'u': case 'x':
            intArg = Spk_CAST(Integer, arg);
            if (!intArg) {
                Spk_Halt(Spk_HALT_TYPE_ERROR, "integer expected");
                goto unwind;
            }
            fprintf(self->stream, chunk, SpkInteger_AsCLong(intArg));
            break;
        case 'e': case 'E': case 'f': case 'g': case 'G':
            floatArg = Spk_CAST(Float, arg);
            if (!floatArg) {
                Spk_Halt(Spk_HALT_TYPE_ERROR, "float expected");
                goto unwind;
            }
            fprintf(self->stream, chunk, SpkFloat_AsCDouble(floatArg));
            break;
        case 's':
            strArg = Spk_CAST(String, arg);
            if (!strArg) {
                Spk_Halt(Spk_HALT_TYPE_ERROR, "string expected");
                goto unwind;
            }
            fprintf(self->stream, chunk, SpkString_AsCString(strArg));
            break;
        default:
            Spk_Halt(Spk_HALT_ASSERTION_ERROR, "conversion letter not implemented");
            goto unwind;
        }
        
        *f = c;
        chunk = f;
    }
    
    if (argIndex != nArgs) {
        Spk_Halt(Spk_HALT_TYPE_ERROR, "too many arguments");
        goto unwind;
    }
    
    free(format);
    Spk_DECREF(formatObj);
    Spk_XDECREF(arg);
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
    
 unwind:
    free(format);
    Spk_XDECREF(formatObj);
    Spk_XDECREF(arg);
    return 0;
}

static SpkUnknown *FileStream_putc(SpkUnknown *_self, SpkUnknown *arg0, SpkUnknown *arg1) {
    SpkFileStream *self;
    SpkChar *c;
    
    self = (SpkFileStream *)_self;
    c = Spk_CAST(Char, arg0);
    if (!c) {
        Spk_Halt(Spk_HALT_TYPE_ERROR, "a character is required");
        return 0;
    }
    if (self->stream)
        fputc(SpkChar_AsCChar(c), self->stream);
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
}

static SpkUnknown *FileStream_puts(SpkUnknown *_self, SpkUnknown *arg0, SpkUnknown *arg1) {
    SpkFileStream *self;
    SpkString *s;
    
    self = (SpkFileStream *)_self;
    s = Spk_CAST(String, arg0);
    if (!s) {
        Spk_Halt(Spk_HALT_TYPE_ERROR, "a string is required");
        return 0;
    }
    if (self->stream)
        fputs(SpkString_AsCString(s), self->stream);
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
}

static SpkUnknown *FileStream_reopen(SpkUnknown *_self, SpkUnknown *arg0, SpkUnknown *arg1) {
    SpkFileStream *self;
    SpkString *pathnameString, *modeString;
    char *pathname, *mode;
    SpkUnknown *result;
    
    self = (SpkFileStream *)_self;
    pathnameString = Spk_CAST(String, arg0);
    if (!pathnameString) {
        Spk_Halt(Spk_HALT_TYPE_ERROR, "a string is required");
        return 0;
    }
    modeString = Spk_CAST(String, arg1);
    if (!modeString) {
        Spk_Halt(Spk_HALT_TYPE_ERROR, "a string is required");
        return 0;
    }
    pathname = SpkString_AsCString(pathnameString);
    mode = SpkString_AsCString(modeString);
    if (self->stream)
        self->stream = freopen(pathname, mode, self->stream);
    else
        self->stream = fopen(pathname, mode);
    result = self->stream ? (SpkUnknown *)self : Spk_GLOBAL(null);
    Spk_INCREF(result);
    return result;
}

static SpkUnknown *FileStream_rewind(SpkUnknown *_self, SpkUnknown *arg0, SpkUnknown *arg1) {
    SpkFileStream *self;
    
    self = (SpkFileStream *)_self;
    if (self->stream)
        rewind(self->stream);
    Spk_INCREF(Spk_GLOBAL(xvoid));
    return Spk_GLOBAL(xvoid);
}


/*------------------------------------------------------------------------*/
/* meta-methods */

static SpkUnknown *ClassFileStream_open(SpkUnknown *self, SpkUnknown *arg0, SpkUnknown *arg1) {
    SpkString *pathnameString, *modeString;
    char *pathname, *mode;
    FILE *stream;
    SpkUnknown *tmp;
    SpkFileStream *newStream;
    
    pathnameString = Spk_CAST(String, arg0);
    if (!pathnameString) {
        Spk_Halt(Spk_HALT_TYPE_ERROR, "a string is required");
        return 0;
    }
    modeString = Spk_CAST(String, arg1);
    if (!modeString) {
        Spk_Halt(Spk_HALT_TYPE_ERROR, "a string is required");
        return 0;
    }
    pathname = SpkString_AsCString(pathnameString);
    mode = SpkString_AsCString(modeString);
    
    stream = fopen(pathname, mode);
    if (!stream) {
        Spk_INCREF(Spk_GLOBAL(null));
        return Spk_GLOBAL(null);
    }
    
    tmp = Spk_Send(Spk_GLOBAL(theInterpreter), self, Spk_new, 0);
    if (!tmp)
        return 0;
    newStream = Spk_CAST(FileStream, tmp);
    if (!newStream) {
        Spk_Halt(Spk_HALT_TYPE_ERROR, "FileStream expected");
        Spk_DECREF(tmp);
        return 0;
    }
    
    /* XXX: Note how this initialization is invisible to subclasses.
     * The stream should be an argument to 'init', but Spike -- unlike
     * Objective-C, for example -- doesn't allow C types in method
     * signatures.
     */
    newStream->stream = stream;
    
    return (SpkUnknown *)newStream;
}


/*------------------------------------------------------------------------*/
/* low-level hooks */

static void FileStream_zero(SpkObject *_self) {
    SpkFileStream *self = (SpkFileStream *)_self;
    (*Spk_CLASS(FileStream)->superclass->zero)(_self);
    self->stream = 0;
}

static void FileStream_dealloc(SpkObject *_self, SpkUnknown **l) {
    SpkFileStream *self = (SpkFileStream *)_self;
    if (self->stream)
        fclose(self->stream);
    (*Spk_CLASS(FileStream)->superclass->dealloc)(_self, l);
}


/*------------------------------------------------------------------------*/
/* class tmpl */

typedef struct SpkFileStreamSubclass {
    SpkFileStream base;
    SpkUnknown *variables[1]; /* stretchy */
} SpkFileStreamSubclass;

static SpkMethodTmpl methods[] = {
    /* attributes */
    { "eof", SpkNativeCode_ARGS_0, &FileStream_eof },
    /* methods */
    { "close",  SpkNativeCode_ARGS_0, &FileStream_close },
    { "flush",  SpkNativeCode_ARGS_0, &FileStream_flush },
    { "getc",   SpkNativeCode_ARGS_0, &FileStream_getc },
    { "gets",   SpkNativeCode_ARGS_1, &FileStream_gets },
    { "open",   SpkNativeCode_ARGS_2, &FileStream_reopen },
    { "printf", SpkNativeCode_ARGS_ARRAY, &FileStream_printf },
    { "putc",   SpkNativeCode_ARGS_1, &FileStream_putc },
    { "puts",   SpkNativeCode_ARGS_1, &FileStream_puts },
    { "reopen", SpkNativeCode_ARGS_2, &FileStream_reopen },
    { "rewind", SpkNativeCode_ARGS_0, &FileStream_rewind },
    { 0 }
};

static SpkMethodTmpl metaMethods[] = {
    { "open",   SpkNativeCode_ARGS_2, &ClassFileStream_open },
    { 0 }
};

SpkClassTmpl Spk_ClassFileStreamTmpl = {
    Spk_HEART_CLASS_TMPL(FileStream, Object), {
        /*accessors*/ 0,
        methods,
        /*lvalueMethods*/ 0,
        offsetof(SpkFileStreamSubclass, variables),
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

int SpkIO_Boot(void) {
    Spk_GLOBAL(xstdin)->stream = stdin;
    Spk_GLOBAL(xstdout)->stream = stdout;
    Spk_GLOBAL(xstderr)->stream = stderr;
    return 1;
}

FILE *SpkFileStream_AsCFileStream(SpkFileStream *self) {
    return self->stream;
}
