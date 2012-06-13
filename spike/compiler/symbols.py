

class Context(object):

    def __init__(self, level):
        self.level = level
        self.scopes = set()
        self.nDead = 0
        return

    def addScope(self, scope):
        import weakref
        self.scopes.add(weakref.ref(scope))
        return

    def removeScope(self, scope):
        import weakref
        self.scopes.remove(weakref.ref(scope))
        # XXX: We could overlap local scopes with the addition of a 'clear' opcode.
        self.nDead += len(scope.entries)
        return

    def countEntries(self):
        # XXX: only variables need slots
        tally = sum([len(scope().entries) for scope in self.scopes])
        tally += self.nDead # no overlap
        return tally

    nDefs = property(countEntries)



class Scope(object):

    def __init__(self, outer, context):
        self.outer = outer
        self.context = context
        self.entries = {}
        return



class SymbolTable:


    currentScope = None


    def enterScope(self, enterNewContext):
        
        outerScope = self.currentScope
        
        # Context objects correspond to the four levels at runtime:
        # built-in, global, instance, local (0 through 3, respectively).
        
        if enterNewContext:
            if outerScope:
                level = max(outerScope.context.level + 1, 3)
            else:
                # built-in scope
                level = 0
            context = Context(level)
            
        else:
            context = outerScope.context
        
        newScope = Scope(outerScope, context)
        context.addScope(newScope)
        
        self.currentScope = newScope
        
        return


    def exitScope(self):
        oldScope = self.currentScope
        oldScope.context.removeScope(oldScope)
        self.currentScope = oldScope.outer
        return


    def insert(self, _def, requestor):
        from expressions import Name
        
        assert isinstance(_def, Name)
        assert _def.definition is None
        
        sym = _def.sym
        cs = self.currentScope
        
        oldDef, scope = self.lookup(sym)
        if oldDef and (scope is cs or scope.context.level == 0):
            # multiply defined
            requestor.redefinedSymbol(_def)
            return
        
        # define the symbol in the current scope
        cs.entries[sym] = _def
        
        # fix-up the Name expression node
        del _def.definition # it is not a reference
        _def.u._def.level = cs.context.level
        _def.u._def.index = cs.context.nDefs - 1
        
        return


    def lookup(self, sym):
        scope = self.currentScope
        while scope:
            _def = scope.entries.get(sym)
            if _def:
                return _def, scope
            scope = scope.outer
        return None, None


    def bind(self, expr, requestor):
        from expressions import Name
        
        assert isinstance(expr, Name)
        
        _def, scope = self.lookup(expr.sym)
        if _def:
            # bind 'expr' to its definition
            expr.definition = _def
            return
        
        # undefined
        requestor.undefinedSymbol(expr)
        return
