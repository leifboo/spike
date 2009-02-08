
#include "io.h"

#include <assert.h>
#include "behavior.h"
#include "bool.h"
#include "char.h"
#include "int.h"
#include "str.h"


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
    (*self->base.klass->superclass->zero)(_self);
    self->stream = 0;
}

static void FileStream_dealloc(Object *_self) {
    FileStream *self = (FileStream *)_self;
    if (self->stream) {
        fclose(self->stream);
        self->stream = 0;
    }
    (*self->base.klass->superclass->dealloc)(_self);
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
