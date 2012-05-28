
class Oper(object):
    def __repr__(self):
        return "<oper %s>" % self.__class__.__name__

class Succ   (Oper): pass
class Pred   (Oper): pass
class Addr   (Oper): pass
class Ind    (Oper): pass
class Pos    (Oper): pass
class Neg    (Oper): pass
class BNeg   (Oper): pass
class LNeg   (Oper): pass
class Mul    (Oper): pass
class Div    (Oper): pass
class Mod    (Oper): pass
class Add    (Oper): pass
class Sub    (Oper): pass
class LShift (Oper): pass
class RShift (Oper): pass
class LT     (Oper): pass
class GT     (Oper): pass
class LE     (Oper): pass
class GE     (Oper): pass
class Eq     (Oper): pass
class NE     (Oper): pass
class BAnd   (Oper): pass
class BXOr   (Oper): pass
class BOr    (Oper): pass
class Apply  (Oper): pass
class Index  (Oper): pass

succ   = Succ()
pred   = Pred()
addr   = Addr()
ind    = Ind()
pos    = Pos()
neg    = Neg()
bneg   = BNeg()
lneg   = LNeg()
mul    = Mul()
div    = Div()
mod    = Mod()
add    = Add()
sub    = Sub()
lshift = LShift()
rshift = RShift()
lt     = LT()
gt     = GT()
le     = LE()
ge     = GE()
eq     = Eq()
ne     = NE()
band   = BAnd()
bxor   = BXOr()
bor    = BOr()
apply  = Apply()
index  = Index()
