
all: pspk


pspk: pspk.o gram.o lexer.o
	$(CC) -o $@ pspk.o gram.o lexer.o

gram.c: gram.y lemon
	./lemon $< || (rm $@ && exit 1)

lexer.c: lexer.l gram.c
	flex -o$@ $<

lemon: lemon.c
	$(CC) -o $@ lemon.c


clean:
	rm -f pspk *.o gram.c gram.h gram.out lexer.c lemon
