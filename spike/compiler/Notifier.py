

import sys


class Notifier(object):


    def __init__(self, stream = sys.stderr):
        self.stream = stream
        self.errorTally = 0
        return


    def badExpr(self, expr, desc):
        print "    badExpr", expr, desc
        self.errorTally += 1
        return


    def redefinedSymbol(self, expr):
        print "    redefinedSymbol", expr.sym
        self.errorTally += 1
        return


    def undefinedSymbol(self, expr):
        print "    undefinedSymbol", expr.sym
        self.errorTally += 1
        return


    def failOnError(self):
        assert self.errorTally == 0
        return


