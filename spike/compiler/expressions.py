

from Expr import Expr


class And(Expr):


    def __init__(self, left, right, token):
        super(And, self).__init__()
        self.left = left
        self.right = right
        self.token = token
        return


    def _iterChildren(self):
        yield ('left', self.left)
        yield ('right', self.right)
        return
    children = property(_iterChildren)


from Expr import Expr


class Assign(Expr):


    def __init__(self, op, left, right, token):
        super(Assign, self).__init__()
        self.op = op
        self.left = left
        self.right = right
        self.token = token
        return


    def _iterChildren(self):
        yield ('op', self.op)
        yield ('left', self.left)
        yield ('right', self.right)
        return
    children = property(_iterChildren)


from Expr import Expr


class Attr(Expr):


    def __init__(self, obj, attr, token):
        super(Attr, self).__init__()
        self.obj = obj
        self.attr = attr
        self.token = token
        return


    def _iterChildren(self):
        yield ('obj', self.obj)
        yield ('attr', self.attr)
        return
    children = property(_iterChildren)


from Expr import Expr


class AttrVar(Expr):


    def __init__(self, obj, attr, token):
        super(AttrVar, self).__init__()
        self.obj = obj
        self.attr = attr
        self.token = token
        return


    def _iterChildren(self):
        yield ('obj', self.obj)
        yield ('attr', self.attr)
        return
    children = property(_iterChildren)


from Expr import Expr


class BinaryOp(Expr):


    def __init__(self, op, left, right, token):
        super(BinaryOp, self).__init__()
        self.op = op
        self.left = left
        self.right = right
        self.token = token
        return


    def _iterChildren(self):
        yield ('op', self.op)
        yield ('left', self.left)
        yield ('right', self.right)
        return
    children = property(_iterChildren)


from Expr import Expr


class Block(Expr):


    def __init__(self, stmtList, expr, token):
        super(Block, self).__init__()
        self.stmtList = stmtList
        self.expr = expr
        self.token = token
        return


    def _iterChildren(self):
        for index, stmt in enumerate(self.stmtList):
            yield (index, self.stmtList)
        yield ('expr', self.expr)
        return
    children = property(_iterChildren)


from Expr import Expr


class Call(Expr):


    def __init__(self, oper, func, args, token):
        super(Call, self).__init__()
        self.oper = oper
        self.func = func
        self.args = args
        self.token = token
        return


    def _iterChildren(self):
        yield ('oper', self.oper)
        yield ('func', self.func)
        yield ('args', self.args)
        return
    children = property(_iterChildren)


from Expr import Expr


class Comma(Expr, list):


    def _iterChildren(self):
        for index, expr in enumerate(self):
            yield (index, expr)
        return
    children = property(_iterChildren)


    def comma(self, right):
        self.append(right)
        return self


from Expr import Expr


class Compound(Expr):


    def __init__(self, expr, token):
        super(Compound, self).__init__()
        self.expr = expr
        self.token = token
        return


    def _iterChildren(self):
        yield ('expr', self.expr)
        return
    children = property(_iterChildren)


from Expr import Expr


class Cond(Expr):


    def __init__(self, cond, left, right, token):
        super(Cond, self).__init__()
        self.cond = cond
        self.left = left
        self.right = right
        self.token = token
        return


    def _iterChildren(self):
        yield ('cond', self.cond)
        yield ('left', self.left)
        yield ('right', self.right)
        return
    children = property(_iterChildren)


from spike.compiler.tree.Node import Node


class Expr(Node):


    def __repr__(self):
        return "<expr %s>" % self.__class__.__name__


    def comma(self, right):
        from Comma import Comma
        c = Comma()
        c.append(self)
        c.append(right)
        return c


from Expr import Expr


class Id(Expr):


    def __init__(self, left, right, token):
        super(Id, self).__init__()
        self.left = left
        self.right = right
        self.token = token
        return


    def _iterChildren(self):
        yield ('left', self.left)
        yield ('right', self.right)
        return
    children = property(_iterChildren)


from Expr import Expr


class Literal(Expr):


    def __init__(self, token):
        super(Literal, self).__init__()
        self.tokens = [token]
        return


    def _iterChildren(self):
        return iter([])
    children = property(_iterChildren)



    def concat(self, token):
        self.tokens.append(token)
        return self


from Expr import Expr


class Name(Expr):


    def __init__(self, token):
        super(Name, self).__init__()
        self.token = token
        return


    def _iterChildren(self):
        return iter([])
    children = property(_iterChildren)


from Expr import Expr


class Or(Expr):


    def __init__(self, left, right, token):
        super(Or, self).__init__()
        self.left = left
        self.right = right
        self.token = token
        return


    def _iterChildren(self):
        yield ('left', self.left)
        yield ('right', self.right)
        return
    children = property(_iterChildren)


from Expr import Expr


class PostOp(Expr):


    def __init__(self, op, expr, token):
        super(PostOp, self).__init__()
        self.op = op
        self.expr = expr
        self.token = token
        return


    def _iterChildren(self):
        yield ('op', self.expr)
        yield ('expr', self.expr)
        return
    children = property(_iterChildren)


from Expr import Expr


class PreOp(Expr):


    def __init__(self, op, expr, token):
        super(PreOp, self).__init__()
        self.op = op
        self.expr = expr
        self.token = token
        return


    def _iterChildren(self):
        yield ('op', self.op)
        yield ('expr', self.expr)
        return
    children = property(_iterChildren)


from Expr import Expr


class UnaryOp(Expr):


    def __init__(self, op, expr, token):
        super(UnaryOp, self).__init__()
        self.op = op
        self.expr = expr
        self.token = token
        return


    def _iterChildren(self):
        yield ('op', self.op)
        yield ('expr', self.expr)
        return
    children = property(_iterChildren)
