XLIB = libxlock.so
CC := $(shell sh -c 'type $(CC) >/dev/null 2>/dev/null && echo $(CC) || echo gcc')
CFLAGS := $(CFLAGS)
LDFLAGS := $(LDFLAGS)


.PHONY : all
all : $(XLIB) test


xlock.o : xlock.c
	$(CC) $(CFLAGS) -fPIC -o $@ -c $<

xsem.o : xsem.c
	$(CC) $(CFLAGS) -fPIC -o $@ -c $<

psem.o : psem.c
	$(CC) $(CFLAGS) -fPIC -o $@ -c $<

$(XLIB): xlock.o xsem.o psem.o
	$(CC) xlock.o xsem.o psem.o -shared -o $@

test: test_xlock.bin test_xsem.bin

test_xlock.bin: test_xlock.o $(XLIB)
	${CC} -o $@ $< $(XLIB)
test_xsem.bin: test_xsem.o $(XLIB)
	${CC} -o $@ $< $(XLIB)


.PHONY : clean
clean:
	rm -rf *.o *.bin *.so


