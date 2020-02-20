CFLAGS += -std=c99 -Wall -Wextra -pedantic -Wold-style-declaration
CFLAGS += -Wmissing-prototypes -Wno-unused-parameter
PREFIX ?= /usr
BINDIR ?= $(PREFIX)/bin
CC     ?= gcc

all: config.h sowm

config.h:
	cp config.def.h config.h

sowm:
	$(CC) -O3 $(CFLAGS) -lX11 $(LDFLAGS) -o sowm sowm.c

install: all
	install -Dm755 sowm $(DESTDIR)$(BINDIR)/sowm

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/sowm

clean:
	rm -f sowm *.o
