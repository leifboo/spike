
class Oper(object):
    def __repr__(self):
        return "<oper %s>" % self.__class__.__name__

class Succ   (Oper): selector = '__succ__'
class Pred   (Oper): selector = '__pred__'
class Addr   (Oper): selector = '__addr__'
class Ind    (Oper): selector = '__ind__'
class Pos    (Oper): selector = '__pos__'
class Neg    (Oper): selector = '__neg__'
class BNeg   (Oper): selector = '__bneg__'
class LNeg   (Oper): selector = '__lneg__'
class Mul    (Oper): selector = '__mul__'
class Div    (Oper): selector = '__div__'
class Mod    (Oper): selector = '__mod__'
class Add    (Oper): selector = '__add__'
class Sub    (Oper): selector = '__sub__'
class LShift (Oper): selector = '__lshift__'
class RShift (Oper): selector = '__rshift__'
class LT     (Oper): selector = '__lt__'
class GT     (Oper): selector = '__gt__'
class LE     (Oper): selector = '__le__'
class GE     (Oper): selector = '__ge__'
class Eq     (Oper): selector = '__eq__'
class NE     (Oper): selector = '__ne__'
class BAnd   (Oper): selector = '__band__'
class BXOr   (Oper): selector = '__bxor__'
class BOr    (Oper): selector = '__bor__'
class Apply  (Oper): selector = '__apply__'
class Index  (Oper): selector = '__index__'

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
