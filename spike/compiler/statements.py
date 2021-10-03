

from Node import Node



class Stmt(Node):


    def __init__(self):
        super(Stmt, self).__init__()
        from cgen import Label
        self.label = Label()
        return


    def __repr__(self):
        return "<stmt %s>" % self.__class__.__name__



class Break(Stmt):
    childAttrNames = ()



class ClassDef(Stmt):


    childAttrNames = ('body', 'metaBody')


    # XXX: legacy code support
    u = property(lambda self: self)
    klass = property(lambda self: self)
    expr = property(lambda self: self.name)
    top = property(lambda self: self.body)
    bottom = property(lambda self: self.metaBody)


    def __init__(self, name, superclassName, body, metaBody):
        super(ClassDef, self).__init__()
        from expressions import Name
        self.name = name
        self.superclassName = superclassName
        self.body = body
        self.metaBody = metaBody
        self.superclassDef = None
        return



class Compound(Stmt, list):


    def __init__(self, iterable = None):
        super(Compound, self).__init__()
        if iterable:
            self.extend(iterable)
        return


    def _iterChildren(self):
        for index, stmt in enumerate(self):
            yield (index, stmt)
        return
    children = property(_iterChildren)


    def pairwise(self):
        '''Return the sequence (s0, s1), (s1, s2), ... (sN, None).'''
        return zip([None] + self, self + [None])[1:]


    def iterWithLabels(self, parentNextLabel):
        '''Generate the sequence (s0, s1.label), (s1, s2.label), (sN, parentNextLabel).'''
        for stmt, nextStmt in self.pairwise():
            childNextLabel = nextStmt.label if nextStmt else parentNextLabel
            yield stmt, childNextLabel
        return



class Continue(Stmt):
    childAttrNames = ()



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


    childAttrNames = ('init', 'test', 'incr', 'body')


    # XXX: legacy code support
    top = property(lambda self: self.body)


    def __init__(self, init, test, incr, body):
        super(For, self).__init__()
        self.init = init
        self.test = test
        self.incr = incr
        self.body = body
        return



class IfElse(Stmt):


    childAttrNames = ('expr', 'ifTrue', 'ifFalse')


    # XXX: legacy code support
    top = property(lambda self: self.ifTrue)
    bottom = property(lambda self: self.ifFalse)


    def __init__(self, expr, ifTrue, ifFalse):
        super(IfElse, self).__init__()
        self.expr = expr
        self.ifTrue = ifTrue
        self.ifFalse = ifFalse
        return



class MethodDef(Stmt):


    childAttrNames = ('decl', 'body')


    # XXX: legacy code support
    u = property(lambda self: self)
    method = property(lambda self: self)


    def __init__(self, decl, body):
        super(MethodDef, self).__init__()

        self.decl = decl
        self.body = body

        self.fixedArgs = []
        self.varArg = None

        self.minArgumentCount = 0
        self.maxArgumentCount = 0
        self.blockCount = 0

        return



class PragmaSource(Stmt):


    childAttrNames = ()


    def __init__(self, pathname):
        super(PragmaSource, self).__init__()
        self.pathname = pathname
        return



class Return(Stmt):


    childAttrNames = ('expr',)


    def __init__(self, expr):
        super(Return, self).__init__()
        self.expr = expr
        return



class SpecDef(Stmt):


    childAttrNames = ('expr',)


    # XXX: legacy code support
    u = property(lambda self: self)
    spec = property(lambda self: self)


    def __init__(self, expr):
        super(SpecDef, self).__init__()
        self.expr = expr
        return



class Trap(Stmt):
    childAttrNames = ()



class VarDef(Stmt):


    childAttrNames = ('expr',)


    defList = property(lambda self: self.expr.asList())


    def __init__(self, expr):
        super(VarDef, self).__init__()
        assert expr.declSpecs is not None # XXX: user error everywhere else
        self.expr = expr
        return



class While(Stmt):


    childAttrNames = ('expr', 'body')


    top = property(lambda self: self.body)


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



# aliases to work-around name collisions with 'expressions'

CompoundStmt    = Compound
ExprStmt        = Expr


# XXX: legacy code support

STMT_BREAK          = Break
STMT_COMPOUND       = Compound
STMT_CONTINUE       = Continue
STMT_DEF_CLASS      = ClassDef
STMT_DEF_METHOD     = MethodDef
#STMT_DEF_MODULE     = ModuleDef
STMT_DEF_SPEC       = SpecDef
STMT_DEF_VAR        = VarDef
STMT_DO_WHILE       = DoWhile
STMT_EXPR           = Expr
STMT_FOR            = For
STMT_IF_ELSE        = IfElse
STMT_PRAGMA_SOURCE  = PragmaSource
STMT_RETURN         = Return
STMT_TRAP           = Trap
STMT_WHILE          = While
STMT_YIELD          = Yield
