CFLAGS+= -Wall
LDADD+= -lX11
LDFLAGS=
PREFIX?= /usr
BINDIR?= $(PREFIX)/bin

CC=gcc

all: sowm

sowm: sowm.o
	$(CC) $(LDFLAGS) -Os -o $@ $+ $(LDADD)

install: all
	install -Dm 755 sowm $(DESTDIR)$(BINDIR)/sowm

clean:
	rm -f sowm *.o
