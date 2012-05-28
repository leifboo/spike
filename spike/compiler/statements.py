

from Node import Node



class Stmt(Node):


    def __repr__(self):
        return "<stmt %s>" % self.__class__.__name__



class Break(Stmt):
    pass



class ClassDef(Stmt):


    childAttrNames = ('body', 'metaBody')


    def __init__(self, name, superclass, body, metaBody):
        super(ClassDef, self).__init__()
        self.name = name.value
        self.superclass = superclass and superclass.value or "Object"
        self.body = body
        self.metaBody = metaBody
        return



class Compound(Stmt, list):


    def _iterChildren(self):
        for index, stmt in enumerate(self):
            yield (index, stmt)
        return
    children = property(_iterChildren)



class Continue(Stmt):
    pass



class DoWhile(Stmt):


    childAttrNames = ('body', 'expr')


    def __init__(self, body, expr):
        super(DoWhile, self).__init__()
        self.body = body
        self.expr = expr
        return



class Expr(Stmt):


    childAttrNames = ('expr',)


    def __init__(self, expr):
        super(Expr, self).__init__()
        self.expr = expr
        return



class For(Stmt):


    childAttrNames = ('expr1', 'expr2', 'expr3', 'body')


    def __init__(self, expr1, expr2, expr3, body):
        super(For, self).__init__()
        self.expr1 = expr1
        self.expr2 = expr2
        self.expr3 = expr3
        self.body = body
        return



class IfElse(Stmt):


    childAttrNames = ('expr', 'ifTrue', 'ifFalse')


    def __init__(self, expr, ifTrue, ifFalse):
        super(IfElse, self).__init__()
        self.expr = expr
        self.ifTrue = ifTrue
        self.ifFalse = ifFalse
        return



class MethodDef(Stmt):


    childAttrNames = ('decl', 'body')


    def __init__(self, decl, body):
        super(MethodDef, self).__init__()
        self.decl = decl
        self.body = body
        return



class Return(Stmt):


    childAttrNames = ('expr',)


    def __init__(self, expr):
        super(Return, self).__init__()
        self.expr = expr
        return



class VarDef(Stmt):


    childAttrNames = ('expr',)


    def __init__(self, expr):
        super(VarDef, self).__init__()
        assert expr.declSpecs is not None # XXX: user error everywhere else
        self.expr = expr
        return



class While(Stmt):


    childAttrNames = ('expr', 'body')


    def __init__(self, expr, body):
        super(While, self).__init__()
        self.expr = expr
        self.body = body
        return



class Yield(Stmt):


    childAttrNames = ('expr',)


    def __init__(self, expr):
        super(Yield, self).__init__()
        self.expr = expr
        return
