

from Stmt import Stmt


class ClassDef(Stmt):


    def __init__(self, name, superclass, body):
        super(ClassDef, self).__init__()
        self.name = name
        self.superclass = superclass
        self.body = body
        return


    def _iterChildren(self):
        yield ('body', self.body)
        return
    children = property(_iterChildren)


from Stmt import Stmt


class Compound(Stmt, list):


    def __init__(self, stmtList):
        super(Compound, self).__init__(stmtList)
        return


    def _iterChildren(self):
        for index, stmt in enumerate(self):
            yield (index, stmt)
        return
    children = property(_iterChildren)


from Stmt import Stmt


class Expr(Stmt):


    def __init__(self, expr):
        super(Expr, self).__init__()
        self.expr = expr
        return


    def _iterChildren(self):
        yield ('expr', self.expr)
        return
    children = property(_iterChildren)


from Stmt import Stmt


class MethodDef(Stmt):


    def __init__(self, expr, stmt):
        super(MethodDef, self).__init__()
        self.expr = expr
        self.stmt = stmt
        return


    def _iterChildren(self):
        yield ('expr', self.expr)
        yield ('stmt', self.stmt)
        return
    children = property(_iterChildren)


from spike.compiler.tree.Node import Node


class Stmt(Node):


    def __repr__(self):
        return "<stmt %s>" % self.__class__.__name__
