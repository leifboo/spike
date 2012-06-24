

from literals import *
from operators import *
from expressions import *
from statements import *
from spike.il import *


SIZEOF_OBJ_PTR = 4
OFFSETOF_CONTEXT_VAR = 24


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

    def __init__(self, outer, minArgumentCount, maxArgumentCount, localCount):
        super(OpcodeGen, self).__init__(outer)
        
        self.minArgumentCount = minArgumentCount
        self.maxArgumentCount = maxArgumentCount
        self.localCount = localCount + int(self.varArgList)
        
        self.epilogueLabel = Label()

        return


class MethodCodeGen(OpcodeGen):

    method = property(lambda self: self)

    def __init__(self, outer, stmt):
        
        self.varArgList = stmt.varArg is not None
        
        super(MethodCodeGen, self).__init__(
            outer = outer,
            minArgumentCount = stmt.u.method.minArgumentCount,
            maxArgumentCount = stmt.u.method.maxArgumentCount,
            localCount = stmt.u.method.localCount,
            )
        
        self.blockCount = stmt.u.method.blockCount
        
        return


class BlockCodeGen(OpcodeGen):

    varArgList = False
    blockCount = 1 # trigger alternate %ebp mapping

    def __init__(self, outer, expr):
        assert outer.kind in (BlockCodeGen, MethodCodeGen), "block not allowed here"
        super(BlockCodeGen, self).__init__(
            outer = outer,
            minArgumentCount = expr.aux.block.argumentCount,
            maxArgumentCount = expr.aux.block.argumentCount,
            localCount = expr.aux.block.localCount,
            )
        self.method = outer.method
        return


#------------------------------------------------------------------------
# C cruft

def fprintf(stream, format, *args):
    output = format % args
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

def emitOpcode(cgen, mnemonic, operands, *args):
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
        emitOpcode(cgen, "pushl", "%lu(%%esp)", SIZEOF_OBJ_PTR * (n - 1))
    return


def instVarOffset(_def, cgen):
    return SIZEOF_OBJ_PTR * _def.u._def.index


def offsetOfIndex(index, cgen):
    
    if cgen.blockCount:
        # all vars are in MethodContext on heap at %ebp
        return OFFSETOF_CONTEXT_VAR + SIZEOF_OBJ_PTR * index

    if cgen.varArgList:
        # all vars are below %ebp
        # skip saved edx, edi, esi, ebx
        return -(SIZEOF_OBJ_PTR * (index + 1 + 4))

    if index < cgen.maxArgumentCount:
        # argument
        # args are pushed in order, so they are reversed in memory
        # skip saved ebp, eip
        return SIZEOF_OBJ_PTR * (cgen.maxArgumentCount - index - 1 + 2)

    # ordinary local variable
    # skip saved edi, esi, ebx
    index -= cgen.maxArgumentCount
    return -(SIZEOF_OBJ_PTR * (index + 1 + 3))


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
            OPCODE_PUSH_SELF:  "%%esi",
            OPCODE_PUSH_SUPER: "%%esi",
            OPCODE_PUSH_FALSE: "$false",
            OPCODE_PUSH_TRUE:  "$true",
            OPCODE_PUSH_NULL:  "$0",
            OPCODE_PUSH_VOID:  "$void",
            }
        builtin = builtins[pushOpcode]
        
        if pushOpcode == OPCODE_PUSH_SUPER:
            assert super, "invalid use of 'super'"
            super[0] = True
        
        emitOpcode(cgen, "pushl", builtin)
        
    else:
        if pushOpcode == OPCODE_PUSH_GLOBAL:
            operands = "%s%s" if isVarDef(_def) else "$%s%s"
            suffix = ".thunk" if ((_def.specifiers & SPEC_STORAGE) == SPEC_STORAGE_EXTERN) and ((_def.specifiers & SPEC_CALL_CONV) == SPEC_CALL_CONV_C) else ""
            emitOpcode(cgen, "pushl", operands, _def.sym, suffix)
        elif pushOpcode == OPCODE_PUSH_INST_VAR:
            emitOpcode(cgen, "pushl", "%d(%%edi)", instVarOffset(_def, cgen))
        elif pushOpcode == OPCODE_PUSH_LOCAL:
            emitOpcode(cgen, "pushl", "%d(%%ebp)", localVarOffset(_def, cgen))
        else:
            assert False, "unexpected push opcode"

    return


def emitCodeForLiteral(literal, cgen):
    
    klass = type(literal)
    
    if klass is Symbol:
        willEmitSym(literal, cgen)
        emitOpcode(cgen, "pushl", "$__sym_%s", mangle(literal))
    
    elif klass is Integer:
        # a negative literal int is currently not possible
        assert literal >= 0, "negative literal int"
        if literal <= 0x1fffffff:
            emitOpcode(cgen, "pushl", "$%ld", (literal << 2) | 0x2)
        else:
            # box
            willEmitInt(literal, cgen)
            emitOpcode(cgen, "pushl", "$__int_%ld", literal)
    
    elif klass is Float:
        index = willEmitFloat(literal, cgen)
        emitOpcode(cgen, "pushl", "$__float_%u", index)
    
    elif klass == Character:
        willEmitChar(literal, cgen)
        emitOpcode(cgen, "pushl", "$__char_%02x", ord(literal))
    
    elif klass is String:
        index = willEmitStr(literal, cgen)
        emitOpcode(cgen, "pushl", "$__str_%u", index)
    
    else:
        assert False, "unknown class of literal: %r" % klass
    
    return


def store(var, cgen):
    _def = var.u.ref.definition
    emitOpcode(cgen, "movl", "(%%esp), %%eax")
    storeOpcode = _def.u._def.storeOpcode
    if storeOpcode == OPCODE_STORE_GLOBAL:
        assert isVarDef(_def), "invalid lvalue"
        emitOpcode(cgen, "movl", "%%eax, %s", _def.sym)
    elif storeOpcode == OPCODE_STORE_INST_VAR:
        emitOpcode(cgen, "movl", "%%eax, %d(%%edi)", instVarOffset(_def, cgen))
    elif storeOpcode == OPCODE_STORE_LOCAL:
        emitOpcode(cgen, "movl", "%%eax, %d(%%ebp)", localVarOffset(_def, cgen))
    return


def pop(cgen):
    emitOpcode(cgen, "popl", "%%eax")
    return


def prologue(cgen):

    if not cgen.varArgList and not cgen.blockCount:
        # nothing to do -- shared prologue did all the work
        return

    # discard local variable area
    emitOpcode(cgen, "addl", "$%lu, %%esp", SIZEOF_OBJ_PTR * cgen.localCount)

    # save argument count for epilogue
    emitOpcode(cgen, "pushl", "%%edx")
    
    if cgen.blockCount:
        # XXX: push Method pointer ?
        emitOpcode(cgen, "pushl", "%%esp") # stackp
        emitOpcode(cgen, "pushl", "%%edi") # instVarPointer
        emitOpcode(cgen, "pushl", "%%esi") # receiver
        emitOpcode(cgen, "pushl", "%%ebx") # methodClass
        emitOpcode(cgen, "leal", "8(%%ebp,%%edx,4), %%eax") # pointer to first arg
        emitOpcode(cgen, "pushl", "%%eax")
        emitOpcode(cgen, "pushl", "%%edx") # actual argument count
        emitOpcode(cgen, "pushl", "$%lu", cgen.localCount)
        emitOpcode(cgen, "pushl", "$%d", cgen.varArgList)
        emitOpcode(cgen, "pushl", "$%lu", cgen.maxArgumentCount)
        emitOpcode(cgen, "pushl", "$%lu", cgen.minArgumentCount)
        emitOpcode(cgen, "call", "SpikeCreateMethodContext")
        emitOpcode(cgen, "addl", "$%lu, %%esp", SIZEOF_OBJ_PTR * 10)
        emitOpcode(cgen, "movl", "%%eax, %%ebp")
        # pointer to new, heap-allocated, initialized MethodContext
        # object is now in %ebp
        return

    if cgen.maxArgumentCount:
        # copy fixed arguments
        emitOpcode(cgen, "movl", "$%lu, %%ecx", cgen.maxArgumentCount)
        loop = Label()
        defineLabel(loop, cgen)
        emitOpcode(cgen, "cmpl", "$%lu, %%edx", cgen.maxArgumentCount)
        emitOpcode(cgen, "jae", ".L%u", getLabel(loop, cgen))
        emitOpcode(cgen, "movl", "$%lu, %%edx", cgen.maxArgumentCount)
        maybeEmitLabel(loop, cgen)
        emitOpcode(cgen, "pushl", "4(%%ebp,%%edx,4)")
        emitOpcode(cgen, "decl", "%%edx")
        emitOpcode(cgen, "loop", ".L%u", getLabel(loop, cgen))

    # create var arg array
    skip = Label()
    defineLabel(skip, cgen)
    emitOpcode(cgen, "pushl", "$Array")
    emitOpcode(cgen, "leal", "8(%%ebp), %%eax")
    emitOpcode(cgen, "orl", "$3, %%eax") # map to CObject
    emitOpcode(cgen, "pushl", "%%eax")
    emitOpcode(cgen, "movl", "-16(%%ebp), %%eax") # compute excess arg count
    emitOpcode(cgen, "subl", "$%lu, %%eax", cgen.maxArgumentCount)
    emitOpcode(cgen, "jae", ".L%u", getLabel(skip, cgen))
    emitOpcode(cgen, "movl", "$0, %%eax")
    maybeEmitLabel(skip, cgen)
    emitOpcode(cgen, "sall", "$2, %%eax") # box it
    emitOpcode(cgen, "orl", "$2, %%eax")
    emitOpcode(cgen, "pushl", "%%eax")
    emitOpcode(cgen, "movl", "$__sym_withContentsOfStack$size$, %%edx")
    emitOpcode(cgen, "movl", "$2, %%ecx")
    emitOpcode(cgen, "call", "SpikeSendMessage") # leave result on stack

    # restore %edx
    emitOpcode(cgen, "movl", "-16(%%ebp), %%edx")
    
    if cgen.localCount > 1:
        # reallocate locals
        emitOpcode(cgen, "movl", "$%lu, %%ecx", cgen.localCount - 1)
        loop = Label()
        defineLabel(loop, cgen)
        maybeEmitLabel(loop, cgen)
        emitOpcode(cgen, "pushl", "$0")
        emitOpcode(cgen, "loop", ".L%u", getLabel(loop, cgen))
    
    return


def epilogue(cgen):
    # discard locals
    if cgen.blockCount:
        # locals on heap
        pass
    elif cgen.varArgList:
        emitOpcode(cgen, "addl", "$%lu, %%esp", SIZEOF_OBJ_PTR * (cgen.localCount + cgen.maxArgumentCount))
    elif cgen.localCount != 0:
        emitOpcode(cgen, "addl", "$%lu, %%esp", SIZEOF_OBJ_PTR * cgen.localCount)

    if cgen.varArgList or cgen.blockCount:
        skip = Label()
        defineLabel(skip, cgen)
        emitOpcode(cgen, "popl", "%%edx") # argumentCount
        emitOpcode(cgen, "cmpl", "$%lu, %%edx", cgen.maxArgumentCount)
        emitOpcode(cgen, "jae", ".L%u", getLabel(skip, cgen))
        emitOpcode(cgen, "movl", "$%lu, %%edx", cgen.maxArgumentCount)
        maybeEmitLabel(skip, cgen)
        emitOpcode(cgen, "shll", "$2, %%edx") # convert to byte count

    # restore registers
    emitOpcode(cgen, "popl", "%%edi") # instVarPointer
    emitOpcode(cgen, "popl", "%%esi") # self
    emitOpcode(cgen, "popl", "%%ebx") # methodClass
    
    # save result
    if cgen.blockCount:
        emitOpcode(cgen, "movl", "%%eax, 8(%%esp,%%edx)")
    elif cgen.varArgList:
        emitOpcode(cgen, "movl", "%%eax, 8(%%ebp,%%edx)")
    else:
        emitOpcode(cgen, "movl", "%%eax, %d(%%ebp)", SIZEOF_OBJ_PTR * (2 + cgen.maxArgumentCount))

    # restore caller's stack frame (%ebp)
    emitOpcode(cgen, "popl", "%%ebp")
    
    # pop arguments and return, leaving result on the stack
    if cgen.varArgList or cgen.blockCount:
        # ret %edx
        emitOpcode(cgen, "popl", "%%eax") # return address
        emitOpcode(cgen, "addl", "%%edx, %%esp")
        emitOpcode(cgen, "jmp", "*%%eax")  # return
    else:
        emitOpcode(cgen, "ret", "$%lu", SIZEOF_OBJ_PTR * cgen.maxArgumentCount)

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

def printStringLiteral(value, out):
    fputc('"', out)
    for c in value:
        # XXX: numeric escape codes
        subst = substDict.get(c)
        if subst:
            fputs(subst, out)
        else:
            fputc(c, out)
    fputc('\"', out)
    return


def mangle(sym):
    # XXX
    sym = sym.replace(' ', '$$')
    sym = sym.replace(':', '$')
    return sym


def emitROData(cgen):

    out = cgen.out
    fprintf(out, "\t.section\t.rodata\n")

    fprintf(out, "\t.align\t4\n")
    for value in cgen.intData:
        fprintf(out,
                "__int_%ld:\n"
                "\t.globl\t__int_%ld\n"
                "\t.type\t__int_%ld, @object\n"
                "\t.size\t__int_%ld, 8\n"
                "\t.long\tInteger\n"
                "\t.long\t%ld\n",
                value, value, value, value, value)

    for i, value in enumerate(cgen.floatData):
        # XXX: I would find it comforting if the exact string from
        # the source was emitted here, or something... spitting out
        # "%f" seems fuzzy.
        fprintf(out,
                "\t.align\t16\n"
                "__float_%u:\n"
                "\t.globl\t__float_%u\n"
                "\t.type\t__float_%u, @object\n"
                "\t.size\t__float_%u, 12\n"
                "\t.long\tFloat\n"
                "\t.double\t%f\n",
                i, i, i, i, value)

    fprintf(out, "\t.align\t4\n")
    for value in [ord(c) for c in sorted(list(cgen.charData))]:
        fprintf(out,
                "__char_%02x:\n"
                "\t.globl\t__char_%02x\n"
                "\t.type\t__char_%02x, @object\n"
                "\t.size\t__char_%02x, 8\n"
                "\t.long\tChar\n"
                "\t.long\t%u\n",
                value, value, value, value, value)

    for i, value in enumerate(cgen.strData):
        fprintf(out,
                "\t.align\t4\n"
                "__str_%u:\n"
                "\t.globl\t__str_%u\n"
                "\t.type\t__str_%u, @object\n"
                "\t.long\tString\n"
                "\t.long\t%lu\n"
                "\t.string\t",
                i, i, i,
                len(value))
        printStringLiteral(value, out)
        fprintf(out,
                "\n"
                "\t.size\t__str_%u, .-__str_%u\n",
                i, i)

    for sym in cgen.symData:
        name = mangle(sym)
        fprintf(out,
                "\t.align\t4\n"
                "__sym_%s:\n"
                "\t.globl\t__sym_%s\n"
                "\t.type\t__sym_%s, @object\n"
                "\t.long\tSymbol\n"
                "\t.long\t%lu\n"
                "\t.string\t\"%s\"\n"
                "\t.size\t__sym_%s, .-__sym_%s\n",
                name, name, name,
                0, # XXX: hash
                sym,
                name, name)

    fprintf(out, "\n")

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
        emitOpcode(cgen, "pushl", "%%ebp")
        emitOpcode(cgen, "pushl", "$%lu", expr.aux.block.argumentCount)
        emitOpcode(cgen, "call", "SpikeBlockCopy")
        emitBranch(OPCODE_BRANCH_ALWAYS, expr.endLabel, cgen)
        emitCodeForBlock(expr, cgen)

    elif expr.kind == EXPR_COMMA:
        for e in expr[:-1]:
            emitCodeForExpr(e, None, cgen)
            pop(cgen)
        emitCodeForExpr(expr[-1], super, cgen)

    elif expr.kind == EXPR_COMPOUND:
        emitOpcode(cgen, "pushl", "__spike_xxx_push_context") # XXX
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
            emitOpcode(cgen, "popl", "%%edx")
            routine = "SpikeSendMessage"

        elif expr.left.kind == EXPR_ATTR_VAR:
            emitCodeForExpr(expr.left.right, None, cgen)
            emitOpcode(cgen, "popl", "%%edx")
            routine = "SpikeSendMessage"

        else:
            routine = {
                OPER_APPLY: "SpikeCall",
                OPER_INDEX: "SpikeGetIndex",
                }[expr.oper]

        if expr.var:
            # push var args
            emitOpcode(cgen, "call", "SpikePushVarArgs")
            emitOpcode(cgen, "addl", "$%lu, %%ecx", argumentCount)
        else:
            emitOpcode(cgen, "movl", "$%lu, %%ecx", argumentCount)

        # call RTL routine to send message
        emitOpcode(cgen, "call", "%s%s", routine, "Super" if isSuper[0] else "")

    elif expr.kind == EXPR_ATTR:
        emitCodeForExpr(expr.left, isSuper, cgen)
        if expr.attr == 'class':
            # "foo.class": push 'klass' is-a pointer
            pop(cgen)
            emitOpcode(cgen, "pushl", "(%%eax)")
        else:
            emitCodeForLiteral(expr.attr, cgen)
            emitOpcode(cgen, "popl", "%%edx")
            emitOpcode(cgen, "call", "SpikeGetAttr%s", "Super" if isSuper[0] else "")

    elif expr.kind == EXPR_ATTR_VAR:
        emitCodeForExpr(expr.left, isSuper, cgen)
        emitCodeForExpr(expr.right, None, cgen)
        emitOpcode(cgen, "popl", "%%edx")
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
        emitOpcode(cgen, "cmpl", "(%%esp), %%eax")
        emitOpcode(cgen, "movl", "$false, (%%esp)")
        emitOpcode(cgen, "je" if expr.inverted else "jne", ".L%u", getLabel(expr.endLabel, cgen))
        emitOpcode(cgen, "movl", "$true, (%%esp)")

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
        emitOpcode(cgen, "popl", "%%edx")
        emitOpcode(cgen, "movl", "$%lu, %%ecx", argumentCount)
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
    emitOpcode(cgen, "movl", "$%lu, %%ecx", resultDepth + 1)
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
            emitOpcode(cgen, "popl", "%%edx")
            emitOpcode(cgen, "call", "SpikeGetAttr%s", "Super" if isSuper[0] else "")
        else:
            assert False, "invalid lvalue"

        inPlaceOp(expr, 1 + argumentCount + 1, # receiver, args, result
                  cgen)


    if expr.left.kind in (EXPR_ATTR, EXPR_ATTR_VAR):
        # XXX: re-think this whole thing
        emitOpcode(cgen, "popl", "%%eax")
        emitOpcode(cgen, "popl", "%%edx")
        emitOpcode(cgen, "pushl", "%%eax")
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
        emitOpcode(cgen, "movl", "$%lu, %%ecx", argumentCount)
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
    emitOpcode(cgen, "movl", "$%lu, %%ecx", argumentCount)
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
                    emitOpcode(cgen, "pushl", "$true" if pushOpcode == OPCODE_PUSH_TRUE else "$false")

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


def blockEpilogue(cgen):
    # save result
    pop(cgen)
    emitOpcode(cgen, "movl", "%%eax, %lu(%%esp)", SIZEOF_OBJ_PTR * (6 + cgen.maxArgumentCount))

    # pop BlockContext pointer
    emitOpcode(cgen, "popl", "%%ecx")

    # restore caller's registers
    emitOpcode(cgen, "popl", "%%edi") # instVarPointer
    emitOpcode(cgen, "popl", "%%esi") # self
    emitOpcode(cgen, "popl", "%%ebx") # methodClass
    emitOpcode(cgen, "popl", "%%ebp") # framePointer
    emitOpcode(cgen, "popl", "%%eax") # return address
    if cgen.maxArgumentCount:
        emitOpcode(cgen, "addl", "$%lu, %%esp", SIZEOF_OBJ_PTR * cgen.maxArgumentCount)

    # save our %eip in BlockContext.pc and resume caller
    emitOpcode(cgen, "call", "SpikeResumeCaller")

    return


def emitCodeForBlock(expr, outer):

    voidDef = Name('void')
    voidDef.level = 0
    voidDef.builtInPushOpcode = OPCODE_PUSH_VOID
    voidExpr = Name('void')
    voidExpr.u.ref.definition = voidDef

    valueExpr = expr.right if expr.right else voidExpr

    # push block code generator
    cgen = BlockCodeGen(outer, expr)

    #
    # prologue
    #
    
    start = Label()
    defineLabel(start, cgen)
    maybeEmitLabel(start, cgen)

    if expr.aux.block.argumentCount:
        # store args into local variables
        for arg in xrange(expr.aux.block.argumentCount):
            index = expr.u._def.index + arg
            emitOpcode(cgen, "movl", "%lu(%%esp), %%eax", SIZEOF_OBJ_PTR * (6 + expr.aux.block.argumentCount - arg - 1))
            emitOpcode(cgen, "movl", "%%eax, %d(%%ebp)", offsetOfIndex(index, cgen))

    #
    # body
    #
    
    body = CompoundStmt(expr.aux.block.stmtList) # XXX ?

    for s, childNextLabel in body.iterWithLabels(valueExpr.label):
        emitCodeForStmt(s, childNextLabel, None, None, cgen)

    emitCodeForExpr(valueExpr, None, cgen)

    #
    # epilogue
    #
    
    blockEpilogue(cgen)

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
    emitOpcode(cgen, "movl", "%%eax, %d(%%ebp)", localVarOffset(_def, cgen))

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
        if cgen.kind == BlockCodeGen:
            # Here the code has to longjmp (in essence) to the home
            # context, and then branch to the enclosing method's
            # epilogue.

            # get the code gen for the enclosing method
            mcg = cgen.method

            # evaluate result
            if stmt.expr:
                emitCodeForExpr(stmt.expr, None, cgen)
            else:
                emitOpcode(cgen, "pushl", "$void")

            # resume home context, moving result to home stack
            emitOpcode(cgen, "call", "SpikeResumeHome")

            # pop result and return from home context
            pop(cgen)
            emitBranch(OPCODE_BRANCH_ALWAYS, mcg.epilogueLabel, cgen)
        else:
            if stmt.expr:
                emitCodeForExpr(stmt.expr, None, cgen)
                pop(cgen)
            else:
                emitOpcode(cgen, "movl", "$void, %%eax")

            emitBranch(OPCODE_BRANCH_ALWAYS, cgen.epilogueLabel, cgen)

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
            emitOpcode(cgen, "pushl", "$void")

        blockEpilogue(cgen)

    else:
        assert False, "unexpected statement node: %r" % stmt

    return


#------------------------------------------------------------------------
# methods

def emitCodeForArgList(stmt, cgen):
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

        if cgen.varArgList:
            # range check
            emitOpcode(cgen, "cmpl", "$%lu, %%edx", cgen.maxArgumentCount)
            emitOpcode(cgen, "jae", ".L%u", getLabel(skip, cgen))

        # switch jump
        emitOpcode(cgen, "subl", "$%lu, %%edx", cgen.minArgumentCount)
        emitOpcode(cgen, "shll", "$2, %%edx") # convert to byte offset
        emitOpcode(cgen, "movl", ".L%u(%%edx), %%eax", getLabel(table, cgen))
        emitOpcode(cgen, "jmp", "*%%eax")

        # switch table
        fprintf(out, "\t.section\t.rodata\n")
        fprintf(out, "\t.align\t4\n")
        maybeEmitLabel(table, cgen)
        tally = 0
        for arg in optionalArgList:
            assert arg.kind == EXPR_ASSIGN, "assignment expected"
            fprintf(out, "\t.long\t.L%u\n", getLabel(arg.label, cgen))
            tally += 1
        if not cgen.varArgList:
            fprintf(out, "\t.long\t.L%u\n", getLabel(skip, cgen))
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

    obj = ""
    code = ".code"

    if outer.kind == ClassCodeGen:
        codeObjectClass = "Method"
        className = outer.classDef.expr.sym
        suffix = ".class" if meta else ""
        ns = {
            METHOD_NAMESPACE_RVALUE: ".0.",
            METHOD_NAMESPACE_LVALUE: ".1.",
            }[stmt.u.method.ns]

    elif selector == 'main':
        codeObjectClass = "Function"
        className = ""
        suffix = ""
        ns = "spike."
    else:
        codeObjectClass = "Function"
        className = ""
        suffix = ""
        ns = ""

    functionName = mangle(selector)

    fprintf(out, "\t.text\n")
    fprintf(out,
            "\t.align\t4\n"
            "%s%s%s%s%s:\n"
            "\t.globl\t%s%s%s%s%s\n"
            "\t.type\t%s%s%s%s%s, @object\n"
            "\t.size\t%s%s%s%s%s, 16\n"
            "\t.long\t%s\n"
            "\t.long\t%lu\n"
            "\t.long\t%lu\n"
            "\t.long\t%lu\n",
            className, suffix, ns, functionName, obj,
            className, suffix, ns, functionName, obj,
            className, suffix, ns, functionName, obj,
            className, suffix, ns, functionName, obj,
            codeObjectClass,
            cgen.minArgumentCount,
            cgen.maxArgumentCount + (0x80000000 if cgen.varArgList else 0), cgen.localCount)
    fprintf(out,
            "%s%s%s%s%s:\n"
            "\t.globl\t%s%s%s%s%s\n"
            "\t.type\t%s%s%s%s%s, @function\n",
            className, suffix, ns, functionName, code,
            className, suffix, ns, functionName, code,
            className, suffix, ns, functionName, code)

    prologue(cgen)

    emitCodeForArgList(stmt, cgen)
    emitCodeForStmt(body, sentinel.label, None, None, cgen)
    emitCodeForStmt(sentinel, None, None, None, cgen)

    maybeEmitLabel(cgen.epilogueLabel, cgen)
    epilogue(cgen)

    fprintf(out,
            "\t.size\t%s%s%s%s.code, .-%s%s%s%s.code\n"
            "\n",
            className, suffix, ns, functionName,
            className, suffix, ns, functionName)

    for s in body:
        if s.kind in (STMT_DEF_CLASS, STMT_DEF_METHOD):
            assert False, "nested class/method not supported by x86 backend"

    return


def emitCFunction(stmt, cgen):

    assert cgen.level == 1, "C functions must be global"

    out = cgen.out

    # XXX: comdat
    sym = stmt.u.method.name
    suffix = ".thunk"

    fprintf(out,
            "\t.data\n"
            "\t.align\t4\n"
            "%s%s:\n"
            "\t.type\t%s%s, @object\n",
            sym, suffix, sym, suffix)

    # Currently, the "signature" is simply the return type (used for
    # boxing).
    signature = {
        0:               "Object",
        SPEC_TYPE_OBJ:   "Object",
        SPEC_TYPE_INT:   "Integer",
        SPEC_TYPE_CHAR:  "Char",
        }[stmt.decl.specifiers & SPEC_TYPE]

    fprintf(out, "\t.long\tCFunction\n" # klass
            "\t.long\t%s\n" # signature
            "\t.long\t%s\n", # pointer
            signature, sym)

    fprintf(out, "\t.size\t%s%s, .-%s%s\n",
            sym, suffix, sym, suffix)
    fprintf(out, "\n")

    return


#------------------------------------------------------------------------
# class and file scope

def emitCodeForCompound(body, meta, cgen):

    assert body.kind == STMT_COMPOUND, "compound statement expected"

    for s in body:

        if s.kind == STMT_DEF_METHOD:
            if (s.decl.specifiers & SPEC_STORAGE) == SPEC_STORAGE_EXTERN:
                if (s.decl.specifiers & SPEC_CALL_CONV) == SPEC_CALL_CONV_C:
                    emitCFunction(s, cgen)
            else:
                emitCodeForMethod(s, meta, cgen)

        elif s.kind == STMT_DEF_VAR:
            if cgen.level == 1:
                # global variable definition

                out = cgen.out

                fprintf(out, "\t.bss\n")
                fprintf(out, "\t.align\t4\n")

                for expr in s.defList:
                    assert expr.kind == EXPR_NAME, "initializers not allowed here"
                    if (expr.specifiers & SPEC_STORAGE) == SPEC_STORAGE_EXTERN:
                        continue
                    sym = expr.sym
                    fprintf(out,
                            "%s:\n"
                            "\t.globl\t%s\n"
                            "\t.type\t%s, @object\n"
                            "\t.size\t%s, 4\n",
                            sym, sym, sym, sym)
                    fprintf(out, "\t.zero\t4\n")

                fprintf(out, "\n")
            else:
                for expr in s.defList:
                    assert expr.kind == EXPR_NAME, "initializers not allowed here"

        elif s.kind == STMT_DEF_CLASS:
            emitCodeForClass(s, cgen)

        elif s.kind == STMT_PRAGMA_SOURCE:
            cgen.module.source = s.u.source

        else:
            assert False, "executable code not allowed here"

    return


#------------------------------------------------------------------------
# classes

def emitCodeForBehaviorObject(classDef, meta, cgen):
    body = classDef.bottom if meta else classDef.top
    out = cgen.out
    sym = classDef.expr.sym

    if classDef.u.klass.superclassName.u.ref.definition.u._def.pushOpcode == OPCODE_PUSH_NULL:
        superSym = None # Class 'Object' has no superclass.
    else:
        superSym = classDef.u.klass.superclassName.sym

    suffix = ".class" if meta else ""

    methodTally = [0 for ns in range(NUM_METHOD_NAMESPACES)]
    for ns in range(NUM_METHOD_NAMESPACES):
        if body is None:
            continue
        for s in body:
            if s.kind == STMT_DEF_METHOD and s.u.method.ns == ns:
                methodTally[ns] += 1

    fprintf(out,
            "\t.data\n"
            "\t.align\t4\n")

    fprintf(out, "%s%s:\n"
            "\t.globl\t%s%s\n"
            "\t.type\t%s%s, @object\n", sym, suffix, sym, suffix, sym, suffix)

    if meta:
        fprintf(out, "\t.long\tMetaclass\n") # klass
    else:
        fprintf(out, "\t.long\t%s.class\n", sym) # klass

    if superSym: # The metaclass hierarchy mirrors the class hierarchy.
        fprintf(out, "\t.long\t%s%s\t/* superclass */\n", superSym, suffix) # superclass
    elif meta: # The metaclass of 'Object' is a subclass of 'Class'.
        fprintf(out, "\t.long\tClass\t/* superclass */\n") # superclass
    else: # Class 'Object' has no superclass.
        fprintf(out, "\t.long\t0\t/* superclass */\n") # superclass

    # XXX: hash at compile time
    for ns in range(NUM_METHOD_NAMESPACES):
        if methodTally[ns]:
            fprintf(out,
                    "\t.long\t%s%s.methodTable.%d\t/* methodTable[%d] */\t\n",
                    sym, suffix, ns, ns) # methodTable[ns]
        else:
            fprintf(out,
                    "\t.long\t0\t/* methodTable[%d] */\t\n",
                    ns) # methodTable[ns]

    # instVarCount
    fprintf(out,
            "\t.long\t%lu\t/* instVarCount */\n",
            classDef.u.klass.classVarCount if meta else classDef.u.klass.instVarCount)

    if meta:
        fprintf(out, "\t.long\t%s\t/* thisClass */\n", sym) # thisClass
    else:
        willEmitSym(classDef.expr.sym, cgen)
        fprintf(out, "\t.long\t__sym_%s\t/* name */\n", sym) # name

    # XXX: Class objects need slots for class variables up the
    # superclass chain.  To be totally consistent, and robust with
    # dynamic linking (opaque superclasses), class objects would have
    # to be created at runtime.
    if not meta:
        fprintf(out, "\t.long\t0,0,0,0,0,0,0,0\t/* XXX class variables */\n")

    fprintf(out,
            "\t.size\t%s%s, .-%s%s\n",
            sym, suffix, sym, suffix)

    # A method table is simply an Array of symbol/method key/value pairs.

    for ns in range(NUM_METHOD_NAMESPACES):
        if not methodTally[ns]:
            continue

        fprintf(out,
                "%s%s.methodTable.%d:\n"
                "\t.globl\t%s%s.methodTable.%d\n"
                "\t.type\t%s%s.methodTable.%d, @object\n",
                sym, suffix, ns,
                sym, suffix, ns,
                sym, suffix, ns)

        fprintf(out, "\t.long\tArray\n")
        fprintf(out, "\t.long\t%d\n", 2 * methodTally[ns])

        for s in body:
            if s.kind == STMT_DEF_METHOD and s.u.method.ns == ns:
                selector = s.u.method.name
                willEmitSym(selector, cgen)
                selector = mangle(selector)
                fprintf(out,
                        "\t.long\t__sym_%s, %s%s.%d.%s\n",
                        selector,
                        sym, suffix, ns, selector)

        fprintf(out,
                "\t.size\t%s%s.methodTable.%d, .-%s%s.methodTable.%d\n",
                sym, suffix, ns,
                sym, suffix, ns)

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

    return


#------------------------------------------------------------------------
# main entry point

def generateCode(tree, out):

    assert tree.kind == STMT_COMPOUND, "compound statement expected"

    cgen = ModuleCodeGen(out)

    # Generate code.
    emitCodeForCompound(tree, False, cgen)

    emitROData(cgen)

    return

