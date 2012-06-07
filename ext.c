
#include "lexer.h"
#include "parser.h"

#include <stdio.h>
#include <Python.h>



static PyObject *parse(PyObject *self, PyObject *args) {
    PyObject *pyin; FILE *in;
    void *parser; yyscan_t lexer;
    char buffer[1024];
    int id, token;
    
    (void)self;
    if (!PyArg_ParseTuple(args, "O!:parse", &PyFile_Type, &pyin))
        return 0;
    in = PyFile_AsFile(pyin);
    
    Lexer_lex_init(&lexer);
    Lexer_restart(in, lexer);
    Lexer_set_lineno(2, lexer);
    Lexer_set_column(1, lexer);
    
    parser = Parser_ParseAlloc(&malloc);
    
    while ((id = Lexer_GetNextToken(&token, lexer)))
        Parser_Parse(parser, id, token);
    if (id != -1)
        Parser_Parse(parser, 0, token);
    
    Parser_ParseFree(parser, &free);
    Lexer_lex_destroy(lexer);
    
    Py_INCREF(Py_None);
    return Py_None;
}



static struct PyMethodDef methods[] = {
    { "parse", (PyCFunction)&parse, METH_VARARGS, 0 },
    { 0 }
};



PyMODINIT_FUNC initext(void) {
    Py_InitModule3("ext", methods, 0);
}

