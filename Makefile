
CFLAGS = -g -ansi -Iinclude -Wall -Wstrict-prototypes

obj = \
	array.o \
	behavior.o \
	bool.o \
	boot.o \
	cgen.o \
	char.o \
	class.o \
	compiler.o \
	dict.o \
	disasm.o \
	float.o \
	gram.o \
	host.o \
	int.o \
	interp.o \
	io.o \
	lexer.o \
	metaclass.o \
	module.o \
	native.o \
	notifier.o \
	obj.o \
	om.o \
	parser.o \
	rodata.o \
	scheck.o \
	spike-1.o \
	st.o \
	str.o \
	sym.o \
	tree.o \
	$(empty)

all: spike-1

spike-1: $(obj)
	$(CC) -o $@ $(obj)

gram.c: gram.y lemon
	./lemon $<

lexer.c: lexer.l gram.c
	flex -o$@ $<

lemon: lemon.c
	$(CC) -o $@ lemon.c

clean:
	rm -f spike-1 lemon $(obj) gram.c gram.h gram.out lexer.c
