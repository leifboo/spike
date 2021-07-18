

from literals import *
from operators import *
from expressions import *
from statements import *
from spike.il import *
from naming import *


SIZEOF_OBJ_PTR = 8
SIZEOF_CONTEXT_HEADER = 16*SIZEOF_OBJ_PTR
OFFSETOF_ARGUMENT_COUNT = 3*SIZEOF_OBJ_PTR
OFFSETOF_STACK_BASE = 14*SIZEOF_OBJ_PTR


cCallingConventions = (SPEC_CALL_CONV_C, SPEC_CALL_CONV_EXTENSION)


#------------------------------------------------------------------------
# data structures

class Label(object):
    num = 0


class CodeGen(object):
    
    kind = property(lambda self: self.__class__)
    
    def __init__(self, outer = None):
        self.outer = outer
        if outer:
            self.module = outer.module
            self.level = outer.level + 1
            self.out = outer.out
        return


class ModuleCodeGen(CodeGen):
    
    module = property(lambda self: self)
    level = 1
    
    def __init__(self, out):
        super(ModuleCodeGen, self).__init__()
        
        self.out = out
        
        self.nextLabelNum = 1

        self.symbolTable = []

        self.intData = []
        self.floatData = []
        self.charData = set()
        self.strData = []
        self.symData = []

        self.source = None

        return


class ClassCodeGen(CodeGen):

    def __init__(self, outer, classDef):
        super(ClassCodeGen, self).__init__(outer)
        self.classDef = classDef
        return


class OpcodeGen(CodeGen):

    varCount = property(lambda self: self.localCount + self.maxArgumentCount)

    def __init__(self, outer, minArgumentCount, maxArgumentCount, localCount):
        super(OpcodeGen, self).__init__(outer)
        
        self.minArgumentCount = minArgumentCount
        self.maxArgumentCount = maxArgumentCount
        self.localCount = localCount
        
        return


class MethodCodeGen(OpcodeGen):

    home = property(lambda self: self)

    def __init__(self, outer, stmt):
        super(MethodCodeGen, self).__init__(
            outer = outer,
            minArgumentCount = stmt.u.method.minArgumentCount,
            maxArgumentCount = stmt.u.method.maxArgumentCount,
            localCount = stmt.u.method.localCount,
            )
        
        self.varArgList = stmt.varArg is not None
        self.blockCount = stmt.u.method.blockCount
        
        return


class BlockCodeGen(OpcodeGen):

    varArgList = False

    def __init__(self, outer, expr):
        assert outer.kind in (BlockCodeGen, MethodCodeGen), "block not allowed here"
        super(BlockCodeGen, self).__init__(
            outer = outer,
            minArgumentCount = expr.aux.block.argumentCount,
            maxArgumentCount = expr.aux.block.argumentCount,
            localCount = expr.aux.block.localCount,
            )
        self.home = outer.home
        self.blockIndex = expr.u._def.index # XXX: interpreter cruft
        return


#------------------------------------------------------------------------
# C cruft

def fprintf(stream, format, *args):
    output = format % args
    stream.write(output)
    return len(output)


def fprintk(stream, format, **kwds):
    output = format % kwds
    stream.write(output)
    return len(output)


def fputs(s, stream):
    stream.write(s)
    return 0


def fputc(c, stream):
    stream.write(c)
    return c


#------------------------------------------------------------------------
# opcodes

def emitOpcode(cgen, mnemonic, operands = None, *args):
    out = cgen.out
    fprintf(out, "\t%s", mnemonic)
    if operands:
        fputs("\t", out)
        fprintf(out, operands, *args)
    fputs("\n", out)
    return


def getLabel(target, cgen):
    if not target.num:
        target.num = cgen.module.nextLabelNum
        cgen.module.nextLabelNum += 1
    return target.num


def defineLabel(label, cgen):
    if not label.num:
        label.num = cgen.module.nextLabelNum
        cgen.module.nextLabelNum += 1
    return


def maybeEmitLabel(label, cgen):
    if label.num:
        fprintf(cgen.out, ".L%u:\n", label.num)
    return


def emitBranch(opcode, target, cgen):
    if opcode == OPCODE_BRANCH_IF_FALSE:
        emitOpcode(cgen, "call", "SpikeTest")
        emitOpcode(cgen, "jz", ".L%u", getLabel(target, cgen))
    elif opcode == OPCODE_BRANCH_IF_TRUE:
        emitOpcode(cgen, "call", "SpikeTest")
        emitOpcode(cgen, "jnz", ".L%u", getLabel(target, cgen))
    elif opcode == OPCODE_BRANCH_ALWAYS:
        emitOpcode(cgen, "jmp", ".L%u", getLabel(target, cgen))
    return


def dupN(n, cgen):
    for i in range(n):
        emitOpcode(cgen, "push", "%lu(%%rsp)", SIZEOF_OBJ_PTR * (n - 1))
    return


def instVarOffset(_def, cgen):
    return SIZEOF_OBJ_PTR * _def.u._def.index


def offsetOfIndex(index, cgen):
    # vars are stored in reverse order starting at end of home context
    return (
        SIZEOF_CONTEXT_HEADER +
        (cgen.home.varCount - index - 1) * SIZEOF_OBJ_PTR
        )

def localVarOffset(_def, cgen):
    return offsetOfIndex(_def.u._def.index, cgen)


def isVarDef(_def):
    return not hasattr(_def, 'definition') and _def.u._def.stmt and _def.u._def.stmt.kind == STMT_DEF_VAR


def isVarRef(expr):
    return hasattr(expr, 'definition') and isVarDef(expr.u.ref.definition)


def emitCodeForName(expr, super, cgen):
    
    _def = expr.u.ref.definition
    assert _def, "missing definition"
    pushOpcode = _def.u._def.pushOpcode
    
    if _def.u._def.level == 0:
        # built-in
        builtins = {
            OPCODE_PUSH_SELF:    "%%rsi",
            OPCODE_PUSH_SUPER:   "%%rsi",
            OPCODE_PUSH_FALSE:   "$" + mangleExternal('false'),
            OPCODE_PUSH_TRUE:    "$" + mangleExternal('true'),
            OPCODE_PUSH_NULL:    "$0",
            OPCODE_PUSH_VOID:    "$" + mangleExternal('void'),
            OPCODE_PUSH_CONTEXT: "%%rbp",
            }
        builtin = builtins[pushOpcode]
        
        if pushOpcode == OPCODE_PUSH_SUPER:
            assert super, "invalid use of 'super'"
            super[0] = True
        
        emitOpcode(cgen, "pushq", builtin)
        
    else:
        if pushOpcode == OPCODE_PUSH_GLOBAL:
            mode = "" if isVarDef(_def) else "$"
            emitOpcode(cgen, "pushq", "%s%s", mode, mangleExternal(_def.sym))
        elif pushOpcode == OPCODE_PUSH_INST_VAR:
            emitOpcode(cgen, "push", "%d(%%rdi)", instVarOffset(_def, cgen))
        elif pushOpcode == OPCODE_PUSH_LOCAL:
            emitOpcode(cgen, "push", "%d(%%rbx)", localVarOffset(_def, cgen))
        else:
            assert False, "unexpected push opcode"

    return


def emitCodeForLiteral(literal, cgen):
    
    klass = type(literal)
    
    if klass is Symbol:
        willEmitSym(literal, cgen)
        emitOpcode(cgen, "pushq", "$%s%s", symPrefix, mangle(literal))
    
    elif klass is Integer:
        # a negative literal int is currently not possible
        assert literal >= 0, "negative literal int"
        if literal <= 0x1fffffff:
            emitOpcode(cgen, "pushq", "$%ld", (literal << 2) | 0x2)
        else:
            # box
            willEmitInt(literal, cgen)
            emitOpcode(cgen, "pushq", "$%s%ld", intPrefix, literal)
    
    elif klass is Float:
        index = willEmitFloat(literal, cgen)
        emitOpcode(cgen, "pushq", "$%s%u", floatPrefix, index)
    
    elif klass == Character:
        willEmitChar(literal, cgen)
        emitOpcode(cgen, "pushq", "$%s%02x", charPrefix, ord(literal))
    
    elif klass is String:
        index = willEmitStr(literal, cgen)
        emitOpcode(cgen, "pushq", "$%s%u", strPrefix, index)
    
    else:
        assert False, "unknown class of literal: %r" % klass
    
    return


def store(var, cgen):
    _def = var.u.ref.definition
    emitOpcode(cgen, "mov", "(%%rsp), %%rax")
    storeOpcode = _def.u._def.storeOpcode
    if storeOpcode == OPCODE_STORE_GLOBAL:
        assert isVarDef(_def), "invalid lvalue"
        emitOpcode(cgen, "mov", "%%rax, %s", mangleExternal(_def.sym))
    elif storeOpcode == OPCODE_STORE_INST_VAR:
        emitOpcode(cgen, "mov", "%%rax, %d(%%rdi)", instVarOffset(_def, cgen))
    elif storeOpcode == OPCODE_STORE_LOCAL:
        emitOpcode(cgen, "mov", "%%rax, %d(%%rbx)", localVarOffset(_def, cgen))
    return


def pop(cgen):
    emitOpcode(cgen, "pop", "%%rax")
    return


def oper(code, isSuper, cgen):
    emitOpcode(cgen, "call", "SpikeOper%s%s", code.name, "Super" if isSuper else "")
    return


#------------------------------------------------------------------------
# rodata

def willEmitLiteral(value, data, cgen):

    for i, v in enumerate(data):
        if v == value:
            return i

    data.append(value)
    return len(data) - 1


def willEmitInt(value, cgen):
    return willEmitLiteral(value, cgen.module.intData, cgen)


def willEmitFloat(value, cgen):
    return willEmitLiteral(value, cgen.module.floatData, cgen)


def willEmitChar(value, cgen):
    cgen = cgen.module
    cgen.charData.add(value)
    return


def willEmitStr(value, cgen):
    return willEmitLiteral(value, cgen.module.strData, cgen)


def willEmitSym(value, cgen):
    return willEmitLiteral(value, cgen.module.symData, cgen)


substDict = {
    '\a': "\\a",
    '\b': "\\b",
    '\f': "\\f",
    '\n': "\\n",
    '\r': "\\r",
    '\t': "\\t",
    '\v': "\\v",
    '\\': "\\\\",
    '"':  "\\\"",
    }

def escapeStringLiteral(value):
    result = '"'
    for c in value:
        # XXX: numeric escape codes
        subst = substDict.get(c)
        if subst:
            result += subst
        else:
            result += c
    result += '"'
    return result


def mangle(sym):
    # XXX
    sym = sym.replace(' ', '$$')
    sym = sym.replace(':', '$')
    return sym


def mangleExternal(sym):
    return externalPrefix + sym


def emitROData(cgen):

    out = cgen.out

    # Conceptually, these constant objects are read-only data.  But
    # they will have relocations against them, so they are not
    # read-only from the linker's perspective.
    fprintf(out, "\t.data\n")

    # We don't bother unique-ifying ints, floats, chars or strings;
    # instead, they have static linkage.

    fprintf(out, "\t.align\t8\n")
    for value in cgen.intData:
        fprintk(out,
                "%(sym)s:\n"
                "\t.type\t%(sym)s, @object\n"
                "\t.size\t%(sym)s, 16\n"
                "\t.quad\t%(klass)s\n"
                "\t.quad\t%(value)d\n",
                sym = "%s%ld" % (intPrefix, value),
                klass = mangleExternal('Integer'),
                value = value)

    for i, value in enumerate(cgen.floatData):
        # XXX: I would find it comforting if the exact string from
        # the source was emitted here, or something... spitting out
        # "%f" seems fuzzy.
        fprintk(out,
                "\t.align\t16\n"
                "%(sym)s:\n"
                "\t.type\t%(sym)s, @object\n"
                "\t.size\t%(sym)s, 16\n"
                "\t.quad\t%(klass)s\n"
                "\t.double\t%(value)f\n",
                sym = "%s%u" % (floatPrefix, i),
                klass = mangleExternal('Float'),
                value = value)

    fprintf(out, "\t.align\t8\n")
    for value in [ord(c) for c in sorted(list(cgen.charData))]:
        fprintk(out,
                "%(sym)s:\n"
                "\t.type\t%(sym)s, @object\n"
                "\t.size\t%(sym)s, 16\n"
                "\t.quad\t%(klass)s\n"
                "\t.quad\t%(value)u\n",
                sym = "%s%02x" % (charPrefix, value),
                klass = mangleExternal('Char'),
                value = value)

    for i, value in enumerate(cgen.strData):
        fprintk(out,
                "\t.align\t8\n"
                "%(sym)s:\n"
                "\t.type\t%(sym)s, @object\n"
                "\t.quad\t%(klass)s\n"
                "\t.quad\t%(size)u\n"
                "\t.string\t%(value)s\n"
                "\t.size\t%(sym)s, .-%(sym)s\n",
                sym = "%s%u" % (strPrefix, i),
                klass = mangleExternal('String'),
                size = len(value) + 1,
                value = escapeStringLiteral(value))

    # We create a mess of comdat section groups to make Symbols unique
    # within an executable or shared object.  (...and therefore
    # globally unique?)
    for sym in cgen.symData:
        name = mangle(sym)
        fprintk(out,
                '\t.section\tspksym.%(name)s,"aG",@progbits,spksym.%(name)s,comdat\n'
                "\t.align\t8\n"
                "%(sym)s:\n"
                "\t.globl\t%(sym)s\n"
                "\t.type\t%(sym)s, @object\n"
                "\t.quad\t%(klass)s\n"
                "\t.quad\t%(hash)u\n"
                "\t.string\t\"%(value)s\"\n"
                "\t.size\t%(sym)s, .-%(sym)s\n",
                name = name,
                sym = symPrefix + name,
                klass = mangleExternal('Symbol'),
                hash = 0, # XXX
                value = sym)

    fprintf(out, "\n")

    return


#------------------------------------------------------------------------
# spksymtab

def emitSymbolTable(cgen):
    out = cgen.out
    fprintf(out, '\t.section\tspksymtab,"",@progbits\n')
    for kind, name in cgen.module.symbolTable:
        fprintf(out, '\t.string\t"%s %s"\n', kind, name)
    fprintf(out, '\n')
    return
    

#------------------------------------------------------------------------
# expressions

def emitCodeForExpr(expr, super, cgen):

    isSuper = [False]

    if super:
        super[0] = False

    maybeEmitLabel(expr.label, cgen)

    if expr.kind == EXPR_LITERAL:
        emitCodeForLiteral(expr.aux.value, cgen)

    elif expr.kind == EXPR_NAME:
        emitCodeForName(expr, super, cgen)

    elif expr.kind == EXPR_BLOCK:
        emitOpcode(cgen, "push", "%%rbx")
        emitOpcode(cgen, "pushq", "$%lu", expr.aux.block.argumentCount)
        emitOpcode(cgen, "call", "SpikeBlockCopy")
        emitBranch(OPCODE_BRANCH_ALWAYS, expr.endLabel, cgen)
        emitCodeForBlock(expr, cgen)

    elif expr.kind == EXPR_COMMA:
        for e in expr[:-1]:
            emitCodeForExpr(e, None, cgen)
            pop(cgen)
        emitCodeForExpr(expr[-1], super, cgen)

    elif expr.kind == EXPR_COMPOUND:
        emitOpcode(cgen, "push", "%%rbp")
        for arg in expr:
            emitCodeForExpr(arg, None, cgen)
        if expr.var:
            emitCodeForExpr(expr.var, None, cgen)

        argumentCount = len(expr)

        emitOpcode(cgen, "call", "__spike_xxx_compoundExpression") # XXX

    elif expr.kind == EXPR_CALL:
        # evaluate receiver
        if expr.left.kind in (EXPR_ATTR, EXPR_ATTR_VAR):
            emitCodeForExpr(expr.left.left, isSuper, cgen)
        else:
            emitCodeForExpr(expr.left, isSuper, cgen)

        # evaluate arguments
        for arg in expr.fixedArgs:
            emitCodeForExpr(arg, None, cgen)
        if expr.varArg:
            emitCodeForExpr(expr.var, None, cgen)

        argumentCount = len(expr.fixedArgs)

        # evaluate selector
        if expr.left.kind == EXPR_ATTR:
            emitCodeForLiteral(expr.left.attr, cgen)
            emitOpcode(cgen, "pop", "%%rdx")
            routine = "SpikeSendMessage"

        elif expr.left.kind == EXPR_ATTR_VAR:
            emitCodeForExpr(expr.left.right, None, cgen)
            emitOpcode(cgen, "pop", "%%rdx")
            routine = "SpikeSendMessage"

        else:
            routine = {
                OPER_APPLY: "SpikeCall",
                OPER_INDEX: "SpikeGetIndex",
                }[expr.oper]

        if expr.var:
            # push var args
            emitOpcode(cgen, "call", "SpikePushVarArgs")
            emitOpcode(cgen, "add", "$%lu, %%rcx", argumentCount)
        else:
            emitOpcode(cgen, "mov", "$%lu, %%rcx", argumentCount)

        # call RTL routine to send message
        emitOpcode(cgen, "call", "%s%s", routine, "Super" if isSuper[0] else "")

    elif expr.kind == EXPR_ATTR:
        emitCodeForExpr(expr.left, isSuper, cgen)
        emitCodeForLiteral(expr.attr, cgen)
        emitOpcode(cgen, "pop", "%%rdx")
        emitOpcode(cgen, "call", "SpikeGetAttr%s", "Super" if isSuper[0] else "")

    elif expr.kind == EXPR_ATTR_VAR:
        emitCodeForExpr(expr.left, isSuper, cgen)
        emitCodeForExpr(expr.right, None, cgen)
        emitOpcode(cgen, "pop", "%%rdx")
        emitOpcode(cgen, "call", "SpikeGetAttr%s", "Super" if isSuper[0] else "")

    elif expr.kind in (EXPR_PREOP, EXPR_POSTOP):

        if expr.left.kind == EXPR_NAME:
            emitCodeForExpr(expr.left, None, cgen)
            if expr.kind == EXPR_POSTOP:
                dupN(1, cgen)

            oper(expr.oper, False, cgen)
            store(expr.left, cgen)
            if expr.kind == EXPR_POSTOP:
                pop(cgen)

        elif expr.left.kind in (EXPR_ATTR, EXPR_ATTR_VAR):
            inPlaceAttrOp(expr, cgen)

        elif expr.left.kind == EXPR_CALL:
            inPlaceIndexOp(expr, cgen)

        else:
            assert False, "invalid lvalue"

    elif expr.kind == EXPR_UNARY:
        emitCodeForExpr(expr.left, isSuper, cgen)
        oper(expr.oper, isSuper[0], cgen)

    elif expr.kind == EXPR_BINARY:
        emitCodeForExpr(expr.left, isSuper, cgen)
        emitCodeForExpr(expr.right, None, cgen)
        oper(expr.oper, isSuper[0], cgen)

    elif expr.kind == EXPR_ID:
        emitCodeForExpr(expr.left, None, cgen)
        emitCodeForExpr(expr.right, None, cgen)
        pop(cgen)
        emitOpcode(cgen, "cmp", "(%%rsp), %%rax")
        emitOpcode(cgen, "movq", "$%s, (%%rsp)", mangleExternal('false'))
        emitOpcode(cgen, "je" if expr.inverted else "jne", ".L%u", getLabel(expr.endLabel, cgen))
        emitOpcode(cgen, "movq", "$%s, (%%rsp)", mangleExternal('true'))

    elif expr.kind == EXPR_AND:
        emitBranchForExpr(expr.left, False, expr.right.endLabel, expr.right.label, True, cgen)
        emitCodeForExpr(expr.right, None, cgen)

    elif expr.kind == EXPR_OR:
        emitBranchForExpr(expr.left, True, expr.right.endLabel, expr.right.label, True, cgen)
        emitCodeForExpr(expr.right, None, cgen)

    elif expr.kind == EXPR_COND:
        emitBranchForExpr(expr.cond, False, expr.right.label, expr.left.label, False, cgen)
        emitCodeForExpr(expr.left, None, cgen)
        emitBranch(OPCODE_BRANCH_ALWAYS, expr.right.endLabel, cgen)
        emitCodeForExpr(expr.right, None, cgen)

    elif expr.kind == EXPR_KEYWORD:
        emitCodeForExpr(expr.left, isSuper, cgen)
        for arg in expr.fixedArgs:
            emitCodeForExpr(arg, None, cgen)

        argumentCount = len(expr.fixedArgs)

        emitCodeForLiteral(expr.selector, cgen)
        emitOpcode(cgen, "pop", "%%rdx")
        emitOpcode(cgen, "mov", "$%lu, %%rcx", argumentCount)
        emitOpcode(cgen, "call", "SpikeSendMessage%s", "Super" if isSuper[0] else "")

    elif expr.kind == EXPR_ASSIGN:

        if expr.left.kind == EXPR_NAME:
            if expr.oper == OPER_EQ:
                emitCodeForExpr(expr.right, None, cgen)
            else:
                emitCodeForExpr(expr.left, None, cgen)
                emitCodeForExpr(expr.right, None, cgen)
                oper(expr.oper, False, cgen)

            store(expr.left, cgen)

        elif expr.left.kind in (EXPR_ATTR, EXPR_ATTR_VAR):
            inPlaceAttrOp(expr, cgen)

        elif expr.left.kind == EXPR_CALL:
            inPlaceIndexOp(expr, cgen)

        else:
            assert False, "invalid lvalue"

    maybeEmitLabel(expr.endLabel, cgen)

    return


def squirrel(resultDepth, cgen):
    # duplicate the last result
    dupN(1, cgen)
    # squirrel it away for later
    emitOpcode(cgen, "mov", "$%lu, %%rcx", resultDepth + 1)
    emitOpcode(cgen, "call", "SpikeRotate")
    return


def inPlaceOp(expr, resultDepth, cgen):
    if expr.kind == EXPR_ASSIGN:
        emitCodeForExpr(expr.right, None, cgen)
    elif expr.kind == EXPR_POSTOP:
        # e.g., "a[i]++" -- squirrel away the original value
        squirrel(resultDepth, cgen)

    oper(expr.oper, False, cgen)

    if expr.kind != EXPR_POSTOP:
        # e.g., "++a[i]" -- squirrel away the new value
        squirrel(resultDepth, cgen)

    return


def inPlaceAttrOp(expr, cgen):
    argumentCount = 0
    isSuper = [False]

    # get/set common receiver
    emitCodeForExpr(expr.left.left, isSuper, cgen)

    # get/set common attr
    if expr.left.kind == EXPR_ATTR:
        emitCodeForLiteral(expr.left.attr, cgen)
        argumentCount += 1
    elif expr.left.kind == EXPR_ATTR_VAR:
        emitCodeForExpr(expr.left.right, None, cgen)
        argumentCount += 1
    else:
        assert False, "invalid lvalue"

    # rhs

    if expr.oper == OPER_EQ:
        emitCodeForExpr(expr.right, None, cgen)
        squirrel(1 + argumentCount + 1, # receiver, args, new value
                 cgen)
    else:
        dupN(argumentCount + 1, cgen)

        if expr.left.kind in (EXPR_ATTR, EXPR_ATTR_VAR):
            emitOpcode(cgen, "pop", "%%rdx")
            emitOpcode(cgen, "call", "SpikeGetAttr%s", "Super" if isSuper[0] else "")
        else:
            assert False, "invalid lvalue"

        inPlaceOp(expr, 1 + argumentCount + 1, # receiver, args, result
                  cgen)


    if expr.left.kind in (EXPR_ATTR, EXPR_ATTR_VAR):
        # XXX: re-think this whole thing
        emitOpcode(cgen, "pop", "%%rax")
        emitOpcode(cgen, "pop", "%%rdx")
        emitOpcode(cgen, "push", "%%rax")
        emitOpcode(cgen, "call", "SpikeSetAttr%s", "Super" if isSuper[0] else "")

    else:
        assert False, "invalid lvalue"

    # discard 'set' method result, exposing the value that was
    # squirrelled away
    pop(cgen)

    return


def inPlaceIndexOp(expr, cgen):
    isSuper = [False]

    assert expr.left.oper == OPER_INDEX, "invalid lvalue"

    # get/set common receiver
    emitCodeForExpr(expr.left.left, isSuper, cgen)

    # get/set common arguments
    for arg in expr.left.fixedArgs:
        emitCodeForExpr(arg, None, cgen)

    # rhs

    argumentCount = len(expr.left.fixedArgs)

    if expr.oper == OPER_EQ:
        emitCodeForExpr(expr.right, None, cgen)
        squirrel(1 + argumentCount + 1, # receiver, args, new value
                 cgen)
    else:
        # get __index__ {
        dupN(argumentCount + 1, cgen)
        emitOpcode(cgen, "mov", "$%lu, %%rcx", argumentCount)
        if expr.left.oper == OPER_APPLY:
            emitOpcode(cgen, "call", "SpikeCall%s", "Super" if isSuper[0] else "")
        elif expr.left.oper == OPER_INDEX:
            emitOpcode(cgen, "call", "SpikeGetIndex%s", "Super" if isSuper[0] else "")
        else:
            assert False, "bad operator"
        # } get __index__

        inPlaceOp(expr, 1 + argumentCount + 1,  # receiver, args, result
                  cgen)

    argumentCount += 1 # new item value
    emitOpcode(cgen, "mov", "$%lu, %%rcx", argumentCount)
    emitOpcode(cgen, "call", "SpikeSetIndex%s", "Super" if isSuper[0] else "")

    # discard 'set' method result, exposing the value that was
    # squirrelled away
    pop(cgen)

    return


def emitBranchForNonControlExpr(expr, cond, label, dup, cgen):

    emitCodeForExpr(expr, None, cgen)

    # XXX: This sequence could be replaced by a special set of
    # branch-or-pop opcodes.
    if dup:
        dupN(1, cgen)
    emitBranch(OPCODE_BRANCH_IF_TRUE if cond else OPCODE_BRANCH_IF_FALSE, label, cgen)
    if dup:
        pop(cgen)

    return


def emitBranchForExpr(expr, cond, label, fallThroughLabel, dup, cgen):

    if expr.kind == EXPR_NAME:
        pushOpcode = expr.u.ref.definition.u._def.pushOpcode
        if pushOpcode in (OPCODE_PUSH_FALSE, OPCODE_PUSH_TRUE):
            killCode = cond if pushOpcode == OPCODE_PUSH_TRUE else not cond
            maybeEmitLabel(expr.label, cgen)
            if killCode:
                if dup:
                    emitOpcode(cgen, "pushq", "$%s",
                               mangleExternal('true' if pushOpcode == OPCODE_PUSH_TRUE else 'false'))

                emitBranch(OPCODE_BRANCH_ALWAYS, label, cgen)

            maybeEmitLabel(expr.endLabel, cgen)
        else:
            emitBranchForNonControlExpr(expr, cond, label, dup, cgen)

    elif expr.kind == EXPR_COMMA:
        for e in expr[:-1]:
            emitCodeForExpr(e, None, cgen)
            # XXX: We could elide this 'pop' if 'e' is a conditional expr.
            pop(cgen)
        emitBranchForExpr(expr[-1], cond, label, fallThroughLabel, dup, cgen)

    elif expr.kind == EXPR_AND:
        maybeEmitLabel(expr.label, cgen)
        if cond:
            # branch if true
            emitBranchForExpr(expr.left, False, fallThroughLabel, expr.right.label, False, cgen)
            emitBranchForExpr(expr.right, True, label, fallThroughLabel, dup, cgen)
        else:
            # branch if false
            emitBranchForExpr(expr.left, False, label, expr.right.label, dup, cgen)
            emitBranchForExpr(expr.right, False, label, fallThroughLabel, dup, cgen)

        maybeEmitLabel(expr.endLabel, cgen)

    elif expr.kind == EXPR_OR:
        maybeEmitLabel(expr.label, cgen)
        if cond:
            # branch if true
            emitBranchForExpr(expr.left, True, label, expr.right.label, dup, cgen)
            emitBranchForExpr(expr.right, True, label, fallThroughLabel, dup, cgen)
        else:
            # branch if false
            emitBranchForExpr(expr.left, True, fallThroughLabel, expr.right.label, False, cgen)
            emitBranchForExpr(expr.right, False, label, fallThroughLabel, dup, cgen)

        maybeEmitLabel(expr.endLabel, cgen)

    elif expr.kind == EXPR_COND:
        maybeEmitLabel(expr.label, cgen)
        emitBranchForExpr(expr.cond, False, expr.right.label, expr.left.label, False, cgen)
        emitBranchForExpr(expr.left, cond, label, fallThroughLabel, dup, cgen)
        emitBranch(OPCODE_BRANCH_ALWAYS, fallThroughLabel, cgen)
        emitBranchForExpr(expr.right, cond, label, fallThroughLabel, dup, cgen)
        maybeEmitLabel(expr.endLabel, cgen)

    else:
        emitBranchForNonControlExpr(expr, cond, label, dup, cgen)

    return


def popBlockArgs(cgen):
    argumentCount = cgen.maxArgumentCount
    if argumentCount:
        # store args into local variables
        for arg in reversed(xrange(argumentCount)):
            index = cgen.blockIndex + arg # XXX: interpreter cruft
            emitOpcode(cgen, "popq", "%d(%%rbx)", offsetOfIndex(index, cgen))
    # pop receiver
    pop(cgen)
    return


def emitCodeForBlock(expr, outer):

    sentinel = Yield(expr.right)

    # push block code generator
    cgen = BlockCodeGen(outer, expr)

    #
    # prologue
    #

    popBlockArgs(cgen)

    start = Label()
    defineLabel(start, cgen)
    maybeEmitLabel(start, cgen)

    #
    # body
    #
    
    body = CompoundStmt(expr.aux.block.stmtList) # XXX ?
    emitCodeForStmt(body, sentinel.label, None, None, cgen)

    #
    # epilogue
    #
    
    emitCodeForStmt(sentinel, None, None, None, cgen)

    # upon resume, loop
    emitBranch(OPCODE_BRANCH_ALWAYS, start, cgen)

    return


def emitCodeForInitializer(expr, cgen):

    maybeEmitLabel(expr.label, cgen)

    emitCodeForExpr(expr.right, None, cgen)

    # similar to store(), but with a definition instead of a
    # reference
    _def = expr.left
    assert _def.kind == EXPR_NAME, "name expected"
    assert _def.u._def.storeOpcode == OPCODE_STORE_LOCAL, "local variable expected"
    pop(cgen)
    emitOpcode(cgen, "mov", "%%rax, %d(%%rbx)", localVarOffset(_def, cgen))

    maybeEmitLabel(expr.endLabel, cgen)

    return


#------------------------------------------------------------------------
# statements

def emitCodeForStmt(stmt, nextLabel, breakLabel, continueLabel, cgen):

    maybeEmitLabel(stmt.label, cgen)

    if stmt.kind == STMT_BREAK:
        assert breakLabel, "break not allowed here"
        emitBranch(OPCODE_BRANCH_ALWAYS, breakLabel, cgen)

    elif stmt.kind == STMT_COMPOUND:
        for s, childNextLabel in stmt.iterWithLabels(nextLabel):
            emitCodeForStmt(s, childNextLabel, breakLabel, continueLabel, cgen)

    elif stmt.kind == STMT_CONTINUE:
        assert continueLabel, "continue not allowed here"
        emitBranch(OPCODE_BRANCH_ALWAYS, continueLabel, cgen)

    elif stmt.kind in (STMT_DEF_METHOD, STMT_DEF_CLASS):
        pass

    elif stmt.kind == STMT_DEF_VAR:
        for expr in stmt.defList:
            if expr.kind == EXPR_ASSIGN:
                emitCodeForInitializer(expr, cgen)

    elif stmt.kind == STMT_DO_WHILE:
        childNextLabel = stmt.expr.label
        defineLabel(stmt.top.label, cgen)
        emitCodeForStmt(stmt.top, childNextLabel, nextLabel, childNextLabel, cgen)
        emitBranchForExpr(stmt.expr, True, stmt.top.label, nextLabel, False, cgen)

    elif stmt.kind == STMT_EXPR:
        if stmt.expr:
            emitCodeForExpr(stmt.expr, None, cgen)
            pop(cgen)

    elif stmt.kind == STMT_FOR:
        if stmt.incr:
            childNextLabel = stmt.incr.label
        elif stmt.test:
            childNextLabel = stmt.test.label
        else:
            childNextLabel = stmt.top.label

        if stmt.init:
            emitCodeForExpr(stmt.init, None, cgen)
            pop(cgen)

        if stmt.test:
            emitBranch(OPCODE_BRANCH_ALWAYS, stmt.test.label, cgen)

        defineLabel(stmt.top.label, cgen)
        emitCodeForStmt(stmt.top, childNextLabel, nextLabel, childNextLabel, cgen)
        if stmt.incr:
            emitCodeForExpr(stmt.incr, None, cgen)
            pop(cgen)

        if stmt.test:
            emitBranchForExpr(stmt.test, True, stmt.top.label, nextLabel, False, cgen)
        else:
            emitBranch(OPCODE_BRANCH_ALWAYS, stmt.top.label, cgen)

    elif stmt.kind == STMT_IF_ELSE:
        elseLabel = stmt.bottom.label if stmt.bottom else nextLabel
        emitBranchForExpr(stmt.expr, False, elseLabel, stmt.top.label, False, cgen)
        emitCodeForStmt(stmt.top, nextLabel, breakLabel, continueLabel, cgen)
        if stmt.bottom:
            emitBranch(OPCODE_BRANCH_ALWAYS, nextLabel, cgen)
            emitCodeForStmt(stmt.bottom, nextLabel, breakLabel, continueLabel, cgen)

    elif stmt.kind == STMT_PRAGMA_SOURCE:
        pass

    elif stmt.kind == STMT_RETURN:
        # evaluate result
        if stmt.expr:
            emitCodeForExpr(stmt.expr, None, cgen)
        elif inClass(cgen):
            # note that "return" inside a block closure is undefined
            # anyway
            emitOpcode(cgen, "push", "%%rsi") # self
        else:
            # naked function
            emitOpcode(cgen, "pushq", "$%s", mangleExternal('void'))
        
        # store result
        if cgen.varArgList:
            emitOpcode(cgen, "mov", "%d(%%rbx), %%rcx", OFFSETOF_ARGUMENT_COUNT)
            emitOpcode(cgen, "popq", "%d(%%rbx,%%rcx,8)",
                       SIZEOF_CONTEXT_HEADER + cgen.home.localCount * SIZEOF_OBJ_PTR
                       )
        else:
            emitOpcode(cgen, "popq", "%d(%%rbx)", offsetOfIndex(-1, cgen))
        
        if cgen.kind == BlockCodeGen:
            # resume home context
            emitOpcode(cgen, "mov", "%d(%%rbx), %%rsp", OFFSETOF_STACK_BASE) # reset stack pointer
            emitOpcode(cgen, "mov", "%%rbx, %%rbp") # activeContext = homeContext

        # return to SpikeEpilogue
        emitOpcode(cgen, "ret")

    elif stmt.kind == STMT_WHILE:
        childNextLabel = stmt.expr.label
        emitBranch(OPCODE_BRANCH_ALWAYS, stmt.expr.label, cgen)
        defineLabel(stmt.top.label, cgen)
        emitCodeForStmt(stmt.top, childNextLabel, nextLabel, childNextLabel, cgen)
        emitBranchForExpr(stmt.expr, True, stmt.top.label, nextLabel, False, cgen)

    elif stmt.kind == STMT_YIELD:
        if stmt.expr:
            emitCodeForExpr(stmt.expr, None, cgen)
        else:
            emitOpcode(cgen, "pushq", "$%s", mangleExternal('void'))

        # save our %rip in BlockContext.pc and resume caller
        emitOpcode(cgen, "call", "SpikeYield")

        # upon return, pop arguments anew
        popBlockArgs(cgen)

    else:
        assert False, "unexpected statement node: %r" % stmt

    return


#------------------------------------------------------------------------
# methods

def emitCodeForArgList(stmt, cgen):

    if cgen.varArgList:
        emitOpcode(cgen, "mov", "%%rbp, %%rdi")
        emitOpcode(cgen, "call", "SpikeMoveVarArgs")
        emitOpcode(cgen, "mov", "104(%%rbp), %%rdi")


    if cgen.minArgumentCount < cgen.maxArgumentCount:
        # generate code for default argument initializers

        out = cgen.out

        for i, arg in enumerate(stmt.fixedArgs):
            if arg.kind == EXPR_ASSIGN:
                optionalArgList = stmt.fixedArgs[i:]
                break
        else:
            assert False, "argument initializer expected"

        skip = Label()
        table = Label()
        defineLabel(skip, cgen)
        defineLabel(table, cgen)

        # load argumentCount
        emitOpcode(cgen, "mov", "%d(%%rbx), %%rcx", OFFSETOF_ARGUMENT_COUNT)

        if cgen.varArgList:
            # range check
            emitOpcode(cgen, "cmp", "$%lu, %%rcx", cgen.maxArgumentCount)
            emitOpcode(cgen, "jae", ".L%u", getLabel(skip, cgen))

        # switch jump
        emitOpcode(cgen, "sub", "$%lu, %%rcx", cgen.minArgumentCount)
        emitOpcode(cgen, "shl", "$3, %%rcx") # convert to byte offset
        emitOpcode(cgen, "mov", ".L%u(%%rcx), %%rax", getLabel(table, cgen))
        emitOpcode(cgen, "jmp", "*%%rax")

        # switch table
        fprintf(out, "\t.section\t.rodata\n")
        fprintf(out, "\t.align\t8\n")
        maybeEmitLabel(table, cgen)
        tally = 0
        for arg in optionalArgList:
            assert arg.kind == EXPR_ASSIGN, "assignment expected"
            fprintf(out, "\t.quad\t.L%u\n", getLabel(arg.label, cgen))
            tally += 1
        if not cgen.varArgList:
            fprintf(out, "\t.quad\t.L%u\n", getLabel(skip, cgen))
        fprintf(out, "\t.text\n")

        assert tally == (cgen.maxArgumentCount - cgen.minArgumentCount), "wrong number of default arguments"

        # assignments
        for arg in optionalArgList:
            emitCodeForInitializer(arg, cgen)

        maybeEmitLabel(skip, cgen)

    return


def emitCodeForMethod(stmt, meta, outer):

    selector = stmt.u.method.name
    sentinel = Return(None)

    # push method code generator
    cgen = MethodCodeGen(outer, stmt)

    body = stmt.body
    assert body.kind == STMT_COMPOUND, "compound statement expected"

    out = cgen.out

    functionName = mangle(selector)

    if outer.kind == ClassCodeGen:
        codeObjectClass = "Method"
        className = outer.classDef.expr.sym
        suffix = ".class" if meta else ""
        ns = {
            METHOD_NAMESPACE_RVALUE: ".0.",
            METHOD_NAMESPACE_LVALUE: ".1.",
            }[stmt.u.method.ns]
        sym = className + suffix + ns + functionName
        codeSym = sym + ".code"
        linkage = ""

    else:
        codeObjectClass = "Function"
        sym = mangleExternal(functionName)
        codeSym = codePrefix + functionName
        linkage = "\t.globl\t%(sym)s\n"

    fprintf(out, "\t.text\n")
    fprintk(out,
            "\t.align\t8\n"
            "%(sym)s:\n"
            + linkage +
            "\t.type\t%(sym)s, @object\n"
            "\t.size\t%(sym)s, 32\n"
            "\t.quad\t%(klass)s\n"
            "\t.quad\t%(minArgumentCount)u\n"
            "\t.quad\t%(maxArgumentCount)u\n"
            "\t.quad\t%(localCount)u\n",
            sym = sym,
            klass = mangleExternal(codeObjectClass),
            minArgumentCount = cgen.minArgumentCount,
            maxArgumentCount = cgen.maxArgumentCount + (0x80000000 if cgen.varArgList else 0),
            localCount = cgen.localCount)
    fprintk(out,
            "%(sym)s:\n"
            "\t.type\t%(sym)s, @function\n",
            sym = codeSym)

    emitCodeForArgList(stmt, cgen)
    emitCodeForStmt(body, sentinel.label, None, None, cgen)
    emitCodeForStmt(sentinel, None, None, None, cgen)

    fprintk(out,
            "\t.size\t%(sym)s, .-%(sym)s\n"
            "\n",
            sym = codeSym)

    for s in body:
        if s.kind in (STMT_DEF_CLASS, STMT_DEF_METHOD):
            assert False, "nested class/method not supported by x86 backend"

    if outer.kind != ClassCodeGen:
        cgen.module.symbolTable.append(('F', functionName))

    return


def emitCFunction(stmt, cgen):

    assert cgen.level == 1, "C functions must be global"

    out = cgen.out

    name = stmt.u.method.name
    sym = mangleExternal(name)

    fprintk(out,
            "\t.data\n"
            "\t.align\t8\n"
            "%(sym)s:\n"
            "\t.globl\t%(sym)s\n"
            "\t.type\t%(sym)s, @object\n",
            sym = sym)

    t = stmt.decl.specifiers & SPEC_TYPE
    cc = stmt.decl.specifiers & SPEC_CALL_CONV

    assert t == SPEC_TYPE_OBJ or cc == SPEC_CALL_CONV_C, "extension functions must have 'obj' return type"

    # Currently, the "signature" is simply the return type (used for
    # boxing).
    signature = {
        0:               "Object",
        SPEC_TYPE_OBJ:   "Object",
        SPEC_TYPE_INT:   "Integer",
        SPEC_TYPE_CHAR:  "Char",
        }[t]

    klass = {
        SPEC_CALL_CONV_C: 'CFunction',
        SPEC_CALL_CONV_EXTENSION: 'XFunction',
        }[cc]

    fprintf(out,
            "\t.quad\t%s\n" # klass
            "\t.quad\t%s\n" # signature
            "\t.quad\t%s\n", # pointer
            mangleExternal(klass),
            mangleExternal(signature),
            name)

    fprintk(out,
            "\t.size\t%(sym)s, .-%(sym)s\n",
            sym = sym)
    fprintf(out, "\n")

    cgen.module.symbolTable.append(('F', name))

    return


#------------------------------------------------------------------------
# class and file scope

def emitCodeForCompound(body, meta, cgen):

    assert body.kind == STMT_COMPOUND, "compound statement expected"

    for s in body:

        if s.kind == STMT_DEF_METHOD:
            if (s.decl.specifiers & SPEC_STORAGE) == SPEC_STORAGE_EXTERN:
                if (s.decl.specifiers & SPEC_CALL_CONV) in cCallingConventions:
                    emitCFunction(s, cgen)
            else:
                emitCodeForMethod(s, meta, cgen)

        elif s.kind == STMT_DEF_VAR:
            if cgen.level == 1:
                # global variable definition

                out = cgen.out

                fprintf(out, "\t.bss\n")
                fprintf(out, "\t.align\t8\n")

                for expr in s.defList:
                    assert expr.kind == EXPR_NAME, "initializers not allowed here"
                    if (expr.specifiers & SPEC_STORAGE) == SPEC_STORAGE_EXTERN:
                        continue
                    fprintk(out,
                            "%(sym)s:\n"
                            "\t.globl\t%(sym)s\n"
                            "\t.type\t%(sym)s, @object\n"
                            "\t.size\t%(sym)s, 8\n",
                            sym = mangleExternal(expr.sym))
                    fprintf(out, "\t.zero\t8\n")

                    cgen.module.symbolTable.append(('v', expr.sym))

                fprintf(out, "\n")

            else:
                for expr in s.defList:
                    assert expr.kind == EXPR_NAME, "initializers not allowed here"

        elif s.kind == STMT_DEF_CLASS:
            emitCodeForClass(s, cgen)

        elif s.kind == STMT_PRAGMA_SOURCE:
            cgen.module.source = s.pathname

        else:
            assert False, "executable code not allowed here"

    return


#------------------------------------------------------------------------
# classes

def mangleMeta(sym):
    return metaPrefix + sym


def methodTableSym(sym, suffix, ns):
    return "%s%s.methodTable.%d" % (sym, suffix, ns)


def emitCodeForBehaviorObject(classDef, meta, cgen):
    body = classDef.bottom if meta else classDef.top
    out = cgen.out
    name = classDef.expr.sym

    if classDef.u.klass.superclassName.u.ref.definition.u._def.pushOpcode == OPCODE_PUSH_NULL:
        superName = None # Class 'Object' has no superclass.
    else:
        superName = classDef.u.klass.superclassName.sym

    if meta:
        suffix = ".class"
        sym = mangleMeta(name)
        superSym = mangleMeta(superName) if superName else None
    else:
        suffix = ""
        sym = mangleExternal(name)
        superSym = mangleExternal(superName) if superName else None

    methodTally = [0 for ns in range(NUM_METHOD_NAMESPACES)]
    for ns in range(NUM_METHOD_NAMESPACES):
        if body is None:
            continue
        for s in body:
            if s.kind == STMT_DEF_METHOD and s.u.method.ns == ns:
                methodTally[ns] += 1

    fprintk(out,
            "\t.data\n"
            "\t.align\t8\n"
            "%(sym)s:\n"
            "\t.globl\t%(sym)s\n"
            "\t.type\t%(sym)s, @object\n",
            sym = sym)

    if meta:
        fprintf(out, "\t.quad\t%s\n", mangleExternal('Metaclass')) # klass
    else:
        fprintf(out, "\t.quad\t%s\n", mangleMeta(name)) # klass

    if superSym: # The metaclass hierarchy mirrors the class hierarchy.
        fprintf(out, "\t.quad\t%s\t/* superclass */\n", superSym) # superclass
    elif meta: # The metaclass of 'Object' is a subclass of 'Class'.
        fprintf(out, "\t.quad\t%s\t/* superclass */\n", mangleExternal('Class')) # superclass
    else: # Class 'Object' has no superclass.
        fprintf(out, "\t.quad\t0\t/* superclass */\n") # superclass

    # XXX: hash at compile time
    for ns in range(NUM_METHOD_NAMESPACES):
        if methodTally[ns]:
            fprintf(out,
                    "\t.quad\t%s\t/* methodTable[%d] */\t\n",
                    methodTableSym(name, suffix, ns), ns) # methodTable[ns]
        else:
            fprintf(out,
                    "\t.quad\t0\t/* methodTable[%d] */\t\n",
                    ns) # methodTable[ns]

    # instVarCount
    fprintf(out,
            "\t.quad\t%lu\t/* instVarCount */\n",
            classDef.u.klass.classVarCount if meta else classDef.u.klass.instVarCount)

    if meta:
        fprintf(out, "\t.quad\t%s\t/* thisClass */\n", mangleExternal(name)) # thisClass
    else:
        willEmitSym(classDef.expr.sym, cgen)
        fprintf(out, "\t.quad\t%s%s\t/* name */\n", symPrefix, name) # name

    # XXX: Class objects need slots for class variables up the
    # superclass chain.  To be totally consistent, and robust with
    # dynamic linking (opaque superclasses), class objects would have
    # to be created at runtime.
    if not meta:
        fprintf(out, "\t.quad\t0,0,0,0,0,0,0,0\t/* XXX class variables */\n")

    fprintk(out,
            "\t.size\t%(sym)s, .-%(sym)s\n",
            sym = sym)

    # A method table is simply an Array of symbol/method key/value pairs.

    for ns in range(NUM_METHOD_NAMESPACES):
        if not methodTally[ns]:
            continue

        mts = methodTableSym(name, suffix, ns)

        fprintk(out,
                "%(mts)s:\n"
                "\t.type\t%(mts)s, @object\n",
                mts = mts)

        fprintf(out, "\t.quad\t%s\n", mangleExternal('Array'))
        fprintf(out, "\t.quad\t%d\n", 2 * methodTally[ns])

        for s in body:
            if s.kind == STMT_DEF_METHOD and s.u.method.ns == ns:
                selector = s.u.method.name
                willEmitSym(selector, cgen)
                selector = mangle(selector)
                fprintf(out,
                        "\t.quad\t%s%s, %s%s.%d.%s\n",
                        symPrefix, selector,
                        name, suffix, ns, selector)

        fprintk(out,
                "\t.size\t%(mts)s, .-%(mts)s\n",
                mts = mts)

    fprintf(out, "\n")

    return


def emitCodeForClass(stmt, outer):

    # push class code generator
    cgen = ClassCodeGen(outer, stmt)

    emitCodeForBehaviorObject(stmt, False, cgen)
    emitCodeForBehaviorObject(stmt, True, cgen)
    if stmt.bottom:
        emitCodeForCompound(stmt.bottom, True, cgen)
    emitCodeForCompound(stmt.top, False, cgen)

    cgen.module.symbolTable.append(('C', stmt.expr.sym))

    return


def inClass(cgen):
    while cgen:
        if cgen.kind == ClassCodeGen:
            return True
        cgen = cgen.outer
    return False


#------------------------------------------------------------------------
# main entry point

def generateCode(tree, out):

    assert tree.kind == STMT_COMPOUND, "compound statement expected"

    cgen = ModuleCodeGen(out)

    # Generate code.
    emitCodeForCompound(tree, False, cgen)

    emitROData(cgen)
    emitSymbolTable(cgen)

    return

