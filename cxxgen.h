
#ifndef __cxxgen_h__
#define __cxxgen_h__


#include "obj.h"

#include <stddef.h>
#include <stdio.h>


struct Stmt;


Unknown *CxxCodeGen_GenerateCode(struct Stmt *tree, FILE *out);


#endif /* __cxxgen_h__ */
