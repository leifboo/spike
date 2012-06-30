

from literals import *
from operators import *
from expressions import *
from statements import *
from spike.il import *



declareBuiltInClasses = False #True
declareObject = False #True



def checkDeclSpecs(declSpecs, checker, _pass):

    specifiers = 0

    for declSpec in declSpecs:
        assert declSpec.kind == EXPR_NAME, "identifier expected"
        checker.st.bind(declSpec, checker.requestor)
        if declSpec.definition:
            # XXX: check for invalid combinations
            specDef = declSpec.definition.u._def.stmt
            if specifiers & specDef.u.spec.mask:
                checker.requestor.invalidSpecifier(declSpec)
            specifiers = (specifiers & (~specDef.u.spec.mask)) | specDef.u.spec.value

    return specifiers


def checkVarDef(varDef, stmt, checker, _pass):

    if _pass == 1:
        specifiers = checkDeclSpecs(varDef.expr.declSpecs, checker, _pass)

    for expr in varDef.defList:
        if expr.kind == EXPR_ASSIGN:
            _def = expr.left
            if expr.oper != OPER_EQ and _pass == 1:
                checker.requestor.invalidVariableDefinition(_def)
                # fall though and define it, to reduce the number of errors
            checkExpr(expr.right, stmt, checker, _pass)
        else:
            _def = expr

        while _def.kind == EXPR_UNARY and _def.oper == OPER_IND:
            _def = _def.left

        if _def.kind != EXPR_NAME:
            if _pass == 1:
                checker.requestor.invalidVariableDefinition(_def)
            continue

        if _pass == 1:
            checker.st.insert(_def, checker.requestor)
            _def.u._def.stmt = stmt
            _def.specifiers = specifiers

    return


def checkBlock(expr, stmt, checker, outerPass):

    if outerPass != 2:
        return

    checker.currentMethod.u.method.blockCount += 1

    checker.st.enterScope(False)

    expr.u._def.index = 0
    
    if expr.aux.block.argList:
        # declare block arguments
        for arg in expr.aux.block.argList:
            if arg.kind != EXPR_NAME:
                checker.requestor.invalidArgumentDefinition(arg)
                continue
            checker.st.insert(arg, checker.requestor)

        # The 'index' of the block itself is the index of the first
        # argument.  The interpreter uses this index to find the
        # destination the block arguments; see OPCODE_CALL_BLOCK.
        firstArg = expr.aux.block.argList[0]
        if firstArg.kind == EXPR_NAME:
            expr.u._def.index = firstArg.u._def.index
        # XXX: handle other declarators

    for innerPass in range(1, 4):
        if expr.aux.block.stmtList:
            for s in expr.aux.block.stmtList:
                checkStmt(s, stmt, checker, innerPass)

        if expr.right:
            checkExpr(expr.right, stmt, checker, innerPass)

    # XXX: this is only needed for arbitrary nesting
    expr.aux.block.localCount = checker.st.currentScope.context.nDefs - expr.aux.block.argumentCount

    checker.st.exitScope()

    return


def checkExpr(expr, stmt, checker, _pass):

    if expr.kind == EXPR_LITERAL:
        pass

    elif expr.kind == EXPR_NAME:
        if _pass == 2:
            checker.st.bind(expr, checker.requestor)

    elif expr.kind == EXPR_BLOCK:
        checkBlock(expr, stmt, checker, _pass)

    elif expr.kind == EXPR_COMMA:
        for e in expr:
            checkExpr(e, stmt, checker, _pass)

    elif expr.kind == EXPR_COMPOUND:
        for arg in expr:
            checkExpr(arg, stmt, checker, _pass)
        if expr.var:
            checkExpr(expr.var, stmt, checker, _pass)

    elif expr.kind in (EXPR_CALL, EXPR_KEYWORD):
        checkExpr(expr.left, stmt, checker, _pass)
        for arg in expr.fixedArgs:
            checkExpr(arg, stmt, checker, _pass)
        if expr.varArg:
            checkExpr(expr.varArg, stmt, checker, _pass)

    elif expr.kind in (EXPR_ATTR, EXPR_POSTOP, EXPR_PREOP, EXPR_UNARY):
        checkExpr(expr.left, stmt, checker, _pass)

    elif expr.kind in (EXPR_AND,
                       EXPR_ASSIGN,
                       EXPR_ATTR_VAR,
                       EXPR_BINARY,
                       EXPR_ID,
                       EXPR_OR,
                       ):
        checkExpr(expr.left, stmt, checker, _pass)
        checkExpr(expr.right, stmt, checker, _pass)

    elif expr.kind == EXPR_COND:
        checkExpr(expr.cond, stmt, checker, _pass)
        checkExpr(expr.left, stmt, checker, _pass)
        checkExpr(expr.right, stmt, checker, _pass)

    return


def checkMethodDef(stmt, outer, checker, outerPass):

    checker.currentMethod = stmt

    expr = stmt.decl
    body = stmt.body
    assert body.kind == STMT_COMPOUND, "compound statement expected"

    if outerPass == 1:
        specifiers = checkDeclSpecs(expr.declSpecs, checker, outerPass)
        expr.specifiers = specifiers
        ns = METHOD_NAMESPACE_RVALUE
        name = None
        
        # XXX: This can only be meaningfully composed with EXPR_CALL.
        while (expr.kind == EXPR_UNARY) and (expr.oper == OPER_IND):
            expr = expr.left

        if expr.kind == EXPR_NAME:
            name = expr.sym

        elif expr.kind == EXPR_ATTR:
            if expr.left.kind != EXPR_NAME:
                checker.requestor.invalidMethodDeclarator(expr)
            else:
                name = expr.attr

        elif expr.kind == EXPR_ASSIGN:
            if expr.right.kind != EXPR_NAME:
                checker.requestor.invalidMethodDeclarator(expr)

            else:
                ns = METHOD_NAMESPACE_LVALUE

                if expr.left.kind == EXPR_NAME:
                    name = expr.left.sym
                    stmt.u.method.fixedArgs = [expr.right]

                elif expr.left.kind == EXPR_CALL:
                    if (expr.left.oper != OPER_INDEX) or (expr.left.left.sym != 'self'):
                        checker.requestor.invalidMethodDeclarator(expr)

                    else:
                        name = expr.left.oper.selector
                        # build the formal parameter list
                        assert expr.left.varArg is None, 'XXX: vararg index lvalue'
                        stmt.u.method.fixedArgs = expr.left.fixedArgs + [expr.right]

                else:
                    checker.requestor.invalidMethodDeclarator(expr)

        elif expr.kind == EXPR_CALL:
            if expr.left.kind != EXPR_NAME:
                checker.requestor.invalidMethodDeclarator(expr)

            else:
                if expr.left.sym == 'self':
                    name = expr.oper.selector

                elif expr.oper == OPER_APPLY:
                    name = expr.left.sym

                else:
                    # XXX: Should we allow "foo[...] {}" as a method
                    # definition?  More generally, could the method
                    # declarator be seen as the application of an inverse
                    # thingy?
                    checker.requestor.invalidMethodDeclarator(expr)

                if name:
                    if (not outer) or (outer.kind != STMT_DEF_CLASS):
                        # declare naked functions
                        checker.st.insert(expr.left, checker.requestor)
                        expr.left.specifiers = specifiers

                    stmt.u.method.fixedArgs = expr.fixedArgs
                    stmt.u.method.varArg = expr.var

        elif expr.kind in (EXPR_UNARY, EXPR_BINARY):
            if (expr.left.kind != EXPR_NAME) or (expr.left.sym != 'self'):
                checker.requestor.invalidMethodDeclarator(expr)

            else:
                oper = expr.oper
                if expr.kind == EXPR_BINARY:
                    if expr.right.kind == EXPR_NAME:
                        stmt.u.method.fixedArgs = [expr.right]

                    elif expr.right.kind == EXPR_LITERAL:
                        # NB: a float might also equal one, so we check for integer
                        if isinstance(expr.right.aux.value, Integer) and expr.right.aux.value == 1:
                            if oper == OPER_ADD:
                                oper = OPER_SUCC
                            elif oper == OPER_SUB:
                                oper = OPER_PRED
                            else:
                                checker.requestor.invalidMethodDeclarator(expr)
                                oper = None
                        else:
                            checker.requestor.invalidMethodDeclarator(expr)
                            oper = None

                    else:
                        checker.requestor.invalidMethodDeclarator(expr)
                        oper = None

                if oper is not None:
                    name = oper.selector

        elif expr.kind == EXPR_KEYWORD:
            if (expr.left.kind != EXPR_NAME) or (expr.left.sym != 'self'):
                checker.requestor.invalidMethodDeclarator(expr)

            else:
                name = expr.selector
                stmt.u.method.fixedArgs = expr.args

        else:
            checker.requestor.invalidMethodDeclarator(expr)

        assert name is None or isinstance(name, Symbol), "expected symbol, not %r" % name
        stmt.u.method.ns = ns
        stmt.u.method.name = name

    elif outerPass == 2:
        if (not outer) or (outer.kind != STMT_DEF_CLASS):
            # naked (global) function -- enter module instance context
            checker.st.enterScope(True)

        checker.st.enterScope(True)

        # declare function arguments
        state = 0
        for arg in stmt.u.method.fixedArgs:
            _def = arg
            while (_def.kind == EXPR_UNARY) and (_def.oper == OPER_IND):
                _def = _def.left

            if state == 0 and _def.kind != EXPR_NAME:
                state = 1

            if state == 0:
                specifiers = checkDeclSpecs(_def.declSpecs, checker, 1)
                _def.specifiers = specifiers
                checker.st.insert(_def, checker.requestor)
                stmt.u.method.minArgumentCount += 1
                stmt.u.method.maxArgumentCount += 1

            else:
                if arg.kind == EXPR_ASSIGN:
                    _def = arg.left
                else:
                    _def = arg
                    if _def.kind == EXPR_NAME:
                        checker.requestor.nonDefaultArgumentFollowsDefaultArgument(arg)
                        # fall though and define it, to reduce the number of errors

                while (_def.kind == EXPR_UNARY) and (_def.oper == OPER_IND):
                    _def = _def.left

                if _def.kind != EXPR_NAME:
                    checker.requestor.invalidArgumentDefinition(arg)
                    continue

                checker.st.insert(_def, checker.requestor)
                stmt.u.method.maxArgumentCount += 1

        arg = stmt.u.method.varArg
        if arg:
            if arg.kind != EXPR_NAME:
                checker.requestor.invalidArgumentDefinition(arg)
            else:
                checker.st.insert(arg, checker.requestor)


        if ((expr.specifiers & SPEC_STORAGE) == SPEC_STORAGE_EXTERN) and len(body):
            checker.requestor.externMethodWithANonEmptyBody(expr)

        for innerPass in range(1, 4):
            for arg in stmt.u.method.fixedArgs:
                if arg.kind == EXPR_ASSIGN:
                    checkExpr(arg.right, stmt, checker, innerPass)

            for s in body:
                checkStmt(s, stmt, checker, innerPass)


        stmt.u.method.localCount = ((checker.st.currentScope.context.nDefs
                                     - stmt.u.method.maxArgumentCount)
                                    - (1 if stmt.u.method.varArg else 0))
        checker.st.exitScope()
        if (not outer) or (outer.kind != STMT_DEF_CLASS):
            checker.st.exitScope()


    checker.currentMethod = None
    return


def registerSubclass(subclassDef, checker):

    # insert this class onto the superclass's subclass list
    superclassDef = subclassDef.u.klass.superclassDef
    superclassDef.subclassDefs.append(subclassDef)

    if superclassDef.u.klass.predefined:
        # this is a 'root' class
        checker.rootClassList.append(subclassDef)

    return


def checkForSuperclassCycle(aClassDef, checker):
    # Floyd's cycle-finding algorithm

    tortoise = hare = aClassDef.u.klass.superclassDef
    if not tortoise:
        return

    hare = hare.u.klass.superclassDef
    while hare and (tortoise != hare):
        tortoise = tortoise.u.klass.superclassDef
        hare = hare.u.klass.superclassDef
        if not hare:
            return
        hare = hare.u.klass.superclassDef

    if hare:
        checker.requestor.cycleInSuperclassChain(aClassDef.u.klass.superclassName)

    return


def checkClassBody(body, stmt, outer, checker, outerPass):
    assert body.kind == STMT_COMPOUND, "compound statement expected"
    checker.st.enterScope(True)
    for innerPass in range(1, 4):
        for s in body:
            checkStmt(s, stmt, checker, innerPass)

    varCount = checker.st.currentScope.context.nDefs
    checker.st.exitScope()
    return varCount


def checkClassDef(stmt, outer, checker, outerPass):

    if outerPass == 1:
        assert stmt.expr.kind == EXPR_NAME, "identifier expected"
        checker.st.insert(stmt.expr, checker.requestor)
        stmt.expr.u._def.stmt = stmt

        checkExpr(stmt.u.klass.superclassName, stmt, checker, outerPass)

    elif outerPass == 2:
        checkExpr(stmt.u.klass.superclassName, stmt, checker, outerPass)

        expr = stmt.u.klass.superclassName.definition
        if expr:
            if (expr.kind == EXPR_NAME and
                expr.u._def.stmt and
                expr.u._def.stmt.kind == STMT_DEF_CLASS
                ):
                # for convenience, cache a direct link to the superclass def
                stmt.u.klass.superclassDef = expr.u._def.stmt
                registerSubclass(stmt, checker)
            elif expr.sym == 'null':
                # for class Object, which has no superclass
                pass
            else:
                checker.requestor.invalidSuperclassSpecification(stmt.u.klass.superclassName)
                return
        # else undefined

        stmt.u.klass.instVarCount = checkClassBody(stmt.top, stmt, outer, checker, outerPass)
        if stmt.bottom:
            stmt.u.klass.classVarCount = checkClassBody(stmt.bottom, stmt, outer, checker, outerPass)
        else:
            stmt.u.klass.classVarCount = 0

    elif outerPass == 3:
        checkExpr(stmt.u.klass.superclassName, stmt, checker, outerPass)
        checkForSuperclassCycle(stmt, checker)

    return


def checkStmt(stmt, outer, checker, outerPass):

    if stmt.kind in (STMT_BREAK, STMT_CONTINUE):
        pass

    elif stmt.kind == STMT_COMPOUND:
        if outerPass == 2:
            checker.st.enterScope(False)
            for innerPass in range(1, 4):
                for s in stmt:
                    checkStmt(s, stmt, checker, innerPass)
            checker.st.exitScope()

    elif stmt.kind == STMT_DEF_VAR:
        checkVarDef(stmt, stmt, checker, outerPass)

    elif stmt.kind == STMT_DEF_METHOD:
        checkMethodDef(stmt, outer, checker, outerPass)

    elif stmt.kind == STMT_DEF_CLASS:
        checkClassDef(stmt, outer, checker, outerPass)

    elif stmt.kind == STMT_DO_WHILE:
        checkExpr(stmt.expr, stmt, checker, outerPass)
        checkStmt(stmt.top, stmt, checker, outerPass)

    elif stmt.kind == STMT_EXPR:
        if stmt.expr:
            checkExpr(stmt.expr, stmt, checker, outerPass)

    elif stmt.kind == STMT_FOR:
        if stmt.init:
            checkExpr(stmt.init, stmt, checker, outerPass)
        if stmt.test:
            checkExpr(stmt.test, stmt, checker, outerPass)
        if stmt.incr:
            checkExpr(stmt.incr, stmt, checker, outerPass)
        checkStmt(stmt.top, stmt, checker, outerPass)

    elif stmt.kind == STMT_IF_ELSE:
        checkExpr(stmt.expr, stmt, checker, outerPass)
        checkStmt(stmt.top, stmt, checker, outerPass)
        if stmt.bottom:
            checkStmt(stmt.bottom, stmt, checker, outerPass)

    elif stmt.kind == STMT_PRAGMA_SOURCE:
        checker.requestor.source = stmt.pathname

    elif stmt.kind in (STMT_RETURN, STMT_YIELD):
        if stmt.expr:
            checkExpr(stmt.expr, stmt, checker, outerPass)

    elif stmt.kind == STMT_WHILE:
        checkExpr(stmt.expr, stmt, checker, outerPass)
        checkStmt(stmt.top, stmt, checker, outerPass)

    else:
        assert False, "unexpected statement node: %r" % stmt

    return



#------------------------------------------------------------------------

from collections import namedtuple


PseudoVariable = namedtuple('PseudoVariable', ('name', 'pushOpcode'))

pseudoVariables = (
    PseudoVariable('self',          OPCODE_PUSH_SELF),
    PseudoVariable('super',         OPCODE_PUSH_SUPER),
    PseudoVariable('false',         OPCODE_PUSH_FALSE),
    PseudoVariable('true',          OPCODE_PUSH_TRUE),
    PseudoVariable('null',          OPCODE_PUSH_NULL),
    PseudoVariable('thisContext',   OPCODE_PUSH_CONTEXT),
    )


Spec = namedtuple('Spec', ('name', 'mask', 'value'))

builtInSpecifiers = (
    Spec('obj',     SPEC_TYPE,      SPEC_TYPE_OBJ),
    Spec('int',     SPEC_TYPE,      SPEC_TYPE_INT),
    Spec('char',    SPEC_TYPE,      SPEC_TYPE_CHAR),
    Spec('import',  SPEC_STORAGE,   SPEC_STORAGE_IMPORT),
    Spec('export',  SPEC_STORAGE,   SPEC_STORAGE_EXPORT),
    Spec('extern',  SPEC_STORAGE,   SPEC_STORAGE_EXTERN),
    Spec('cdecl',   SPEC_CALL_CONV, SPEC_CALL_CONV_C),
    )


def declareBuiltInSpecifier(bis, st):
    from statements import SpecDef
    from expressions import Name
    
    specDef = SpecDef(Name(bis.name))
    specDef.expr.u._def.stmt = specDef
    specDef.u.spec.mask = bis.mask
    specDef.u.spec.value = bis.value
    
    st.insert(specDef.expr, None)
    
    return


def newPseudoVariable(pv, st):
    from expressions import Name
    newExpr = Name(pv.name)
    newExpr.level = 0
    newExpr.builtInPushOpcode = pv.pushOpcode
    return newExpr


def predefinedClassDef(predefClass, checker):
    from statements import ClassDef
    from expressions import Name
    
    newStmt = ClassDef()
    newStmt.expr = Name(predefClass.name)
    newStmt.expr.u._def.stmt = newStmt
    newStmt.u.klass.predefined = True
    
    return newStmt


def globalVarDef(name, checker):
    from statements import VarDef
    from expressions import Name
    
    newStmt = VarDef()
    newStmt.expr = Name(name)
    newStmt.expr.u._def.stmt = newStmt
    
    return newStmt


def declareClass(aClass, predefList, checker):
    classDef = predefinedClassDef(aClass, checker)
    checker.st.insert(classDef.expr, checker.requestor)
    classDef.expr.u._def.initValue = aClass
    predefList.append(classDef)
    return


def declareBuiltIn(st, requestor):
    
    # XXX: where to exit this scope?
    st.enterScope(True) # built-in scope
    
    for pv in pseudoVariables:
        pvDef = newPseudoVariable(pv, st)
        st.insert(pvDef, requestor)

    for bis in builtInSpecifiers:
        declareBuiltInSpecifier(bis, st)

    return



class Checker(object):
    pass



def check(tree, st, requestor):
    
    assert tree.kind == STMT_COMPOUND, "compound statement expected"
    
    predefList = []
    rootClassList = []
    
    checker = Checker()
    checker.st = st
    checker.requestor = requestor
    checker.currentMethod = None
    
    checker.st.enterScope(True) # global scope
    
    # 'Object' is always needed -- see Parser_NewClassDef()
    if declareObject: # Well, almost always.
        declareClass(Object, predefList, checker)

    if declareBuiltInClasses:
        # XXX: What about the other core classes?
        declareClass(Array, predefList, checker)
        declareClass(IdentityDictionary, predefList, checker)
        declareClass(Symbol, predefList, checker)
        for cbr in essentialClassBootRec:
            classVar = heart + cbr.classVarOffset
            declareClass(classVar[00], predefList, checker)
        for cbr in classBootRec:
            classVar = heart + cbr.classVarOffset
            declareClass(classVar[00], predefList, checker)

    checker.rootClassList = rootClassList

    for _pass in range(1, 4):
        for s in tree:
            checkStmt(s, None, checker, _pass)

    dataSize = checker.st.currentScope.context.nDefs

    checker.st.exitScope() # global

    return
