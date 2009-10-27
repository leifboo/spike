
CFLAGS = -g -ansi -Iinclude -Wall -Wstrict-prototypes

boot_obj = \
	array.o \
	behavior.o \
	boot.o \
	cgen.o \
	char.o \
	class.o \
	compiler.o \
	cxxgen.o \
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
	st.o \
	str.o \
	sym.o \
	tree.o \
	$(empty)

obj = \
	xgenerated.o \
	$(boot_obj)

spk = \
	bool.spk \
	$(empty)


all: spike-1 cspk

spike-1: spike-1.o $(obj)
	$(CC) -o $@ spike-1.o $(obj)

xcspk: xcspk.o $(boot_obj)
	$(CC) -o $@ xcspk.o $(boot_obj)

xgenerated.c: xcspk
	./xcspk $(spk)

cspk: cspk.o $(boot_obj)
	$(CC) -o $@ cspk.o $(boot_obj)

gram.c: gram.y lemon
	./lemon $<

lexer.c: lexer.l gram.c
	flex -o$@ $<

lemon: lemon.c
	$(CC) -o $@ lemon.c


clean:
	rm -f spike-1 xcspk cspk lemon $(obj) gram.c gram.h gram.out lexer.c xgenerated.c
