
#ifndef __x86gen_h__
#define __x86gen_h__


#include "obj.h"

#include <stddef.h>
#include <stdio.h>


struct Stmt;


Unknown *X86CodeGen_GenerateCode(struct Stmt *tree, FILE *out);


#endif /* __x86gen_h__ */
