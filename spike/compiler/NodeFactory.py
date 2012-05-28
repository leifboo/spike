

class NodeFactory(object):

    from statements import ClassDef   as classDef
    from statements import Compound   as stmtCompound
    from statements import Expr       as stmtExpr
    from statements import MethodDef  as methodDef

    from expressions import And       as exprAnd
    from expressions import Assign    as exprAssign
    from expressions import Attr      as exprAttr
    from expressions import AttrVar   as exprAttrVar
    from expressions import BinaryOp  as exprBinaryOp
    from expressions import Block     as exprBlock
    from expressions import Call      as exprCall
    from expressions import Compound  as exprCompound
    from expressions import Cond      as exprCond
    from expressions import Expr      as exprExpr
    from expressions import Id        as exprId
    from expressions import Literal   as exprLiteral
    from expressions import Name      as exprName
    from expressions import Or        as exprOr
    from expressions import PostOp    as exprPostOp
    from expressions import PreOp     as exprPreOp
    from expressions import UnaryOp   as exprUnaryOp
    
    from operators import succ    as operSucc
    from operators import pred    as operPred
    from operators import addr    as operAddr
    from operators import ind     as operInd
    from operators import pos     as operPos
    from operators import neg     as operNeg
    from operators import bneg    as operBNeg
    from operators import lneg    as operLNeg
    from operators import mul     as operMul
    from operators import div     as operDiv
    from operators import mod     as operMod
    from operators import add     as operAdd
    from operators import sub     as operSub
    from operators import lshift  as operLShift
    from operators import rshift  as operRShift
    from operators import lt      as operLT
    from operators import gt      as operGT
    from operators import le      as operLE
    from operators import ge      as operGE
    from operators import eq      as operEq
    from operators import ne      as operNE
    from operators import band    as operBAnd
    from operators import bxor    as operBXOr
    from operators import bor     as operBOr
    from operators import apply   as operApply
    from operators import index   as operIndex

