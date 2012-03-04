
#ifndef __host_h__
#define __host_h__


#include "om.h"
#include <stdarg.h>
#include <stdio.h>


/****/ void Host_Init(void);


/* halting */
/****/ void Host_Halt(int, const char *);
/****/ void Host_HaltWithString(int, Unknown *);
/****/ void Host_VHaltWithFormat(int, const char *, va_list);


/* base types */
/*****/ int Host_IsInteger(Unknown *);
/*****/ int Host_IsString(Unknown *);
/****/ long Host_IntegerAsCLong(Unknown *);
Unknown *Host_IntegerFromCLong(long);
/**/ double Host_FloatAsCDouble(Unknown *);
Unknown *Host_FloatFromCDouble(double);
/****/ char Host_CharAsCChar(Unknown *);
Unknown *Host_CharFromCChar(char);
/***/ char *Host_StringAsCString(Unknown *);
Unknown *Host_StringFromCString(const char *);
Unknown *Host_StringFromCStringAndLength(const char *, size_t);
Unknown *Host_StringConcat(Unknown **, Unknown *);


/* symbols */
/*****/ int Host_IsSymbol(Unknown *);
const char *Host_SymbolAsCString(Unknown *);
Unknown *Host_SymbolFromCString(const char *);
Unknown *Host_SymbolFromCStringAndLength(const char *, size_t);


/* selectors */
const char *Host_SelectorAsCString(Unknown *);
Unknown *Host_NewKeywordSelectorBuilder(void);
/****/ void Host_AppendKeyword(Unknown **, Unknown *);
Unknown *Host_GetKeywordSelector(Unknown *, Unknown *);


/* symbol dictionaries */
Unknown *Host_NewSymbolDict(void);
/****/ void Host_DefineSymbol(Unknown *, Unknown *, Unknown *);
Unknown *Host_SymbolValue(Unknown *, Unknown *);
Unknown *Host_FindSymbol(Unknown *, Unknown *);
/*****/ int Host_NextSymbol(Unknown *, size_t *,
                               Unknown **, Unknown **);


/* arguments */
/*****/ int Host_IsArgs(Unknown *);
/**/ size_t Host_ArgsSize(Unknown *);
Unknown *Host_GetArg(Unknown *, size_t);
Unknown *Host_EmptyArgs(void);
Unknown *Host_ArgsFromVAList(va_list);
Unknown *Host_GetArgs(Unknown **, size_t,
                            Unknown *, size_t);

/* i/o */
/*****/ int Host_IsFileStream(Unknown *);
/***/ FILE *Host_FileStreamAsCFileStream(Unknown *);


/* as yet unclassified */
Unknown *Host_ObjectAsString(Unknown *);
/****/ void Host_PrintObject(Unknown *, FILE *);


#endif /* __host_h__ */
