
#ifndef __disasm_h__
#define __disasm_h__


#include <stdio.h>

struct Behavior;
struct Method;
struct Module;


void SpkDisassembler_disassembleMethod(struct Method *, FILE *);
void SpkDisassembler_disassembleClass(struct Behavior *, FILE *);
void SpkDisassembler_disassembleModule(struct Module *, FILE *);


#endif /* __disasm_h__ */
