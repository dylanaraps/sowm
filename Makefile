CFLAGS += -std=c99 -Wall -Wextra -pedantic
CFLAGS += -Wmissing-prototypes -Wno-unused-parameter
PREFIX ?= /usr
BINDIR ?= $(PREFIX)/bin

CC ?= gcc

all: config.h sowm

config.h:
	cp config.def.h config.h

sowm: sowm.o
	$(CC) $(LDFLAGS) -O3 -o $@ $+ -lX11

install: all
	install -Dm 755 sowm $(DESTDIR)$(BINDIR)/sowm

clean:
	rm -f sowm *.o
