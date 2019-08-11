#!/usr/bin/env python


from Tool import Tool


class Assembler(Tool):


    #
    # running
    #

    def main(self, argv):
        from sys import stderr
        from spike.assembler import Assembler

        self.parseArguments(argv)

        if not self.inputFiles:
            print >>stderr, "%s: no input files" % self.prog
            return 1

        outputFile = self.options.outputFile or "spkout.s"

        assembler = Assembler()
        out = open(outputFile, "w")
        assembler.assemble(self.inputFiles, out)
        out.close()

        return 0


    #
    # processing command-line options
    #

    def addOptions(self, parser):
        parser.add_option("-o",
                          dest="outputFile",
                          help="place the output into FILE", metavar="FILE")
        return


