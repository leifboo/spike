

from collections import namedtuple



Opcode = namedtuple('Opcode', ('label', 'operation', 'operands'))



class Assembler(object):


    def assemble(self, inputFiles, out):
        from spike.backends.x86_32 import CodeGenerator

        opcodes = []

        for inputFile in inputFiles:
            stream = open(inputFile)
            self.parse(stream, opcodes)
            stream.close()

        cgen = CodeGenerator(out)
        cgen.translate(opcodes)

        return


    def parse(self, stream, opcodes):
        import re
        
        regexp = re.compile("[\t]*")
        
        for line in stream:

            # strip comments
            semicolon = line.find(';')
            if semicolon != -1:
                line = line[:semicolon]

            # tokenize the line
            if not line:
                continue
            if line[-1] == '\n':
                line = line[:-1]
                if not line:
                    continue
            line = regexp.split(line)
            while line and line[-1] == '':
                line.pop()
            if not line:
                continue
            assert len(line) <= 3, "invalid line: %r" % line
            line += [None] * (3 - len(line))

            # normalize label, if any
            if line[0]:
                assert line[0][-1] == ':', "invalid label '%s'" % line[0]
                line[0] = line[0][:-1]
            else:
                line[0] = None

            # split operands, if any
            if line[2]:
                line[2] = line[2].split(',')

            line = Opcode(*line)
            opcodes.append(line)

        return


