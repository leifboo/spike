

class NodeFactory(object):

    from statements import Break      as stmtBreak
    from statements import Compound   as stmtCompound
    from statements import Continue   as stmtContinue
    from statements import ClassDef   as stmtDefClass
    from statements import MethodDef  as stmtDefMethod
    # STMT_DEF_MODULE
    # STMT_DEF_SPEC
    from statements import VarDef     as stmtDefVar
    from statements import DoWhile    as stmtDoWhile
    from statements import Expr       as stmtExpr
    from statements import For        as stmtFor
    from statements import IfElse     as stmtIfElse
    # STMT_PRAGMA_SOURCE
    from statements import Return     as stmtReturn
    from statements import Trap       as stmtTrap
    from statements import While      as stmtWhile
    from statements import Yield      as stmtYield

    def stmtExprOrDefVar(self, expr):
        declSpecs = getattr(expr, 'declSpecs', None)
        if declSpecs:
            return self.stmtDefVar(expr)
        return self.stmtExpr(expr)
    
    from expressions import And       as exprAnd
    from expressions import Assign    as exprAssign
    from expressions import Attr      as exprAttr
    from expressions import AttrVar   as exprAttrVar
    from expressions import Binary    as exprBinary
    from expressions import Block     as exprBlock
    from expressions import Call      as exprCall
    from expressions import Compound  as exprCompound
    from expressions import Cond      as exprCond
    from expressions import Id        as exprId
    from expressions import Keyword   as exprKeyword
    from expressions import Literal   as exprLiteral
    from expressions import Name      as exprName
    exprNI = lambda self, left, right: self.exprId(left, right, True)
    from expressions import Or        as exprOr
    from expressions import PostOp    as exprPostOp
    from expressions import PreOp     as exprPreOp
    from expressions import Unary     as exprUnary
    
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

    from tokens import Integer     as int
    from tokens import Float       as float
    from tokens import Character   as char
    from tokens import String      as str
    from tokens import Symbol      as sym
    from tokens import Identifier  as id
    from tokens import Specifier   as spec
    from tokens import Keyword     as kw
