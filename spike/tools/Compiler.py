#!/usr/bin/env python


from Tool import Tool


class Compiler(Tool):


    #
    # running
    #

    def main(self, argv):
        from sys import stderr
        from os.path import splitext
        from elftools.elf.elffile import ELFFile
        from spike.compiler import compile


        obj = ".o" # XXX: .obj with VC++

        
        #
        # Process options.
        #

        self.parseArguments(argv)

        if not self.inputFiles:
            print >>stderr, "%s: no input files" % self.prog
            return 1

        options = self.options


        #
        # Classify input files.
        #

        sourceFiles = []
        objectFiles = []
        unknownFiles = []
        for pathname in self.inputFiles:
            ext = splitext(pathname)[1]
            if ext == ".spk":
                sourceFiles.append(pathname)
            elif ext == obj:
                objectFiles.append(pathname)
            else:
                unknownFiles.append(pathname)


        #
        # Scan object files for externals.
        #

        externals = []
        for pathname in objectFiles:
            elffile = ELFFile(open(pathname, "rb"))
            section = elffile.get_section_by_name("spksymtab")
            if section is None:
                continue
            data = section.data()
            externals.extend(
                [item.split(' ') for item in data.split('\0') if item]
                )


        #
        # Determine the output filename.
        #

        outputFile = options.outputFile

        if not outputFile:

            # Since the Spike compiler is usually invoked with multiple
            # source files, the tradition of deriving a default output
            # filename from the input filename doesn't make sense.
            outputFile = "spkout"

            # The extension depends upon which stages will be executed.
            if options.compileOnly:
                outputFile += ".s"
            elif options.link:
                pass # XXX: outputFile += ".exe" or ".dll" on Windows
            else:
                outputFile += obj


        #
        # Compile.
        #

        yipAssemblyFile = None
        if sourceFiles:
            nativeAssemblyFile = outputFile if options.compileOnly else self.temp(".s")
            if options.verbose:
                print >>sys.stderr, ' '.join(
                    ["<compile>", "-o", nativeAssemblyFile] + sourceFiles)
            out = open(nativeAssemblyFile, "w")
            compile(sourceFiles, out, externals = externals)
            out.close()
        elif True:
            nativeAssemblyFile = None
        elif sourceFiles: # r179: unfinished 'yip' stuff
            yipAssemblyFile = outputFile if options.yipOnly else self.temp(".yip")
            if options.verbose:
                print >>stderr, ' '.join(
                    ["<compile>", "-o", yipAssemblyFile] + sourceFiles)
            out = open(yipAssemblyFile, "w")
            compile(sourceFiles, out, externals = externals)
            out.close()
        else:
            yipAssemblyFile = None


        if options.yipOnly:
            return 0


        #
        # Assemble yip code.
        #

        if yipAssemblyFile:
            nativeAssemblyFile = outputFile if not options.compileOnly else self.temp(".s")
            self.yip(yipAssemblyFile, nativeAssemblyFile)
        elif False:
            nativeAssemblyFile = None


        if options.compileOnly:
            return 0


        #
        # Assemble native code.
        #

        if nativeAssemblyFile:
            objectFile = outputFile if not options.link else self.temp(obj)
            self.assemble(nativeAssemblyFile, objectFile)
        else:
            objectFile = None


        if options.link:

            #
            # Link.
            #

            if objectFile:
                objectFiles.append(objectFile)
            exeFile = outputFile
            self.link(objectFiles, unknownFiles, exeFile)

        return 0


    #
    # processing command-line options
    #

    def addOptions(self, parser):
        parser.add_option("-c",
                          action="store_false", dest="link", default=True,
                          help="compile and assemble, but do not link")
        parser.add_option("-g",
                          action="store_true", dest="debug", default=False,
                          help="generate debug information")
        parser.add_option("-l", "--library",
                          action="append", dest="libraries", default=[],
                          help="search for library LIBNAME", metavar="LIBNAME")
        parser.add_option("-L", "--library-path",
                          action="append", dest="libraryPath", default=[],
                          help="add DIRECTORY to library search path", metavar="DIRECTORY")
        parser.add_option("-o",
                          dest="outputFile",
                          help="place the output into FILE", metavar="FILE")
        parser.add_option("-S",
                          action="store_true", dest="compileOnly", default=False,
                          help="compile only; do not assemble or link")
        parser.add_option("-v", "--verbose",
                          action="store_true", dest="verbose", default=False,
                          help="display the programs invoked by the compiler")
        parser.add_option("-y", "--yip",
                          action="store_true", dest="yipOnly", default=False,
                          help="output yip IL; do not translate, assemble, or link")
        return


    #
    # spawning subprocesses
    #

    def yip(self, inputPathname, outputPathname):
        self.spawn("yip", "-o", outputPathname, inputPathname)
        return


    def assemble(self, inputPathname, outputPathname):
        self.spawn("as", "--32", "-o", outputPathname, inputPathname)
        return


    def link(self, objectFiles, unknownFiles, outputPathname):
        # XXX: Spawn linker directly?
        flags = []
        if self.options.debug:
            flags.append("-g")
        self.spawn("gcc", "-D_GNU_SOURCE", "-m32", "-o", outputPathname,
                   *(objectFiles + unknownFiles + flags + ["-lm"]))
        return


