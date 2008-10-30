
CFLAGS = -g -ansi

obj = \
	behavior.o \
	bool.o \
	cgen.o \
	char.o \
	class.o \
	dict.o \
	disasm.o \
	gram.o \
	int.o \
	interp.o \
	lexer.o \
	metaclass.o \
	module.o \
	obj.o \
	parser.o \
	scheck.o \
	spike.o \
	st.o \
	str.o \
	$(empty)

all: spike

spike: $(obj)
	$(CC) -o $@ $(obj)

gram.c: gram.y lemon
	./lemon $<

lexer.c: lexer.l gram.c
	flex -o$@ $<

lemon: lemon.c
	$(CC) -o $@ lemon.c

clean:
	rm -f spike lemon $(obj) gram.c gram.h gram.out lexer.c
