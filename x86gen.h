
#ifndef __spk_x86gen_h__
#define __spk_x86gen_h__


#include "obj.h"

#include <stddef.h>
#include <stdio.h>


struct SpkStmt;


SpkUnknown *SpkX86CodeGen_GenerateCode(struct SpkStmt *tree, FILE *out);


#endif /* __spk_x86gen_h__ */
