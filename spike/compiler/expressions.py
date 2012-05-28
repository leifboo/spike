

from Node import Node



class Expr(Node):


    def __repr__(self):
        return "<expr %s>" % self.__class__.__name__


    def comma(self, right):
        c = Comma()
        c.append(self)
        c.append(right)
        return c



class And(Expr):


    childAttrNames = ('left', 'right')


    def __init__(self, left, right):
        super(And, self).__init__()
        self.left = left
        self.right = right
        return



class Assign(Expr):


    childAttrNames = ('op', 'left', 'right')


    def __init__(self, op, left, right):
        super(Assign, self).__init__()
        self.op = op
        self.left = left
        self.right = right
        return



class Attr(Expr):


    childAttrNames = ('obj', 'attr')


    def __init__(self, obj, attr):
        super(Attr, self).__init__()
        self.obj = obj
        self.attr = attr
        return



class AttrVar(Expr):


    childAttrNames = ('obj', 'attr')


    def __init__(self, obj, attr):
        super(AttrVar, self).__init__()
        self.obj = obj
        self.attr = attr
        return



class Binary(Expr):


    childAttrNames = ('op', 'left', 'right')


    def __init__(self, op, left, right):
        super(Binary, self).__init__()
        self.op = op
        self.left = left
        self.right = right
        return



class Block(Expr):


    def __init__(self, args, stmtList, expr):
        super(Block, self).__init__()
        self.args = args
        self.stmtList = stmtList
        self.expr = expr
        return


    def _iterChildren(self):
        for index, stmt in enumerate(self.stmtList):
            yield (index, self.stmtList)
        yield ('expr', self.expr)
        return
    children = property(_iterChildren)



class Call(Expr):


    childAttrNames = ('oper', 'func', 'fixedArgs', 'varArgs')


    def __init__(self, oper, func, fixedArgs, varArgs):
        super(Call, self).__init__()
        self.oper = oper
        self.func = func
        self.fixedArgs = fixedArgs
        self.varArgs = varArgs
        return



class Comma(Expr, list):


    def _iterChildren(self):
        for index, expr in enumerate(self):
            yield (index, expr)
        return
    children = property(_iterChildren)


    def comma(self, right):
        self.append(right)
        return self



class Compound(Expr):


    childAttrNames = ('expr',)


    def __init__(self, expr):
        super(Compound, self).__init__()
        self.expr = expr
        return



class Cond(Expr):


    childAttrNames = ('cond', 'left', 'right')


    def __init__(self, cond, left, right):
        super(Cond, self).__init__()
        self.cond = cond
        self.left = left
        self.right = right
        return



class Id(Expr):


    childAttrNames = ('left', 'right')


    def __init__(self, left, right, invert = False):
        super(Id, self).__init__()
        self.left = left
        self.right = right
        self.invert = invert
        return



class Keyword(Expr):


    childAttrNames = ('receiver', 'keyword', 'argKeywords', 'args')


    def __init__(self, receiver, keyword, argKeywords, args):
        super(Keyword, self).__init__()
        self.receiver = receiver
        self.keyword = keyword
        self.argKeywords = argKeywords
        self.args = args
        return



class Literal(Expr):


    childAttrNames = ()


    def __init__(self, token):
        super(Literal, self).__init__()
        self.tokens = [token]
        return


    def concat(self, token):
        self.tokens.append(token)
        return self



class Name(Expr):


    childAttrNames = ()


    def __init__(self, token):
        super(Name, self).__init__()
        self.token = token
        return



class Or(Expr):


    childAttrNames = ('left', 'right')


    def __init__(self, left, right):
        super(Or, self).__init__()
        self.left = left
        self.right = right
        return



class PostOp(Expr):


    childAttrNames = ('op', 'expr')


    def __init__(self, op, expr):
        super(PostOp, self).__init__()
        self.op = op
        self.expr = expr
        return



class PreOp(Expr):


    childAttrNames = ('op', 'expr')


    def __init__(self, op, expr):
        super(PreOp, self).__init__()
        self.op = op
        self.expr = expr
        return



class Unary(Expr):


    childAttrNames = ('op', 'expr')


    def __init__(self, op, expr):
        super(Unary, self).__init__()
        self.op = op
        self.expr = expr
        return
