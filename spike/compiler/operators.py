
from literals import Symbol

class Oper(object):
    name = property(lambda self: self.__class__.__name__)
    def __repr__(self):
        return "<oper %s>" % self.__class__.__name__

class Succ   (Oper): selector = Symbol('__succ__')
class Pred   (Oper): selector = Symbol('__pred__')
class Addr   (Oper): selector = Symbol('__addr__')
class Ind    (Oper): selector = Symbol('__ind__')
class Pos    (Oper): selector = Symbol('__pos__')
class Neg    (Oper): selector = Symbol('__neg__')
class BNeg   (Oper): selector = Symbol('__bneg__')
class LNeg   (Oper): selector = Symbol('__lneg__')
class Mul    (Oper): selector = Symbol('__mul__')
class Div    (Oper): selector = Symbol('__div__')
class Mod    (Oper): selector = Symbol('__mod__')
class Add    (Oper): selector = Symbol('__add__')
class Sub    (Oper): selector = Symbol('__sub__')
class LShift (Oper): selector = Symbol('__lshift__')
class RShift (Oper): selector = Symbol('__rshift__')
class LT     (Oper): selector = Symbol('__lt__')
class GT     (Oper): selector = Symbol('__gt__')
class LE     (Oper): selector = Symbol('__le__')
class GE     (Oper): selector = Symbol('__ge__')
class Eq     (Oper): selector = Symbol('__eq__')
class NE     (Oper): selector = Symbol('__ne__')
class BAnd   (Oper): selector = Symbol('__band__')
class BXOr   (Oper): selector = Symbol('__bxor__')
class BOr    (Oper): selector = Symbol('__bor__')
class Apply  (Oper): selector = Symbol('__apply__')
class Index  (Oper): selector = Symbol('__index__')

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


# XXX: legacy code support

OPER_SUCC   = succ
OPER_PRED   = pred
OPER_ADDR   = addr
OPER_IND    = ind
OPER_POS    = pos
OPER_NEG    = neg
OPER_BNEG   = bneg
OPER_LNEG   = lneg
OPER_MUL    = mul
OPER_DIV    = div
OPER_MOD    = mod
OPER_ADD    = add
OPER_SUB    = sub
OPER_LSHIFT = lshift
OPER_RSHIFT = rshift
OPER_LT     = lt
OPER_GT     = gt
OPER_LE     = le
OPER_GE     = ge
OPER_EQ     = eq
OPER_NE     = ne
OPER_BAND   = band
OPER_BXOR   = bxor
OPER_BOR    = bor

OPER_APPLY  = apply
OPER_INDEX  = index
