
#ifndef __spk_disasm_h__
#define __spk_disasm_h__


#include <stdio.h>


struct SpkModuleTmpl;


void SpkDisassembler_DisassembleModule(struct SpkModuleTmpl *, FILE *);
void SpkDisassembler_DisassembleModuleAsCCode(struct SpkModuleTmpl *, FILE *);


#endif /* __spk_disasm_h__ */
