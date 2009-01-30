
#ifndef __disasm_h__
#define __disasm_h__


#include <stdio.h>

struct Behavior;
struct Method;
struct Module;


void SpkDisassembler_disassembleMethodOpcodes(struct Method *, unsigned int, FILE *);
void SpkDisassembler_disassembleMethod(struct Method *, unsigned int, FILE *);
void SpkDisassembler_disassembleClass(struct Behavior *, unsigned int, FILE *);
void SpkDisassembler_disassembleModule(struct Module *, FILE *);


#endif /* __disasm_h__ */
