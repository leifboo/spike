
#include "lexer.h"
#include "parser.h"

#include <stdio.h>
#include <Python.h>


FILE *spkout, *spkerr;


static PyObject *parse(PyObject *self, PyObject *args) {
    PyObject *pyin = 0, *pyout = 0, *pyerr = 0;
    FILE *spkin;
    void *parser; yyscan_t lexer;
    int id, token;
    
    (void)self;
    if (!PyArg_ParseTuple(args, "O!O!O!:parse",
                          &PyFile_Type, &pyin,
                          &PyFile_Type, &pyout,
                          &PyFile_Type, &pyerr))
        return 0;
    spkin  = PyFile_AsFile(pyin);
    spkout = PyFile_AsFile(pyout);
    spkerr = PyFile_AsFile(pyerr);
    
    Lexer_lex_init(&lexer);
    Lexer_restart(spkin, lexer);
    Lexer_set_lineno(2, lexer);
    Lexer_set_column(1, lexer);
    
    parser = Parser_ParseAlloc(&malloc);
    
    while ((id = Lexer_GetNextToken(&token, lexer)))
        Parser_Parse(parser, id, token, lexer);
    if (id != -1)
        Parser_Parse(parser, 0, token, lexer);
    
    Parser_ParseFree(parser, &free);
    Lexer_lex_destroy(lexer);
    
    Py_INCREF(Py_None);
    return Py_None;
}


#ifndef NDEBUG
static PyObject *trace(PyObject *self, PyObject *args) {
    PyObject *pyTraceFile = 0;
    FILE *traceFile; char *tracePrompt;
    
    (void)self;
    if (!PyArg_ParseTuple(args, "O!s:trace",
                          &PyFile_Type, &pyTraceFile,
                          &tracePrompt))
        return 0;
    traceFile = PyFile_AsFile(pyTraceFile);
    
    Parser_ParseTrace(traceFile, tracePrompt);
    
    Py_INCREF(Py_None);
    return Py_None;
}
#endif



static struct PyMethodDef methods[] = {
    { "parse", (PyCFunction)&parse, METH_VARARGS, 0 },
#ifndef NDEBUG
    { "trace", (PyCFunction)&trace, METH_VARARGS, 0 },
#endif
    { 0 }
};



PyMODINIT_FUNC initext(void) {
    Py_InitModule3("ext", methods, 0);
}

