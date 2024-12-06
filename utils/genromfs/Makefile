
# Makefile for the genromfs program.
ifeq ($(OS), Windows_NT)
	LDLIBS = -lshlwapi -lws2_32
else
#	Nothing
endif

CFLAGS = -O2 -Wall #-g#
LDFLAGS = -s

all: genromfs

genromfs: genromfs.o

clean:
	rm -f *.o

distclean: clean
	rm -f genromfs

install: all install-bin install-man

install-bin:
	install -m 755 genromfs /usr/bin

install-man:
	install -m 644 genromfs.8 /usr/man/man8

