
#ifndef __spk_disasm_h__
#define __spk_disasm_h__


#include "obj.h"
#include <stdio.h>


struct SpkBehavior;
struct SpkMethod;
struct SpkModule;


void SpkDisassembler_DisassembleModule(struct SpkModule *, FILE *);
void SpkDisassembler_DisassembleModuleAsCCode(struct SpkModule *, FILE *);


#endif /* __spk_disasm_h__ */
