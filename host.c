
#include "host.h"

#include "array.h"
#include "behavior.h"
#include "dict.h"
#include "float.h"
#include "int.h"
#include "interp.h"
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
    SpkInterpreter_PrintCallStack(theInterpreter);
    fprintf(stderr, "halt: %s: %s\n", haltDesc(code), message);
}

/****/ void SpkHost_HaltWithString(int code, SpkUnknown *message) {
    SpkString *str = Spk_CAST(String, message);
    SpkHost_Halt(code, SpkString_asString(str));
}

/****/ void SpkHost_VHaltWithFormat(int code, const char *format, va_list args) {
    fprintf(stderr, "halt: %s: ", haltDesc(code));
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
}


/*------------------------------------------------------------------------*/
/* base types */

/*****/ int SpkHost_IsInteger(SpkUnknown *op) {
    return Spk_CAST(Integer, op) != (SpkInteger *)0;
}

/****/ long SpkHost_IntegerAsCLong(SpkUnknown *op) {
    return SpkInteger_asLong(Spk_CAST(Integer, op));
}

SpkUnknown *SpkHost_IntegerFromCLong(long value) {
    return (SpkUnknown *)SpkInteger_fromLong(value);
}

SpkUnknown *SpkHost_FloatFromCDouble(double value) {
    return (SpkUnknown *)SpkFloat_fromCDouble(value);
}

/***/ char *SpkHost_StringAsCString(SpkUnknown *op) {
    SpkString *str = Spk_CAST(String, op);
    return SpkString_asString(str);
}

SpkUnknown *SpkHost_StringFromCStringAndLength(const char *str, size_t len) {
    return (SpkUnknown *)SpkString_fromCStringAndLength(str, len);
}

SpkUnknown *SpkHost_StringConcat(SpkUnknown **a, SpkUnknown *b) {
    return (SpkUnknown *)SpkString_concat((SpkString **)a, Spk_CAST(String, b));
}


/*------------------------------------------------------------------------*/
/* symbols */

/*****/ int SpkHost_IsSymbol(SpkUnknown *op) {
    return Spk_CAST(Symbol, op) != (SpkSymbol *)0;
}

/***/ char *SpkHost_SymbolAsCString(SpkUnknown *unk) {
    SpkSymbol *sym = Spk_CAST(Symbol, unk);
    if (sym) {
        return sym->str;
    }
    return 0;
}

SpkUnknown *SpkHost_SymbolFromString(const char *str) {
    return (SpkUnknown *)SpkSymbol_get(str);
}

SpkUnknown *SpkHost_SymbolFromCStringAndLength(const char *str, size_t len) {
    return (SpkUnknown *)SpkSymbol_fromCStringAndLength(str, len);
}


/*------------------------------------------------------------------------*/
/* selectors */

/***/ char *SpkHost_SelectorAsCString(SpkUnknown *selector) {
    SpkSymbol *sym = Spk_CAST(Symbol, selector);
    if (sym) {
        return sym->str;
    }
    return 0;
}

SpkUnknown *SpkHost_NewKeywordSelectorBuilder(void) {
    return (SpkUnknown *)SpkArray_new(2);
}

/****/ void SpkHost_AppendKeyword(SpkUnknown **builder, SpkUnknown *kw) {
    SpkArray *kwArray;
    size_t size, i;
    SpkUnknown *vd;
    
    kwArray = Spk_CAST(Array, *builder);
    size = SpkArray_size(kwArray);
    for (i = 0; i < size; ++i) {
        SpkUnknown *item = SpkArray_item(kwArray, i);
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
        newArray = SpkArray_new(newSize);
        for (i = 0; i < size; ++i) {
            SpkUnknown *item = SpkArray_item(kwArray, i);
            vd = SpkArray_setItem(newArray, i, item);
            Spk_DECREF(vd);
            Spk_DECREF(item);
        }
        Spk_DECREF(kwArray);
        kwArray = newArray;
        *builder = (SpkUnknown *)kwArray;
    }
    vd = SpkArray_setItem(kwArray, i, kw);
    Spk_DECREF(vd);
}

SpkUnknown *SpkHost_GetKeywordSelector(SpkUnknown *builder, SpkUnknown *kw) {
    SpkArray *kwArray;
    SpkSymbol *sym, *selector;
    size_t size, len, i;
    char *str;
    
    kwArray = Spk_CAST(Array, builder);
    size = SpkArray_size(kwArray);
    len = 0;
    
    if (kw) {
        sym = Spk_CAST(Symbol, kw);
        len += strlen(sym->str) + 1;
    }
    for (i = 0; i < size; ++i) {
        SpkUnknown *item = SpkArray_item(kwArray, i);
        if (item == Spk_uninit) {
            Spk_DECREF(item);
            break;
        }
        sym = Spk_CAST(Symbol, item);
        len += strlen(sym->str) + 1;
        Spk_DECREF(item);
    }
    
    str = (char *)malloc(len + 1);
    str[0] = '\0';
    
    if (kw) {
        sym = Spk_CAST(Symbol, kw);
        strcat(str, sym->str);
        strcat(str, " ");
    }
    for (i = 0; i < size; ++i) {
        SpkUnknown *item = SpkArray_item(kwArray, i);
        if (item == Spk_uninit) {
            Spk_DECREF(item);
            break;
        }
        sym = Spk_CAST(Symbol, item);
        strcat(str, sym->str);
        strcat(str, ":");
        Spk_DECREF(item);
    }
    str[len] = '\0';
    
    selector = SpkSymbol_get(str);
    free(str);
    return (SpkUnknown *)selector;
}


/*------------------------------------------------------------------------*/
/* symbol dictionaries */

SpkUnknown *SpkHost_NewSymbolDict(void) {
    return (SpkUnknown *)SpkIdentityDictionary_new();
}

/****/ void SpkHost_DefineSymbol(SpkUnknown *symbolDict, SpkUnknown *selector, SpkUnknown *method) {
    SpkIdentityDictionary *dict = Spk_CAST(IdentityDictionary, symbolDict);
    SpkIdentityDictionary_atPut(dict, selector, method);
}

SpkUnknown *SpkHost_SymbolValue(SpkUnknown *symbolDict, SpkUnknown *key) {
    SpkIdentityDictionary *dict = Spk_CAST(IdentityDictionary, symbolDict);
    return SpkIdentityDictionary_at(dict, key);
}

SpkUnknown *SpkHost_FindSymbol(SpkUnknown *symbolDict, SpkUnknown *method) {
    SpkIdentityDictionary *dict = Spk_CAST(IdentityDictionary, symbolDict);
    return SpkIdentityDictionary_keyAtValue(dict, method);
}

/*****/ int SpkHost_NextSymbol(SpkUnknown *symbolDict,
                               size_t *pPos,
                               SpkUnknown **pKey,
                               SpkUnknown **pValue)
{
    SpkIdentityDictionary *dict;
    size_t pos;
    
    dict = Spk_CAST(IdentityDictionary, symbolDict);
    pos = *pPos;
    for ( ; pos < dict->size; ++pos) {
        SpkUnknown *key = dict->keyArray[pos];
        if (key) {
            *pPos = pos + 1;
            *pKey = key;
            *pValue = dict->valueArray[pos];
            return 1;
        }
    }
    *pPos = pos;
    *pKey = 0;
    *pValue = 0;
    return 0;
}


/*------------------------------------------------------------------------*/
/* literal dictionaries */

SpkUnknown *SpkHost_NewLiteralDict(void) {
    /* XXX: IdentityDictionary doesn't really do the job.  It works
     * for Symbols, but it fails to uniquify strings, integers,
     * floats...
     */
    return (SpkUnknown *)SpkIdentityDictionary_new();
}

unsigned int SpkHost_InsertLiteral(SpkUnknown *literalDict, SpkUnknown *literal) {
    SpkIdentityDictionary *dict;
    SpkUnknown *value;
    unsigned int index;
    
    dict = Spk_CAST(IdentityDictionary, literalDict);
    value = SpkIdentityDictionary_at(dict, literal);
    if (value) {
        return SpkInteger_asLong((SpkInteger *)Spk_CAST(Integer, value));
    }
    
    index = dict->tally;
    value = (SpkUnknown *)SpkInteger_fromLong(index);
    SpkIdentityDictionary_atPut(dict, literal, value);
    Spk_DECREF(value);
    return index;
}


/*------------------------------------------------------------------------*/
/* arguments  */

/*****/ int SpkHost_IsArgs(SpkUnknown *op) {
    return Spk_CAST(Array, op) != (SpkArray *)0;
}

/**/ size_t SpkHost_ArgsSize(SpkUnknown *args) {
    return SpkArray_size(Spk_CAST(Array, args));
}

SpkUnknown *SpkHost_GetArg(SpkUnknown *args, size_t index) {
    return SpkArray_item(Spk_CAST(Array, args), index);
}

SpkUnknown *SpkHost_EmptyArgs(void) {
    return (SpkUnknown *)SpkArray_new(0);
}

SpkUnknown *SpkHost_ArgsFromVAList(va_list ap) {
    return (SpkUnknown *)SpkArray_fromVAList(ap);
}

SpkUnknown *SpkHost_GetArgs(SpkUnknown **stackPointer,
                            size_t argumentCount,
                            SpkUnknown *varArgs,
                            size_t skip)
{
    SpkArray *varArgArray;
    
    varArgArray = varArgs ? Spk_CAST(Array, varArgs) : 0;
    return (SpkUnknown *)SpkArray_withArguments(stackPointer,
                                                argumentCount,
                                                varArgArray,
                                                skip);
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
        str = SpkString_asString(printString);
        fputs(str, stream);
    }
    Spk_DECREF(printObj);
}
