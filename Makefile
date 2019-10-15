CFLAGS+= -std=c99 -Wall -Wno-deprecated-declarations -pedantic
LDADD+= -lX11
LDFLAGS=
PREFIX?= /usr
BINDIR?= $(PREFIX)/bin

CC ?= gcc

all: config.h sowm

config.h:
	cp config.def.h config.h

sowm: sowm.o
	$(CC) $(LDFLAGS) -Os -o $@ $+ $(LDADD)

install: all
	install -Dm 755 sowm $(DESTDIR)$(BINDIR)/sowm

clean:
	rm -f sowm *.o
