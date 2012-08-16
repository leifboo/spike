

from Node import Node



SPEC_TYPE           = 0x0000000F
SPEC_TYPE_OBJ       = 0x00000001
SPEC_TYPE_INT       = 0x00000002
SPEC_TYPE_CHAR      = 0x00000003

SPEC_STORAGE        = 0x000000F0
SPEC_STORAGE_IMPORT = 0x00000010
SPEC_STORAGE_EXPORT = 0x00000020
SPEC_STORAGE_EXTERN = 0x00000030

SPEC_CALL_CONV           = 0x00000F00
SPEC_CALL_CONV_C         = 0x00000100
SPEC_CALL_CONV_EXTENSION = 0x00000200



class Expr(Node):


    declSpecs = ()


    lineNum = property(lambda self: self.firstChildExpr().lineNum)


    def __init__(self):
        super(Expr, self).__init__()
        from cgen import Label
        self.label = Label()
        self.endLabel = Label()
        return


    def __repr__(self):
        return "<expr %s>" % self.__class__.__name__


    def firstChildExpr(self):
        for attr, child in self.children:
            if isinstance(child, Expr):
                return child
        return None


    def comma(self, right):
        c = Comma()
        c.append(self)
        c.append(right)
        return c


    def asList(self):
        return [self]



class And(Expr):


    childAttrNames = ('left', 'right')


    def __init__(self, left, right):
        super(And, self).__init__()
        self.left = left
        self.right = right
        return



class Assign(Expr):


    childAttrNames = ('oper', 'left', 'right')


    def __init__(self, oper, left, right):
        super(Assign, self).__init__()
        self.oper = oper
        self.left = left
        self.right = right
        return



class Attr(Expr):


    childAttrNames = ('obj', 'attr')


    # XXX: legacy code support
    left = property(lambda self: self.obj)


    def __init__(self, obj, attrToken):
        from literals import Symbol
        assert isinstance(attrToken.value, Symbol)
        super(Attr, self).__init__()
        self.obj = obj
        self.attr = attrToken.value
        return



class AttrVar(Expr):


    childAttrNames = ('obj', 'attr')


    # XXX: legacy code support
    left = property(lambda self: self.obj)
    right = property(lambda self: self.attr)


    def __init__(self, obj, attr):
        super(AttrVar, self).__init__()
        self.obj = obj
        self.attr = attr
        return



class Binary(Expr):


    childAttrNames = ('oper', 'left', 'right')


    def __init__(self, oper, left, right):
        super(Binary, self).__init__()
        self.oper = oper
        self.left = left
        self.right = right
        return



class Block(Expr):


    # XXX: legacy code support
    u = property(lambda self: self)
    _def = property(lambda self: self)
    aux = property(lambda self: self)
    block = property(lambda self: self)
    right = property(lambda self: self.expr)
    argList = property(lambda self: self.args)
    argumentCount = property(lambda self: len(self.args))


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


    childAttrNames = ('oper', 'func', 'fixedArgs', 'varArg')


    # XXX: legacy code support
    left = property(lambda self: self.func)
    var = property(lambda self: self.varArg)


    def __init__(self, oper, func, fixedArgs, varArg):
        super(Call, self).__init__()
        self.oper = oper
        self.func = func
        self.fixedArgs = fixedArgs
        self.varArg = varArg
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


    def asList(self):
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


    def __init__(self, left, right, inverted = False):
        super(Id, self).__init__()
        self.left = left
        self.right = right
        self.inverted = inverted
        return



class Keyword(Expr):


    childAttrNames = ('receiver', 'keyword', 'argKeywords', 'args')


    # XXX: legacy code support
    left = property(lambda self: self.receiver)


    # so as to be interchangable with Call expr
    fixedArgs = property(lambda self: self.args)
    varArg = None


    def getSelector(self):
        from literals import Symbol
        selector = ''
        if self.keyword:
            selector += self.keyword
            if self.argKeywords:
                selector += ' '
        for kw in self.argKeywords:
            selector += kw + ':'
        return Symbol(selector)
    selector = property(getSelector)


    def __init__(self, receiver, keyword, argKeywords, args):
        super(Keyword, self).__init__()
        self.receiver = receiver
        self.keyword = keyword.value if keyword else None
        self.argKeywords = [kw.value for kw in argKeywords] if argKeywords else []
        self.args = args
        return



class Literal(Expr):


    childAttrNames = ()


    # XXX: legacy code support
    aux = property(lambda self: self)


    lineNum = property(lambda self: self.tokens[0].lineNum)


    def __init__(self, token):
        super(Literal, self).__init__()
        self.tokens = [token]
        self.value = token.value
        return


    def __repr__(self):
        return "<expr %s %r>" % (self.__class__.__name__, self.value)


    def concat(self, token):
        # called for string literals only
        self.tokens.append(token)
        self.value += token.value
        return self



class Name(Expr):


    childAttrNames = ()


    # XXX: legacy code support
    u = property(lambda self: self)
    ref = property(lambda self: self)
    _def = property(lambda self: self)


    specifiers = 0

    lineNum = property(lambda self: self.token.lineNum)


    def __init__(self, arg):
        from literals import Symbol
        super(Name, self).__init__()
        if isinstance(arg, basestring):
            self.token = None
            self.sym = Symbol(arg)
        else:
            assert isinstance(arg.value, Symbol)
            self.token = arg
            self.sym = arg.value
        # initial state is undefined reference
        self.definition = None
        self.stmt = None # XXX: cycle
        return


    def __repr__(self):
        return "<expr %s %r>" % (self.__class__.__name__, self.sym)


    def getPushOpcode(self):
        from spike.il import (
            OPCODE_PUSH_GLOBAL,
            OPCODE_PUSH_INST_VAR,
            OPCODE_PUSH_LOCAL,
            )
        opcode = {
            0: self.builtInPushOpcode,
            1: OPCODE_PUSH_GLOBAL,
            2: OPCODE_PUSH_INST_VAR,
            3: OPCODE_PUSH_LOCAL,
        }
        return opcode[self.level]

    def getStoreOpcode(self):
        from spike.il import (
            OPCODE_STORE_GLOBAL,
            OPCODE_STORE_INST_VAR,
            OPCODE_STORE_LOCAL,
            )
        opcode = {
            1: OPCODE_STORE_GLOBAL,
            2: OPCODE_STORE_INST_VAR,
            3: OPCODE_STORE_LOCAL,
        }
        return opcode[self.level]

    builtInPushOpcode = None
    pushOpcode = property(getPushOpcode)
    storeOpcode = property(getStoreOpcode)



class Or(Expr):


    childAttrNames = ('left', 'right')


    def __init__(self, left, right):
        super(Or, self).__init__()
        self.left = left
        self.right = right
        return



class PostOp(Expr):


    childAttrNames = ('oper', 'expr')


    # XXX: legacy code support
    left = property(lambda self: self.expr)


    def __init__(self, oper, expr):
        super(PostOp, self).__init__()
        self.oper = oper
        self.expr = expr
        return



class PreOp(Expr):


    childAttrNames = ('oper', 'expr')


    # XXX: legacy code support
    left = property(lambda self: self.expr)


    def __init__(self, oper, expr):
        super(PreOp, self).__init__()
        self.oper = oper
        self.expr = expr
        return



class Unary(Expr):


    childAttrNames = ('oper', 'expr')


    # XXX: legacy code support
    left = property(lambda self: self.expr)


    def __init__(self, oper, expr):
        super(Unary, self).__init__()
        self.oper = oper
        self.expr = expr
        return



# aliases to work-around name collisions with 'statements'

CompoundExpr = Compound


# XXX: legacy code support

EXPR_AND        = And
EXPR_ASSIGN     = Assign
EXPR_ATTR       = Attr
EXPR_ATTR_VAR   = AttrVar
EXPR_BINARY     = Binary
EXPR_BLOCK      = Block
EXPR_CALL       = Call
EXPR_COMMA      = Comma
EXPR_COMPOUND   = Compound
EXPR_COND       = Cond
EXPR_ID         = Id
EXPR_KEYWORD    = Keyword
EXPR_LITERAL    = Literal
EXPR_NAME       = Name
#EXPR_NI         = NI
EXPR_OR         = Or
EXPR_POSTOP     = PostOp
EXPR_PREOP      = PreOp
EXPR_UNARY      = Unary
