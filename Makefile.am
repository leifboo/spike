## Process this file with automake to produce Makefile.in

bin_PROGRAMS = spike cspk spikex
noinst_PROGRAMS = xcspk
noinst_LIBRARIES = libspike.a

spike_SOURCES = spike.c
nodist_spike_SOURCES = xgenerated.c
BUILT_SOURCES = xgenerated.c
spike_LDADD = libspike.a

cspk_SOURCES = cspk.c
cspk_LDADD = libspike.a

xcspk_SOURCES = xcspk.c
xcspk_LDADD = libspike.a

spikex_SOURCES = spikex.c


libspike_a_SOURCES = \
	array.c \
	array.h \
	behavior.c \
	behavior.h \
	bool.h \
	boot.c \
	boot.h \
	cgen.c \
	cgen.h \
	char.c \
	char.h \
	class.c \
	class.h \
	compiler.c \
	compiler.h \
	cxxgen.c \
	cxxgen.h \
	dict.c \
	dict.h \
	disasm.c \
	disasm.h \
	float.c \
	float.h \
	gram.c \
	gram.h \
	heart.h \
	host.c \
	host.h \
	int.c \
	int.h \
	interp.c \
	interp.h \
	io.c \
	io.h \
	lexer.c \
	lexer.h \
	metaclass.c \
	metaclass.h \
	module.c \
	module.h \
	native.c \
	native.h \
	notifier.c \
	notifier.h \
	obj.c \
	obj.h \
	om.c \
	om.h \
	oper.h \
	parser.c \
	parser.h \
	rodata.c \
	rodata.h \
	scheck.c \
	scheck.h \
	st.c \
	st.h \
	str.c \
	str.h \
	sym.c \
	sym.h \
	tree.c \
	tree.h \
	x86gen.c \
	x86gen.h \
	$(empty)

spk = \
	$(srcdir)/bool.spk \
	$(empty)

EXTRA_DIST = \
	gram.y \
	lexer.l \
	lemon.c \
	lempar.c \
	$(spk) \
	$(TESTS) \
	$(test_io) \
	run-test \
	$(empty)


gram.c: $(srcdir)/gram.y #lemon
	$(CC) -o lemon $(srcdir)/lemon.c # XXX
	./lemon $< || (rm -f gram.c && exit 1)

lexer.c: $(srcdir)/lexer.l gram.c
	flex -o$@ $<

lemon: $(srcdir)/lemon.c
	$(CC) -o $@ $(srcdir)/lemon.c

xgenerated.c: xcspk
	./xcspk $(spk)


CLEANFILES = \
	lemon \
	test.out \
	test.err \
	test.tmp \
	xgenerated.c \
	$(empty)

MAINTAINERCLEANFILES = \
	lexer.c \
	gram.c \
	gram.h \
	gram.out \
	$(empty)


# tests

TESTS_ENVIRONMENT = $(top_srcdir)/run-test $(top_srcdir) $(top_builddir)/spike

TESTS = \
	examples/accgen.spk \
	examples/blocks.spk \
	examples/circle.spk \
	examples/fib.spk \
	examples/fibgen.spk \
	examples/fma.spk \
	examples/foreach.spk \
	examples/functor.spk \
	examples/hello.spk \
	examples/lst/8queen.spk \
	examples/lst/blocks.spk \
	examples/lst/cond.spk \
	examples/lst/super.spk \
	examples/lst/temp.spk \
	examples/nlines.spk \
	examples/point.spk \
	examples/primes.spk \
	tests/attr.spk \
	tests/bc-in-blocks.spk \
	tests/cycle.spk \
	tests/default-args.spk \
	tests/dnu.spk \
	tests/dot-star.spk \
	tests/empty.spk \
	tests/fe.spk \
	tests/gen.spk \
	tests/gremlin.spk \
	tests/index.spk \
	tests/iter-musings.spk \
	tests/kw-permutations.spk \
	tests/leaf.spk \
	tests/my-array.spk \
	tests/no-main.spk \
	tests/old.spk \
	tests/pi.spk \
	tests/rodata.spk \
	tests/static-errors.spk \
	tests/syntax-error.spk \
	tests/unary-kw.spk \
	tests/varargs.spk \
	$(empty)

XFAIL_TESTS = \
	examples/primes.spk \
	tests/bc-in-blocks.spk \
	tests/empty.spk \
	tests/kw-permutations.spk \
	tests/no-main.spk \
	tests/pi.spk \
	tests/syntax-error.spk \
	$(empty)

test_io = \
	examples/accgen.out \
	examples/blocks.out \
	examples/circle.in \
	examples/circle.out \
	examples/fib.out \
	examples/fibgen.out \
	examples/fma.out \
	examples/foreach.out \
	examples/functor.out \
	examples/hello.out \
	examples/lst/8queen.out \
	examples/lst/blocks.out \
	examples/lst/cond.out \
	examples/lst/super.out \
	examples/lst/temp.out \
	examples/nlines.in \
	examples/nlines.out \
	examples/point.out \
	examples/primes.out \
	tests/attr.out \
	tests/cycle.err \
	tests/cycle.out \
	tests/default-args.out \
	tests/dnu.out \
	tests/dot-star.out \
	tests/fe.out \
	tests/gen.out \
	tests/gremlin.out \
	tests/index.out \
	tests/iter-musings.out \
	tests/leaf.out \
	tests/my-array.out \
	tests/old.out \
	tests/pi.out \
	tests/rodata.out \
	tests/static-errors.err \
	tests/static-errors.out \
	tests/unary-kw.out \
	tests/varargs.out \
	$(empty)
