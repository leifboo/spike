
from Node import Node
from NodeFactory import NodeFactory
from Notifier import Notifier
import sys



def parse(pathname):
    import spike.ext as ext
    
    input = open(pathname)
    output = open("out", "w+") # XXX
    error = open("err", "w") # XXX
    
    input.readline() # XXX: skip shebang
    
    ext.parse(input, output, error)
    
    output.seek(0)
    
    namespace = dict(
        f = NodeFactory()
        )
    exec output in namespace
    
    from spike.compiler.statements import Compound
    return Compound(namespace['root'])


def compile(pathnames, out, err = sys.stderr):
    from spike.compiler.scheck import check, declareBuiltIn
    from spike.compiler.symbols import SymbolTable
    from spike.compiler.cgen import generateCode
    from spike.compiler.statements import Compound, PragmaSource
    
    notifier = Notifier(stream = err)
    
    st = SymbolTable()
    declareBuiltIn(st, notifier)
    
    tree = Compound()
    
    for pathname in pathnames:
        tree.append(PragmaSource(pathname))
        tree.extend(parse(pathname))
    
    check(tree, st, notifier)
    
    notifier.failOnError()
    
    generateCode(tree, out)
    
    return


def assembleAndLink(assembly):
    from os import spawnlp, P_WAIT
    status = spawnlp(
        P_WAIT,
        "gcc", "gcc", "-g", "-I.",
        #"-DLOOKUP_DEBUG",
        assembly,
        "rtl/Array.s",
        "rtl/BlockContext.s",
        "rtl/blocks.s",
        "rtl/CFunction.s",
        "rtl/CObject.s",
        "rtl/Context.c",
        "rtl/Char.s",
        "rtl/error.s",
        "rtl/Function.s",
        "rtl/Integer.s",
        "rtl/main.s",
        "rtl/Object.s",
        "rtl/rot.s",
        "rtl/send.s",
        "rtl/singletons.s",
        "rtl/String.c",
        "rtl/String.s",
        "rtl/Symbol.c",
        "rtl/test.s",
        "rtl/lookup.c",
        )
    return status

