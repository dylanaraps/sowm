CFLAGS+= -std=c99 -Wall -Wextra -pedantic -Wno-deprecated-declarations
LDADD+= -lX11
LDFLAGS=
PREFIX?= /usr
BINDIR?= $(PREFIX)/bin

CC ?= gcc

all: config.h sowm

config.h:
	cp config.def.h config.h

sowm: sowm.o
	$(CC) $(LDFLAGS) -O3 -o $@ $+ $(LDADD)

install: all
	install -Dm 755 sowm $(DESTDIR)$(BINDIR)/sowm

clean:
	rm -f sowm *.o
