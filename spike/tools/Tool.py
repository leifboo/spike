#!/usr/bin/env python


import sys


class Tool(object):


    from optparse import OptionParser


    #
    # initializing
    #

    def __init__(self):
        self.tempFiles = []
        return


    #
    # running
    #

    def main(self, argv):
        raise NotImplementedError()


    def start(self, argv):
        from os.path import basename

        self.prog = basename(argv[0])

        status = 1

        try:
            status = self.main(argv)
        except self.SpawnException, e:
            print >>sys.stderr, "%s: %s" % (self.prog, e)
        finally:
            self.removeTempFiles()

        return status


    #
    # processing command-line options
    #

    def createOptionParser(self):
        parser = self.OptionParser(usage="%prog [options] FILE...")
        self.addOptions(parser)
        return parser


    def addOptions(self, parser):
        return


    def parseArguments(self, argv):
        parser = self.createOptionParser()
        self.options, self.inputFiles = parser.parse_args(argv[1:])
        return



    #
    # handling temporary files
    #

    def temp(self, ext):
        from tempfile import mktemp
        pathname = mktemp() + ext
        self.tempFiles.append(pathname)
        return pathname


    def removeTempFiles(self):
        for pathname in self.tempFiles:
            try:
                self.remove(pathname)
            except OSError, e:
                print >>sys.stderr, "rm: '%s': error %s" % (pathname, e)
        return


    def remove(self, pathname):
        import os
        if self.options.verbose:
            print >>sys.stderr, "rm %s" % pathname
        os.remove(pathname)
        return


    #
    # spawning subprocesses
    #

    class SpawnException(Exception): pass


    def spawn(self, *argv):
        import os
        from sys import stderr
        
        if self.options.verbose:
            print >>stderr, ' '.join(argv)
        status = os.spawnvp(os.P_WAIT, argv[0], argv)
        if status < 0:
            raise self.SpawnException("%s: signal %d" % (argv[0], -status))
        elif status != 0:
            raise self.SpawnException("%s: exit %d" % (argv[0], status))
        return


