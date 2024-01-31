
from Node import Node
from NodeFactory import NodeFactory
from Notifier import Notifier
import sys



def parse(pathname, notifier):
    import spike.ext as ext
    
    input = open(pathname)
    output = open("out", "w+") # XXX
    error = open("err", "w+") # XXX
    
    input.readline() # XXX: skip shebang
    
    ext.parse(input, output, error)
    
    error.seek(0)
    namespace = dict(
        n = notifier
        )
    notifier.source = pathname
    exec error in namespace
    accepted = namespace['accepted']
    
    # NB: Input with recoverable parse errors is still "accepted".
    if not accepted:
        return None
    
    output.seek(0)
    namespace = dict(
        f = NodeFactory()
        )
    exec output in namespace
    
    from spike.compiler.statements import Compound
    return Compound(namespace['root'])


try:
    from spike.ext import trace
except:
    pass


def compile(pathnames, out, err = sys.stderr, externals = None):
    from spike.compiler.scheck import check, declareBuiltIn, declareExternalSymbols
    from spike.compiler.symbols import SymbolTable
    from spike.compiler.cgen import generateCode
    from spike.compiler.statements import Compound, PragmaSource
    
    notifier = Notifier(stream = err)
    
    st = SymbolTable()
    declareBuiltIn(st, notifier)
    if externals:
        declareExternalSymbols(externals, st, notifier)
    
    tree = Compound()
    
    for pathname in pathnames:
        t = parse(pathname, notifier)
        if t is None:
            continue
        tree.append(PragmaSource(pathname))
        tree.extend(t)

    check(tree, st, notifier)
    
    notifier.failOnError()
    
    generateCode(tree, out)
    
    return


def assembleAndLink(assembly):
    from os import spawnlp, P_WAIT
    from os.path import dirname, join
    import spike

    rtl = [
        "Array.s",
        "BlockContext.s",
        "blocks.s",
        "CFunction.s",
        "CObject.s",
        "Context.c",
        "Char.s",
        "epilogue.s",
        "error.s",
        "Float.c",
        "Float.s",
        "Function.s",
        "Integer.s",
        "main.s",
        "Object.s",
        "prologue.s",
        "rot.s",
        "send.s",
        "singletons.s",
        "stacktrace.c",
        "String.c",
        "String.s",
        "Symbol.c",
        "test.s",
        "trap.c",
        "lookup.c",
        "XFunction.s",
        ]
    sdir = join(dirname(spike.__file__), 'rtl')
    rtl = [join(sdir, item) for item in rtl]
    rtl.append("-lm")

    status = spawnlp(
        P_WAIT,
        "gcc", "gcc", "-D_GNU_SOURCE", "-g", "-I.",
        #"-DLOOKUP_DEBUG",
        assembly,
        *rtl
        )
    return status

