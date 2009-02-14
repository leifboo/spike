
#include "io.h"

#include "array.h"
#include "behavior.h"
#include "bool.h"
#include "char.h"
#include "float.h"
#include "int.h"
#include "str.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>


Behavior *Spk_ClassFileStream;
FileStream *Spk_stdin, *Spk_stdout, *Spk_stderr;


/*------------------------------------------------------------------------*/
/* attributes */

static Object *FileStream_eof(Object *_self, Object *arg0, Object *arg1) {
    FileStream *self;
    
    self = (FileStream *)_self;
    return self->stream
           ? (feof(self->stream) ? Spk_true : Spk_false)
           : Spk_true;
}


/*------------------------------------------------------------------------*/
/* methods */

static Object *FileStream_close(Object *_self, Object *arg0, Object *arg1) {
    FileStream *self;
    
    self = (FileStream *)_self;
    if (self->stream) {
        fclose(self->stream);
        self->stream = 0;
    }
    return Spk_void;
}

static Object *FileStream_flush(Object *_self, Object *arg0, Object *arg1) {
    FileStream *self;
    
    self = (FileStream *)_self;
    if (self->stream)
        fflush(self->stream);
    return Spk_void;
}

static Object *FileStream_getc(Object *_self, Object *arg0, Object *arg1) {
    FileStream *self;
    int c;
    
    self = (FileStream *)_self;
    if (!self->stream)
        return Spk_null;
    c = fgetc(self->stream);
    if (c == EOF)
        return Spk_null;
    return (Object *)SpkChar_fromChar((char)c);
}

static Object *FileStream_gets(Object *_self, Object *arg0, Object *arg1) {
    FileStream *self;
    Integer *size;
    String *s;
    
    self = (FileStream *)_self;
    assert(size = Spk_CAST(Integer, arg0)); /* XXX */
    if (!self->stream)
        return Spk_null;
    s = SpkString_fromStream(self->stream, (size_t)SpkInteger_asLong(size));
    return s ? (Object *)s : Spk_null;
}

static Object *FileStream_printf(Object *_self, Object *arg0, Object *arg1) {
    FileStream *self;
    Array *args; size_t nArgs, argIndex;
    String *formatString; char *format; size_t formatSize;
    char c, convOp, *f, *chunk;
    Object *arg; Char *charArg; Integer *intArg; Float *floatArg; String *strArg;
    static char *convOps = "cdeEfgGinopsuxX%";
    
    self = (FileStream *)_self;
    assert(args = Spk_CAST(Array, arg0)); /* XXX */
    nArgs = SpkArray_size(args);
    assert(nArgs > 0 && "wrong number of arguments");
    assert(formatString = Spk_CAST(String, SpkArray_item(args, 0))); /* XXX */
    formatSize = SpkString_size(formatString);
    format = (char *)malloc(formatSize);
    memcpy(format, SpkString_asString(formatString), formatSize);
    
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
        assert(c && "invalid conversion specification");
        
        convOp = c;
        c = *++f;
        if (convOp == '%') {
            assert(f - chunk == 2 && "invalid conversion specification");
            fputc('%', self->stream);
            chunk = f;
            continue;
        }
        *f = 0;
        
        /* consume an argument */
        assert(argIndex < nArgs && "too few arguments");
        arg = SpkArray_item(args, argIndex++);
        
        switch (convOp) {
        case 'c':
            assert(charArg = Spk_CAST(Char, arg));
            fprintf(self->stream, chunk, SpkChar_asChar(charArg));
            break;
        case 'd': case 'i': case 'o': case 'u': case 'x':
            assert(intArg = Spk_CAST(Integer, arg));
            fprintf(self->stream, chunk, SpkInteger_asLong(intArg));
            break;
        case 'e': case 'E': case 'f': case 'g': case 'G':
            assert(floatArg = Spk_CAST(Float, arg));
            fprintf(self->stream, chunk, SpkFloat_asDouble(floatArg));
            break;
        case 's':
            assert(strArg = Spk_CAST(String, arg));
            fprintf(self->stream, chunk, SpkString_asString(strArg));
            break;
        default:
            assert(0 && "conversion letter not implemented");
        }
        
        *f = c;
        chunk = f;
    }
    
    assert(argIndex == nArgs && "too many arguments");
    
    free(format);
    return Spk_void;
}

static Object *FileStream_putc(Object *_self, Object *arg0, Object *arg1) {
    FileStream *self;
    Char *c;
    
    self = (FileStream *)_self;
    assert(c = Spk_CAST(Char, arg0)); /* XXX */
    if (self->stream)
        fputc(SpkChar_asChar(c), self->stream);
    return Spk_void;
}

static Object *FileStream_puts(Object *_self, Object *arg0, Object *arg1) {
    FileStream *self;
    String *s;
    
    self = (FileStream *)_self;
    assert(s = Spk_CAST(String, arg0)); /* XXX */
    if (self->stream)
        fputs(SpkString_asString(s), self->stream);
    return Spk_void;
}

static Object *FileStream_reopen(Object *_self, Object *arg0, Object *arg1) {
    /* XXX: "open" should be a class method */
    FileStream *self;
    String *pathnameString, *modeString;
    char *pathname, *mode;
    
    self = (FileStream *)_self;
    assert(pathnameString = Spk_CAST(String, arg0)); /* XXX */
    assert(modeString = Spk_CAST(String, arg1)); /* XXX */
    pathname = SpkString_asString(pathnameString);
    mode = SpkString_asString(modeString);
    if (self->stream)
        self->stream = freopen(pathname, mode, self->stream);
    else
        self->stream = fopen(pathname, mode);
    return self->stream ? (Object *)self : Spk_null;
}

static Object *FileStream_rewind(Object *_self, Object *arg0, Object *arg1) {
    FileStream *self;
    
    self = (FileStream *)_self;
    if (self->stream)
        rewind(self->stream);
    return Spk_void;
}


/*------------------------------------------------------------------------*/
/* low-level hooks */

static void FileStream_zero(Object *_self) {
    FileStream *self = (FileStream *)_self;
    (*Spk_ClassFileStream->superclass->zero)(_self);
    self->stream = 0;
}

static void FileStream_dealloc(Object *_self) {
    FileStream *self = (FileStream *)_self;
    if (self->stream) {
        fclose(self->stream);
        self->stream = 0;
    }
    (*Spk_ClassFileStream->superclass->dealloc)(_self);
}


/*------------------------------------------------------------------------*/
/* class template */

static SpkMethodTmpl methods[] = {
    /* attributes */
    { "eof", SpkNativeCode_ARGS_0, &FileStream_eof },
    /* methods */
    { "close",  SpkNativeCode_METH_ATTR | SpkNativeCode_ARGS_0, &FileStream_close },
    { "flush",  SpkNativeCode_METH_ATTR | SpkNativeCode_ARGS_0, &FileStream_flush },
    { "getc",   SpkNativeCode_METH_ATTR | SpkNativeCode_ARGS_0, &FileStream_getc },
    { "gets",   SpkNativeCode_METH_ATTR | SpkNativeCode_ARGS_1, &FileStream_gets },
    { "open",   SpkNativeCode_METH_ATTR | SpkNativeCode_ARGS_2, &FileStream_reopen },
    { "printf", SpkNativeCode_METH_ATTR | SpkNativeCode_ARGS_ARRAY, &FileStream_printf },
    { "putc",   SpkNativeCode_METH_ATTR | SpkNativeCode_ARGS_1, &FileStream_putc },
    { "puts",   SpkNativeCode_METH_ATTR | SpkNativeCode_ARGS_1, &FileStream_puts },
    { "reopen", SpkNativeCode_METH_ATTR | SpkNativeCode_ARGS_2, &FileStream_reopen },
    { "rewind", SpkNativeCode_METH_ATTR | SpkNativeCode_ARGS_0, &FileStream_rewind },
    { 0, 0, 0}
};

SpkClassTmpl Spk_ClassFileStreamTmpl = {
    "FileStream",
    offsetof(FileStreamSubclass, variables),
    sizeof(FileStream),
    0,
    0,
    methods,
    0,
    &FileStream_zero,
    &FileStream_dealloc
};
