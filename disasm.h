
#ifndef __disasm_h__
#define __disasm_h__


#include <stdio.h>


struct ModuleTmpl;


void Disassembler_DisassembleModule(struct ModuleTmpl *, FILE *);
void Disassembler_DisassembleModuleAsCCode(struct ModuleTmpl *, FILE *);


#endif /* __disasm_h__ */
