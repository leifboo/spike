
all: gram.c lexer.c


gram.c: gram.y lemon
	./lemon $< || (rm $@ && exit 1)

lexer.c: lexer.l gram.c
	flex -o$@ $<

lemon: lemon.c
	$(CC) -o $@ lemon.c


clean:
	rm -f *.o gram.c gram.h gram.out lexer.c lemon
