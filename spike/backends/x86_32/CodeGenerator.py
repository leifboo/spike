

from ..CodeGenerator import CodeGenerator as Base

from spike.compiler.naming import *


SIZEOF_OBJ_PTR = 4
SIZEOF_CONTEXT_HEADER = 16*SIZEOF_OBJ_PTR
OFFSETOF_ARGUMENT_COUNT = 3*SIZEOF_OBJ_PTR
OFFSETOF_STACK_BASE = 14*SIZEOF_OBJ_PTR


# registers
eax = "%eax"
ebx = "%ebx"
ecx = "%ecx"
edx = "%edx"
esi = "%esi"
edi = "%edi"
ebp = "%ebp"
esp = "%esp"

# instructions
add   = "addl"
call  = "call"
cmp   = "cmpl"
je    = "je"
jmp   = "jmp"
jne   = "jne"
jnz   = "jnz"
jz    = "jz"
mov   = "movl"
nop   = "nop"
pop   = "popl"
push  = "pushl"
ret   = "ret"

# addressing modes
def ea(disp, base, index=None, scale=None):
    if index is None:
        if disp == 0:
            return "(%s)" % base
        return "%d(%s)" % (disp, base)
    if scale is None:
        return "%d(%s,%s)" % (disp, base, index)
    return "%d(%s,%s,%d)" % (disp, base, index, scale)


externalObject = object()


class CodeGenerator(Base):


    def emitOpcode(self, operation, *operands):
        # XXX: support other assemblers in addition to GAS
        if len(operands) == 2:
            self.out.write("\t%s\t%s, %s\n" % (operation, operands[0], operands[1]))
        elif len(operands) == 1:
            self.out.write("\t%s\t%s\n" % (operation, operands[0]))
        elif len(operands) == 0:
            self.out.write("\t%s\n" % operation)
        else:
            assert False
        return


    def emitLabel(self, opcode):
        if opcode.label:
            self.out.write(opcode.label + ':')
        return


    def translate_xxx(self, opcode):
        return


    #
    # no operation opcode
    #

    def translate_nop(self, opcode):
        self.emitOpcode(nop)
        return


    #
    # load/store/stack opcodes
    #

    pushOperands = {
        '.null':         "$0",
        '.self':         esi,
        '.super':        esi,
        '.thisContext':  ebp,
        '.false':        externalObject,
        '.true':         externalObject,
        '.void':         externalObject,
        }

    def translate_push(self, opcode):
        operand = opcode.operands[0]
        self.emitOpcode(push, self.translateOperand(operand))
        return


    def translate_store(self, opcode):
        operand = opcode.operands[0]
        xo = self.translateMemoryOperand(operand)
        self.emitOpcode(mov, ea(0, esp), eax)
        return


    def translate_pop(self, opcode):
        self.pop()
        return


    def translate_dup(self, opcode):
        n = int(opcode.operands[0])
        for i in range(n):
            self.emitOpcode(push, ea(SIZEOF_OBJ_PTR * (n - 1), esp))
        return


    def translate_rot(self, opcode):
        n = int(opcode.operands[0])
        self.emitOpcode(mov, "$%u" % n, ecx)
        self.emitOpcode(call, "SpikeRotate")
        return


    def pop(self):
        self.emitOpcode(pop, eax)
        return


    def translateOperand(self, operand):
        if operand[0] == '.':
            xo = self.pushOperands[operand]
            if xo is externalObject:
                xo = '$' + self.mangleExternal(operand[1:])
        else:
            xo = self.translateMemoryOperand(operand)
        return xo


    def translateMemoryOperand(self, operand):
        if operand[0] == '&':
            xo = '$' + self.mangleExternal(operand[1:])
        else:
            xo = self.mangleExternal(operand)
        return xo


    #
    # control opcodes
    #

    def translate_brf(self, opcode):
        target = opcode.operands[0]
        self.emitOpcode(call, "SpikeTest")
        self.emitOpcode(jz, target)
        return


    def translate_brt(self, opcode):
        target = opcode.operands[0]
        self.emitOpcode(call, "SpikeTest")
        self.emitOpcode(jnz, target)
        return


    def translate_bra(self, opcode):
        target = opcode.operands[0]
        self.emitOpcode(jmp, target)
        return


    def translate_ret(self, opcode):
        # return to SpikeEpilogue
        self.emitOpcode(ret)
        return


    def translate_home(self, opcode):
        # reset stack pointer
        self.emitOpcode(mov, ea(OFFSETOF_STACK_BASE, ebx), esp)
        # activeContext = homeContext
        self.emitOpcode(mov, ebx, ebp)
        # return to SpikeEpilogue
        self.emitOpcode(ret)
        return


    def translate_yield(self, opcode):
        # save our %eip in BlockContext.pc and resume caller
        self.emitOpcode(call, "SpikeYield")
        return


    #
    # identity comparison opcode
    #

    def translate_id(self, opcode):
        endLabel = opcode.operands[0]
        inverted = bool(int(opcode.operands[1]))
        self.pop()
        self.emitOpcode(cmp, ea(0, esp), eax)
        self.emitOpcode(mov, self.translateOperand('false'), ea(0, esp))
        self.emitOpcode(je if inverted else jne, endLabel)
        self.emitOpcode(mov, self.translateOperand('true'), ea(0, esp))
        return


    #
    # send opcode
    #

    def translate_send(self, opcode):
        operands = opcode.operands

        selector = operands[0]

        fixedArgumentCount = None
        if len(operands) >= 2 and operands[1]:
            fixedArgumentCount = int(operands[1])

        varArg = False
        isSuper = False
        lValue = False
        for flag in operands[2:]:
            if flag == 'va':
                varArg = True
            elif flag == 'super':
                isSuper = True
            elif flag == 'lv':
                lValue = True
            else:
                assert False, "invalid 'send' flag: %r" % flag

        # determine the appropriate entry point into the send message
        # bottleneck
        if fixedArgumentCount in (0, 1) and (not selector or selector[0] != '.'):
            if selector:
                assert selector[0] == '#'
                self.emitOpcode(mov, '$' + self.mangleSelector(selector[1:]), edx)
            else:
                self.emitOpcode(pop, edx)
            routine = "SpikeSendMessage%d" % fixedArgumentCount
            fixedArgumentCount = None # implicit
        elif selector:
            if selector[0] == '#':
                self.emitOpcode(mov, '$' + self.mangleSelector(selector[1:]), edx)
                routine = "SpikeSendMessage"
            elif selector[0] == '.':
                selector = selector[1:]
                # XXX: There should be only a single path here.  Make
                # the names consistent: SpikeOperApply,
                # SpikeOperInd(ex)(LValue).
                if selector == "Apply":
                    routine = "SpikeCall"
                    assert fixedArgumentCount is not None and not lValue
                elif selector in ("Ind", "Index"):
                    routine = "Spike" + ("Set" if lValue else "Get") + selector
                    lValue = False
                    if selector == "Index":
                        assert fixedArgumentCount is not None
                else:
                    routine = "SpikeOper%s" % selector
                    assert fixedArgumentCount is None and not varArg and not lValue
            else:
                assert False, "invalid selector: %r" % selector
        else:
            self.emitOpcode(pop, edx)
            routine = "SpikeSendMessage"

        if fixedArgumentCount is not None:
            if varArg:
                # push var args
                self.emitOpcode(call, "SpikePushVarArgs")
                self.emitOpcode(add, "$%u" % fixedArgumentCount, ecx)
            else:
                self.emitOpcode(mov, "$%u" % fixedArgumentCount, ecx)

        # call RTL routine to send message
        if isSuper:
            routine += "Super"
        if lValue:
            routine += "LValue"
        self.emitOpcode(call, routine)

        return


    #
    # utilities
    #

    #
    # name mangling
    #

    def mangleExternal(self, sym):
        return externalPrefix + sym


    def mangleSelector(self, sym):
        return symPrefix + sym

