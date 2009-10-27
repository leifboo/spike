
#ifndef __spk_cxxgen_h__
#define __spk_cxxgen_h__


#include "obj.h"

#include <stddef.h>
#include <stdio.h>


struct SpkStmt;


SpkUnknown *SpkCxxCodeGen_GenerateCode(struct SpkStmt *tree, FILE *out);


#endif /* __spk_cxxgen_h__ */
