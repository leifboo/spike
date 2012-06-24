
from collections import namedtuple
import literals


Token = namedtuple('Token', ('text', 'lineNo'))


class Integer(Token):
    value = property(lambda self: literals.Integer(self.text))


class Float(Token):
    value = property(lambda self: literals.Float(self.text))


class Character(Token):
    value = property(lambda self: literals.Character(eval(self.text)))


class String(Token):
    value = property(lambda self: literals.String(eval(self.text)))


class Symbol(Token):
    value = property(lambda self: literals.Symbol(self.text[1:]))


class Identifier(Token):
    value = property(lambda self: literals.Symbol(self.text))


class Specifier(Token):
    value = property(lambda self: literals.Symbol(self.text))


class Keyword(Token):
    # keywords can be used as symbols; e.g., "foo.class"
    value = property(lambda self: literals.Symbol(self.text))
