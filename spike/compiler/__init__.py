
from Node import Node
from NodeFactory import NodeFactory
from Notifier import Notifier



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


