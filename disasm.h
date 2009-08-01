
#ifndef __spk_disasm_h__
#define __spk_disasm_h__


#include "obj.h"
#include <stdio.h>


struct SpkBehavior;
struct SpkMethod;
struct SpkModule;


void SpkDisassembler_DisassembleMethodOpcodes(struct SpkMethod *, SpkUnknown **, unsigned int, FILE *);
void SpkDisassembler_DisassembleMethod(struct SpkMethod *, SpkUnknown **, unsigned int, FILE *);
void SpkDisassembler_DisassembleClass(struct SpkBehavior *, unsigned int, FILE *);
void SpkDisassembler_DisassembleModule(struct SpkModule *, FILE *);


#endif /* __spk_disasm_h__ */
