

class Node(object):


    # XXX: legacy code support
    kind = property(lambda self: self.__class__)


    def _iterChildren(self):
        for name in self.childAttrNames:
            yield (name, getattr(self, name))
        return
    children = property(_iterChildren)


    def dump(self, stream, indent=0):
        # XXX: Expand argument lists?  Show declspecs?  (Ditto for
        # 'graphviz'.)
        for attr, child in self.children:
            print >>stream, "%s%s: %r" % ("    " * indent, attr, child)
            if hasattr(child, 'dump'):
                child.dump(stream, indent + 1)
        return


    def graphviz(self, stream):
        print >>stream, '    n%d[label="%r"];' % (id(self), self)
        
        for attr, child in self.children:
            if child is None:
                pass
            elif hasattr(child, 'graphviz'):
                child.graphviz(stream)
            else:
                print >>stream, '    n%d[label="%r"];' % (id(child), child)
        
        print >>stream
        for attr, child in self.children:
            if child is None:
                continue
            print >>stream, '    n%d->n%d[label="%s"];' % (
                id(self), id(child), attr)

        return
