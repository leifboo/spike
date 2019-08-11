

class CodeGenerator(object):


    def __init__(self, out):
        self.out = out
        return


    def translate(self, opcodes):
        for opcode in opcodes:
            self.emitLabel(opcode)
            if opcode.operation:
                method = getattr(self, 'translate_' + opcode.operation)
                method(opcode)
            else:
                self.out.write("\n")
        return


    def emitLabel(self, opcode):
        raise NotImplementedError()


