
#include "host.h"

#include "array.h"
#include "behavior.h"
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


/****/ void SpkHost_Init(void) {
}


/*------------------------------------------------------------------------*/
/* halting */

static const char *haltDesc(int code) {
    const char *desc;
    
    switch (code) {
    case Spk_HALT_ASSERTION_ERROR: desc = "assertion error"; break;
    case Spk_HALT_ERROR:           desc = "error";           break;
    case Spk_HALT_INDEX_ERROR:     desc = "index error";     break;
    case Spk_HALT_MEMORY_ERROR:    desc = "memory error";    break;
    case Spk_HALT_RUNTIME_ERROR:   desc = "runtime error";   break;
    case Spk_HALT_TYPE_ERROR:      desc = "type error";      break;
    case Spk_HALT_VALUE_ERROR:     desc = "value error";     break;
    default:
        desc = "unknown";
        break;
    }
    return desc;
}


/****/ void SpkHost_Halt(int code, const char *message) {
    if (theInterpreter)
        SpkInterpreter_PrintCallStack(theInterpreter);
    fprintf(stderr, "halt: %s: %s\n", haltDesc(code), message);
    abort(); /* XXX: The proper thing to do is unwind. */
}

/****/ void SpkHost_HaltWithString(int code, SpkUnknown *message) {
    SpkString *str = Spk_CAST(String, message);
    SpkHost_Halt(code, SpkString_AsCString(str));
}

/****/ void SpkHost_VHaltWithFormat(int code, const char *format, va_list args) {
    fprintf(stderr, "halt: %s: ", haltDesc(code));
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    abort(); /* XXX */
}


/*------------------------------------------------------------------------*/
/* base types */

/*****/ int SpkHost_IsInteger(SpkUnknown *op) {
    return Spk_CAST(Integer, op) != (SpkInteger *)0;
}

/*****/ int SpkHost_IsString(SpkUnknown *op) {
    return Spk_CAST(String, op) != (SpkString *)0;
}

/****/ long SpkHost_IntegerAsCLong(SpkUnknown *op) {
    return SpkInteger_AsCLong(Spk_CAST(Integer, op));
}

SpkUnknown *SpkHost_IntegerFromCLong(long value) {
    return (SpkUnknown *)SpkInteger_FromCLong(value);
}

SpkUnknown *SpkHost_FloatFromCDouble(double value) {
    return (SpkUnknown *)SpkFloat_FromCDouble(value);
}

/***/ char *SpkHost_StringAsCString(SpkUnknown *op) {
    SpkString *str = Spk_CAST(String, op);
    return SpkString_AsCString(str);
}

SpkUnknown *SpkHost_StringFromCString(const char *str) {
    return (SpkUnknown *)SpkString_FromCString(str);
}

SpkUnknown *SpkHost_StringFromCStringAndLength(const char *str, size_t len) {
    return (SpkUnknown *)SpkString_FromCStringAndLength(str, len);
}

SpkUnknown *SpkHost_StringConcat(SpkUnknown **a, SpkUnknown *b) {
    return (SpkUnknown *)SpkString_Concat((SpkString **)a, Spk_CAST(String, b));
}


/*------------------------------------------------------------------------*/
/* symbols */

/*****/ int SpkHost_IsSymbol(SpkUnknown *op) {
    return Spk_CAST(Symbol, op) != (SpkSymbol *)0;
}

const char *SpkHost_SymbolAsCString(SpkUnknown *unk) {
    SpkSymbol *sym = Spk_CAST(Symbol, unk);
    if (sym) {
        return SpkSymbol_AsCString(sym);
    }
    return 0;
}

SpkUnknown *SpkHost_SymbolFromCString(const char *str) {
    return (SpkUnknown *)SpkSymbol_FromCString(str);
}

SpkUnknown *SpkHost_SymbolFromCStringAndLength(const char *str, size_t len) {
    return (SpkUnknown *)SpkSymbol_FromCStringAndLength(str, len);
}


/*------------------------------------------------------------------------*/
/* selectors */

const char *SpkHost_SelectorAsCString(SpkUnknown *selector) {
    SpkSymbol *sym = Spk_CAST(Symbol, selector);
    if (sym) {
        return SpkSymbol_AsCString(sym);
    }
    return 0;
}

SpkUnknown *SpkHost_NewKeywordSelectorBuilder(void) {
    return (SpkUnknown *)SpkArray_New(2);
}

/****/ void SpkHost_AppendKeyword(SpkUnknown **builder, SpkUnknown *kw) {
    SpkArray *kwArray;
    size_t size, i;
    SpkUnknown *vd;
    
    kwArray = Spk_CAST(Array, *builder);
    size = SpkArray_Size(kwArray);
    for (i = 0; i < size; ++i) {
        SpkUnknown *item = SpkArray_GetItem(kwArray, i);
        if (item == Spk_uninit) {
            Spk_DECREF(item);
            break;
        }
        Spk_DECREF(item);
    }
    if (i == size) {
        SpkArray *newArray;
        size_t newSize;
        
        newSize = size * 2;
        newArray = SpkArray_New(newSize);
        for (i = 0; i < size; ++i) {
            SpkUnknown *item = SpkArray_GetItem(kwArray, i);
            vd = SpkArray_SetItem(newArray, i, item);
            Spk_DECREF(vd);
            Spk_DECREF(item);
        }
        Spk_DECREF(kwArray);
        kwArray = newArray;
        *builder = (SpkUnknown *)kwArray;
    }
    vd = SpkArray_SetItem(kwArray, i, kw);
    Spk_DECREF(vd);
}

static int compareSymbols(SpkUnknown *const *p1, SpkUnknown *const *p2) {
    SpkSymbol *sym1, *sym2;
    const char *s1, *s2;
    
    sym1 = Spk_CAST(Symbol, *p1);
    sym2 = Spk_CAST(Symbol, *p2);
    if (!sym1 && !sym2)
        return 0;
    if (!sym1)
        return 1;
    if (!sym2)
        return -1;
    s1 = SpkSymbol_AsCString(sym1);
    s2 = SpkSymbol_AsCString(sym2);
    return strcmp(s1, s2);
}

SpkUnknown *SpkHost_GetKeywordSelector(SpkUnknown *builder, SpkUnknown *kw) {
    SpkArray *kwArray;
    SpkSymbol *sym, *selector;
    size_t size, len, i, tally;
    char *str;
    
    kwArray = Spk_CAST(Array, builder);
    size = SpkArray_Size(kwArray);
    len = 0;
    
    if (kw) {
        sym = Spk_CAST(Symbol, kw);
        len += strlen(SpkSymbol_AsCString(sym));
        if (0) {
            /* XXX: The formal arguments must be permuted in the same
               way. */
            SpkArray_Sort(kwArray, &compareSymbols);
        }
    }
    for (i = 0; i < size; ++i) {
        SpkUnknown *item = SpkArray_GetItem(kwArray, i);
        if (item == Spk_uninit) {
            Spk_DECREF(item);
            break;
        }
        sym = Spk_CAST(Symbol, item);
        len += strlen(SpkSymbol_AsCString(sym)) + 1;
        Spk_DECREF(item);
    }
    
    tally = i;
    if (tally > 0)
        ++len;
    
    str = (char *)malloc(len + 1);
    str[0] = '\0';
    
    if (kw) {
        sym = Spk_CAST(Symbol, kw);
        strcat(str, SpkSymbol_AsCString(sym));
        if (tally > 0)
            strcat(str, " ");
    }
    for (i = 0; i < size; ++i) {
        SpkUnknown *item = SpkArray_GetItem(kwArray, i);
        if (item == Spk_uninit) {
            Spk_DECREF(item);
            break;
        }
        sym = Spk_CAST(Symbol, item);
        strcat(str, SpkSymbol_AsCString(sym));
        strcat(str, ":");
        Spk_DECREF(item);
    }
    str[len] = '\0';
    
    selector = SpkSymbol_FromCString(str);
    free(str);
    return (SpkUnknown *)selector;
}


/*------------------------------------------------------------------------*/
/* symbol dictionaries */

SpkUnknown *SpkHost_NewSymbolDict(void) {
    return (SpkUnknown *)SpkIdentityDictionary_New();
}

/****/ void SpkHost_DefineSymbol(SpkUnknown *symbolDict, SpkUnknown *selector, SpkUnknown *method) {
    SpkIdentityDictionary *dict = Spk_CAST(IdentityDictionary, symbolDict);
    SpkIdentityDictionary_SetItem(dict, selector, method);
}

SpkUnknown *SpkHost_SymbolValue(SpkUnknown *symbolDict, SpkUnknown *key) {
    SpkIdentityDictionary *dict = Spk_CAST(IdentityDictionary, symbolDict);
    return SpkIdentityDictionary_GetItem(dict, key);
}

SpkUnknown *SpkHost_FindSymbol(SpkUnknown *symbolDict, SpkUnknown *method) {
    SpkIdentityDictionary *dict = Spk_CAST(IdentityDictionary, symbolDict);
    return SpkIdentityDictionary_KeyWithValue(dict, method);
}

/*****/ int SpkHost_NextSymbol(SpkUnknown *symbolDict,
                               size_t *pPos,
                               SpkUnknown **pKey,
                               SpkUnknown **pValue)
{
    SpkIdentityDictionary *dict;
    
    dict = Spk_CAST(IdentityDictionary, symbolDict);
    return SpkIdentityDictionary_Next(dict, pPos, pKey, pValue);
}


/*------------------------------------------------------------------------*/
/* literal dictionaries */

SpkUnknown *SpkHost_NewLiteralDict(void) {
    /* XXX: IdentityDictionary doesn't really do the job.  It works
     * for Symbols, but it fails to uniquify strings, integers,
     * floats...
     */
    return (SpkUnknown *)SpkIdentityDictionary_New();
}

unsigned int SpkHost_InsertLiteral(SpkUnknown *literalDict, SpkUnknown *literal) {
    SpkIdentityDictionary *dict;
    SpkUnknown *value;
    unsigned int index;
    
    dict = Spk_CAST(IdentityDictionary, literalDict);
    value = SpkIdentityDictionary_GetItem(dict, literal);
    if (value) {
        index = SpkInteger_AsCLong((SpkInteger *)Spk_CAST(Integer, value));
    } else {
        index = SpkIdentityDictionary_Size(dict);
        value = (SpkUnknown *)SpkInteger_FromCLong(index);
        SpkIdentityDictionary_SetItem(dict, literal, value);
    }
    Spk_DECREF(value);
    return index;
}


/*------------------------------------------------------------------------*/
/* arguments  */

/*****/ int SpkHost_IsArgs(SpkUnknown *op) {
    return Spk_CAST(Array, op) != (SpkArray *)0;
}

/**/ size_t SpkHost_ArgsSize(SpkUnknown *args) {
    return SpkArray_Size(Spk_CAST(Array, args));
}

SpkUnknown *SpkHost_GetArg(SpkUnknown *args, size_t index) {
    return SpkArray_GetItem(Spk_CAST(Array, args), index);
}

SpkUnknown *SpkHost_EmptyArgs(void) {
    return (SpkUnknown *)SpkArray_New(0);
}

SpkUnknown *SpkHost_ArgsFromVAList(va_list ap) {
    return (SpkUnknown *)SpkArray_FromVAList(ap);
}

SpkUnknown *SpkHost_GetArgs(SpkUnknown **stackPointer,
                            size_t argumentCount,
                            SpkUnknown *varArgs,
                            size_t skip)
{
    SpkArray *varArgArray;
    
    varArgArray = varArgs ? Spk_CAST(Array, varArgs) : 0;
    return (SpkUnknown *)SpkArray_WithArguments(stackPointer,
                                                argumentCount,
                                                varArgArray,
                                                skip);
}


/*------------------------------------------------------------------------*/
/* i/o */

/*****/ int SpkHost_IsFileStream(SpkUnknown *op) {
    return Spk_CAST(FileStream, op) != (SpkFileStream *)0;
}

/***/ FILE *SpkHost_FileStreamAsCFileStream(SpkUnknown *op) {
    SpkFileStream *stream = Spk_CAST(FileStream, op);
    return SpkFileStream_AsCFileStream(stream);
}


/*------------------------------------------------------------------------*/
/* as yet unclassified */

SpkUnknown *SpkHost_ObjectAsString(SpkUnknown *obj) {
    return Spk_SendMessage(
        theInterpreter,
        obj,
        Spk_METHOD_NAMESPACE_RVALUE,
        Spk_printString,
        Spk_emptyArgs
        );
}

/****/ void SpkHost_PrintObject(SpkUnknown *obj, FILE *stream) {
    SpkUnknown *printObj;
    SpkString *printString;
    char *str;
    
    printObj = SpkHost_ObjectAsString(obj);
    if (!printObj)
        return;
    printString = Spk_CAST(String, printObj);
    if (printString) {
        str = SpkString_AsCString(printString);
        fputs(str, stream);
    }
    Spk_DECREF(printObj);
}
