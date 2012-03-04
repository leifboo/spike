
#include "host.h"

#include "array.h"
#include "behavior.h"
#include "char.h"
#include "dict.h"
#include "float.h"
#include "heart.h"
#include "int.h"
#include "interp.h"
#include "io.h"
#include "native.h"
#include "rodata.h"
#include "str.h"
#include "sym.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


char *kwSep[2] = { " ", ":" };


/****/ void Host_Init(void) {
}


/*------------------------------------------------------------------------*/
/* halting */

static const char *haltDesc(int code) {
    const char *desc;
    
    switch (code) {
    case HALT_ASSERTION_ERROR: desc = "assertion error"; break;
    case HALT_ERROR:           desc = "error";           break;
    case HALT_INDEX_ERROR:     desc = "index error";     break;
    case HALT_KEY_ERROR:       desc = "key error";       break;
    case HALT_MEMORY_ERROR:    desc = "memory error";    break;
    case HALT_RUNTIME_ERROR:   desc = "runtime error";   break;
    case HALT_TYPE_ERROR:      desc = "type error";      break;
    case HALT_VALUE_ERROR:     desc = "value error";     break;
    default:
        desc = "unknown";
        break;
    }
    return desc;
}


/****/ void Host_Halt(int code, const char *message) {
    if (GLOBAL(theInterpreter))
        Interpreter_PrintCallStack(GLOBAL(theInterpreter));
    fprintf(stderr, "halt: %s: %s\n", haltDesc(code), message);
    abort(); /* XXX: The proper thing to do is unwind. */
}

/****/ void Host_HaltWithString(int code, Unknown *message) {
    String *str = CAST(String, message);
    Host_Halt(code, String_AsCString(str));
}

/****/ void Host_VHaltWithFormat(int code, const char *format, va_list args) {
    fprintf(stderr, "halt: %s: ", haltDesc(code));
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    abort(); /* XXX */
}


/*------------------------------------------------------------------------*/
/* base types */

/*****/ int Host_IsInteger(Unknown *op) {
    return CAST(Integer, op) != (Integer *)0;
}

/*****/ int Host_IsString(Unknown *op) {
    return CAST(String, op) != (String *)0;
}

/****/ long Host_IntegerAsCLong(Unknown *op) {
    return Integer_AsCLong(CAST(Integer, op));
}

Unknown *Host_IntegerFromCLong(long value) {
    return (Unknown *)Integer_FromCLong(value);
}

/**/ double Host_FloatAsCDouble(Unknown *op) {
    return Float_AsCDouble(CAST(Float, op));
}

Unknown *Host_FloatFromCDouble(double value) {
    return (Unknown *)Float_FromCDouble(value);
}

/****/ char Host_CharAsCChar(Unknown *op) {
    return Char_AsCChar(CAST(Char, op));
}

Unknown *Host_CharFromCChar(char value) {
    return (Unknown *)Char_FromCChar(value);
}

/***/ char *Host_StringAsCString(Unknown *op) {
    String *str = CAST(String, op);
    return String_AsCString(str);
}

Unknown *Host_StringFromCString(const char *str) {
    return (Unknown *)String_FromCString(str);
}

Unknown *Host_StringFromCStringAndLength(const char *str, size_t len) {
    return (Unknown *)String_FromCStringAndLength(str, len);
}

Unknown *Host_StringConcat(Unknown **a, Unknown *b) {
    return (Unknown *)String_Concat((String **)a, CAST(String, b));
}


/*------------------------------------------------------------------------*/
/* symbols */

/*****/ int Host_IsSymbol(Unknown *op) {
    return CAST(Symbol, op) != (Symbol *)0;
}

const char *Host_SymbolAsCString(Unknown *unk) {
    Symbol *sym = CAST(Symbol, unk);
    if (sym) {
        return Symbol_AsCString(sym);
    }
    return 0;
}

Unknown *Host_SymbolFromCString(const char *str) {
    return (Unknown *)Symbol_FromCString(str);
}

Unknown *Host_SymbolFromCStringAndLength(const char *str, size_t len) {
    return (Unknown *)Symbol_FromCStringAndLength(str, len);
}


/*------------------------------------------------------------------------*/
/* selectors */

const char *Host_SelectorAsCString(Unknown *selector) {
    Symbol *sym = CAST(Symbol, selector);
    if (sym) {
        return Symbol_AsCString(sym);
    }
    return 0;
}

Unknown *Host_NewKeywordSelectorBuilder(void) {
    return (Unknown *)Array_New(2);
}

/****/ void Host_AppendKeyword(Unknown **builder, Unknown *kw) {
    Array *kwArray;
    size_t size, i;
    Unknown *vd;
    
    kwArray = CAST(Array, *builder);
    size = Array_Size(kwArray);
    for (i = 0; i < size; ++i) {
        Unknown *item = Array_GetItem(kwArray, i);
        if (item == GLOBAL(uninit)) {
            DECREF(item);
            break;
        }
        DECREF(item);
    }
    if (i == size) {
        Array *newArray;
        size_t newSize;
        
        newSize = size * 2;
        newArray = Array_New(newSize);
        for (i = 0; i < size; ++i) {
            Unknown *item = Array_GetItem(kwArray, i);
            vd = Array_SetItem(newArray, i, item);
            DECREF(vd);
            DECREF(item);
        }
        DECREF(kwArray);
        kwArray = newArray;
        *builder = (Unknown *)kwArray;
    }
    vd = Array_SetItem(kwArray, i, kw);
    DECREF(vd);
}

static int compareSymbols(Unknown *const *p1, Unknown *const *p2) {
    Symbol *sym1, *sym2;
    const char *s1, *s2;
    
    sym1 = CAST(Symbol, *p1);
    sym2 = CAST(Symbol, *p2);
    if (!sym1 && !sym2)
        return 0;
    if (!sym1)
        return 1;
    if (!sym2)
        return -1;
    s1 = Symbol_AsCString(sym1);
    s2 = Symbol_AsCString(sym2);
    return strcmp(s1, s2);
}

Unknown *Host_GetKeywordSelector(Unknown *builder, Unknown *kw) {
    Array *kwArray;
    Symbol *sym, *selector;
    size_t size, len, i, tally;
    char *str;
    
    kwArray = CAST(Array, builder);
    size = Array_Size(kwArray);
    len = 0;
    
    if (kw) {
        sym = CAST(Symbol, kw);
        len += strlen(Symbol_AsCString(sym));
        if (0) {
            /* XXX: The formal arguments must be permuted in the same
               way. */
            Array_Sort(kwArray, &compareSymbols);
        }
    }
    for (i = 0; i < size; ++i) {
        Unknown *item = Array_GetItem(kwArray, i);
        if (item == GLOBAL(uninit)) {
            DECREF(item);
            break;
        }
        sym = CAST(Symbol, item);
        len += strlen(Symbol_AsCString(sym)) + strlen(kwSep[1]);
        DECREF(item);
    }
    
    tally = i;
    if (tally > 0)
        len += strlen(kwSep[0]);
    
    str = (char *)malloc(len + 1);
    str[0] = '\0';
    
    if (kw) {
        sym = CAST(Symbol, kw);
        strcat(str, Symbol_AsCString(sym));
        if (tally > 0)
            strcat(str, kwSep[0]);
    }
    for (i = 0; i < size; ++i) {
        Unknown *item = Array_GetItem(kwArray, i);
        if (item == GLOBAL(uninit)) {
            DECREF(item);
            break;
        }
        sym = CAST(Symbol, item);
        strcat(str, Symbol_AsCString(sym));
        strcat(str, kwSep[1]);
        DECREF(item);
    }
    str[len] = '\0';
    
    selector = Symbol_FromCString(str);
    free(str);
    return (Unknown *)selector;
}


/*------------------------------------------------------------------------*/
/* symbol dictionaries */

Unknown *Host_NewSymbolDict(void) {
    return (Unknown *)IdentityDictionary_New();
}

/****/ void Host_DefineSymbol(Unknown *symbolDict, Unknown *selector, Unknown *method) {
    IdentityDictionary *dict = CAST(IdentityDictionary, symbolDict);
    IdentityDictionary_SetItem(dict, selector, method);
}

Unknown *Host_SymbolValue(Unknown *symbolDict, Unknown *key) {
    IdentityDictionary *dict = CAST(IdentityDictionary, symbolDict);
    return IdentityDictionary_GetItem(dict, key);
}

Unknown *Host_FindSymbol(Unknown *symbolDict, Unknown *method) {
    IdentityDictionary *dict = CAST(IdentityDictionary, symbolDict);
    return IdentityDictionary_KeyWithValue(dict, method);
}

/*****/ int Host_NextSymbol(Unknown *symbolDict,
                               size_t *pPos,
                               Unknown **pKey,
                               Unknown **pValue)
{
    IdentityDictionary *dict;
    
    dict = CAST(IdentityDictionary, symbolDict);
    return IdentityDictionary_Next(dict, pPos, pKey, pValue);
}


/*------------------------------------------------------------------------*/
/* arguments  */

/*****/ int Host_IsArgs(Unknown *op) {
    return CAST(Array, op) != (Array *)0;
}

/**/ size_t Host_ArgsSize(Unknown *args) {
    return Array_Size(CAST(Array, args));
}

Unknown *Host_GetArg(Unknown *args, size_t index) {
    return Array_GetItem(CAST(Array, args), index);
}

Unknown *Host_EmptyArgs(void) {
    return (Unknown *)Array_New(0);
}

Unknown *Host_ArgsFromVAList(va_list ap) {
    return (Unknown *)Array_FromVAList(ap);
}

Unknown *Host_GetArgs(Unknown **stackPointer,
                            size_t argumentCount,
                            Unknown *varArgs,
                            size_t skip)
{
    Array *varArgArray;
    
    varArgArray = varArgs ? CAST(Array, varArgs) : 0;
    return (Unknown *)Array_WithArguments(stackPointer,
                                                argumentCount,
                                                varArgArray,
                                                skip);
}


/*------------------------------------------------------------------------*/
/* i/o */

/*****/ int Host_IsFileStream(Unknown *op) {
    return CAST(FileStream, op) != (FileStream *)0;
}

/***/ FILE *Host_FileStreamAsCFileStream(Unknown *op) {
    FileStream *stream = CAST(FileStream, op);
    return FileStream_AsCFileStream(stream);
}


/*------------------------------------------------------------------------*/
/* as yet unclassified */

Unknown *Host_ObjectAsString(Unknown *obj) {
    return SendMessage(
        GLOBAL(theInterpreter),
        obj,
        METHOD_NAMESPACE_RVALUE,
        printString,
        emptyArgs
        );
}

/****/ void Host_PrintObject(Unknown *obj, FILE *stream) {
    Unknown *printObj;
    String *printString;
    char *str;
    
    printObj = Host_ObjectAsString(obj);
    if (!printObj)
        return;
    printString = CAST(String, printObj);
    if (printString) {
        str = String_AsCString(printString);
        fputs(str, stream);
    }
    DECREF(printObj);
}
