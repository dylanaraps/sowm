CFLAGS+= -Wall
LDADD+= -lX11 
LDFLAGS=
PREFIX?= /usr
BINDIR?= $(PREFIX)/bin

CC=gcc

all: catwm

catwm: catwm.o
	$(CC) $(LDFLAGS) -Os -o $@ $+ $(LDADD)

install: all
	install -Dm 755 catwm $(DESTDIR)$(BINDIR)/catwm

clean:
	rm -f catwm *.o
