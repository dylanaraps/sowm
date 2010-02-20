CFLAGS+= -Wall
LDADD+= -lX11
LDFLAGS=
EXEC=catwm

PREFIX?= /usr
BINDIR?= $(PREFIX)/bin

CC= gcc

all: $(EXEC)

catwm: catwm.o
	$(CC) $(LDFLAGS) -o $@ $+ $(LDADD)

install: all
	install -m 755 catwm $(DESTDIR)$(BINDIR)

clean:
	rm -f catwm *.o
