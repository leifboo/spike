# Spike: The Language

Spike is a brand-new programming language. It was born out of
frustration with existing programming languages, and a love of
Smalltalk. In essence, Spike is Smalltalk with a C-like syntax.

Spike is still a puppy. He's still in alpha.

There is no documentation except for this "readme". There is no
linker; but you won't miss it, because there are no libraries to speak
of (not even a standard library). There is no garbage collector, so
Spike is best used for short-lived programs. I'll stop here, because
the number of things that are missing are too numerous to mention.

## Installation

Spike currently only works on Linux x86_64.

```
make  # generate gram.c
python setup.py install --prefix=PREFIX
 # add 'spike' to your PATH and PYTHONPATH
./examples/hello.spk
```

## How It Works

Spike's parser is generated using the Lemon Parser Generator from the
SQLite project. The parser is packaged as an extension module
(spike.ext) that spits out Python code. This Python code, when
evaluated, constructs the parse tree.

The compiler, written in Python, consumes the parser's output using a
Python 'exec' statement. Then it generates x86_64 assembly! The final
executable is produced using GCC.

The generated code uses the native x86 stack for argument and message
passing. MethodContext and BlockContext objects reside on the
stack. For this reason, certain examples such as 'accgen.spk' use the
'closure' message to copy a block's BlockContext to the
heap. Otherwise, undefined behavior will result.

## More Information

Earlier incarnations of Spike may be found on SourceForge:

[https://spike.sourceforge.net/](https://spike.sourceforge.net/)

The tarball found there features an interpreter written in C (a more
traditional approach).

