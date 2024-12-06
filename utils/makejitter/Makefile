all: makejitter

CFLAGS = -O2 -Wall -I../../../kos-ports/include #-g#

makejitter: makejitter.o

clean:
	rm -f jitter_table.h *.o

distclean: clean
	rm -f makejitter

install: all install-bin

install-bin:
	install -m 755 makejitter /usr/bin

