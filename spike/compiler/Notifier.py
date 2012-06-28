

import sys


_ = lambda text: text


class Notifier(object):


    def __init__(self, stream = sys.stderr):
        self.stream = stream
        self.errorTally = 0
        self.source = "<unknown>"
        return


    def notify(self, expr, desc):
        print >>self.stream, "%s:%u: %s" % (self.source, expr.lineNum, desc)
        self.errorTally += 1
        return


    def redefinedSymbol(self, expr):
        self.notify(expr, _("symbol '%s' multiply defined") % expr.sym)
        return


    def undefinedSymbol(self, expr):
        self.notify(expr, _("symbol '%s' undefined") % expr.sym)
        return


    def invalidVariableDefinition(self, expr):
        self.notify(expr, _("invalid variable definition"))
        return


    def invalidArgumentDefinition(self, expr):
        self.notify(expr, _("invalid argument definition"))
        return


    def invalidMethodDeclarator(self, expr):
        self.notify(expr, _("invalid method declarator"))
        return


    def invalidSpecifier(self, expr):
        self.notify(expr, _("invalid specifier"))
        return


    def invalidSuperclassSpecification(self, expr):
        self.notify(expr, _("invalid superclass specification"))
        return


    def nonDefaultArgumentFollowsDefaultArgument(self, expr):
        self.notify(expr, _("non-default argument follows default argument"))
        return


    def cycleInSuperclassChain(self, expr):
        self.notify(expr, _("cycle in superclass chain"))
        return


    def externMethodWithANonEmptyBody(self, expr):
        self.notify(expr, _("'extern' method with a non-empty body"))
        return


    def failOnError(self):
        if self.errorTally != 0:
            import sys
            sys.exit(1)
        return


