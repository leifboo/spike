
#ifndef __spk_host_h__
#define __spk_host_h__


#include "om.h"
#include <stdarg.h>
#include <stdio.h>


/****/ void SpkHost_Init(void);


/* halting */
/****/ void SpkHost_Halt(int, const char *);
/****/ void SpkHost_HaltWithString(int, SpkUnknown *);
/****/ void SpkHost_VHaltWithFormat(int, const char *, va_list);


/* base types */
/*****/ int SpkHost_IsInteger(SpkUnknown *);
/*****/ int SpkHost_IsString(SpkUnknown *);
/****/ long SpkHost_IntegerAsCLong(SpkUnknown *);
SpkUnknown *SpkHost_IntegerFromCLong(long);
/**/ double SpkHost_FloatAsCDouble(SpkUnknown *);
SpkUnknown *SpkHost_FloatFromCDouble(double);
/****/ char SpkHost_CharAsCChar(SpkUnknown *);
SpkUnknown *SpkHost_CharFromCChar(char);
/***/ char *SpkHost_StringAsCString(SpkUnknown *);
SpkUnknown *SpkHost_StringFromCString(const char *);
SpkUnknown *SpkHost_StringFromCStringAndLength(const char *, size_t);
SpkUnknown *SpkHost_StringConcat(SpkUnknown **, SpkUnknown *);


/* symbols */
/*****/ int SpkHost_IsSymbol(SpkUnknown *);
const char *SpkHost_SymbolAsCString(SpkUnknown *);
SpkUnknown *SpkHost_SymbolFromCString(const char *);
SpkUnknown *SpkHost_SymbolFromCStringAndLength(const char *, size_t);


/* selectors */
const char *SpkHost_SelectorAsCString(SpkUnknown *);
SpkUnknown *SpkHost_NewKeywordSelectorBuilder(void);
/****/ void SpkHost_AppendKeyword(SpkUnknown **, SpkUnknown *);
SpkUnknown *SpkHost_GetKeywordSelector(SpkUnknown *, SpkUnknown *);


/* symbol dictionaries */
SpkUnknown *SpkHost_NewSymbolDict(void);
/****/ void SpkHost_DefineSymbol(SpkUnknown *, SpkUnknown *, SpkUnknown *);
SpkUnknown *SpkHost_SymbolValue(SpkUnknown *, SpkUnknown *);
SpkUnknown *SpkHost_FindSymbol(SpkUnknown *, SpkUnknown *);
/*****/ int SpkHost_NextSymbol(SpkUnknown *, size_t *,
                               SpkUnknown **, SpkUnknown **);


/* literal dictionaries */
SpkUnknown *SpkHost_NewLiteralDict(void);
unsigned int SpkHost_InsertLiteral(SpkUnknown *, SpkUnknown *);


/* arguments */
/*****/ int SpkHost_IsArgs(SpkUnknown *);
/**/ size_t SpkHost_ArgsSize(SpkUnknown *);
SpkUnknown *SpkHost_GetArg(SpkUnknown *, size_t);
SpkUnknown *SpkHost_EmptyArgs(void);
SpkUnknown *SpkHost_ArgsFromVAList(va_list);
SpkUnknown *SpkHost_GetArgs(SpkUnknown **, size_t,
                            SpkUnknown *, size_t);

/* i/o */
/*****/ int SpkHost_IsFileStream(SpkUnknown *);
/***/ FILE *SpkHost_FileStreamAsCFileStream(SpkUnknown *);


/* as yet unclassified */
SpkUnknown *SpkHost_ObjectAsString(SpkUnknown *);
/****/ void SpkHost_PrintObject(SpkUnknown *, FILE *);


#endif /* __spk_host_h__ */
