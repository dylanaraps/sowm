.POSIX:

PREFIX = /usr/local

ALL_WARN    = -Wall -Wextra -pedantic -Wmissing-prototypes -Wstrict-prototypes
ALL_CFLAGS  = $(CFLAGS) $(CPPFLAGS) -std=c99 $(ALL_WARN)
ALL_LDFLAGS = $(LDFLAGS) $(LIBS) -lxcb

CC = cc

OBJ = src/event.o src/sowm.o
HDR = src/event.h src/globals.h

.c.o:
	$(CC) $(ALL_CFLAGS) -c -o $@ $<

sowm: $(OBJ)
	$(CC) $(ALL_CFLAGS) -o $@ $(OBJ) $(ALL_LDFLAGS)

$(OBJ): $(HDR)

install: sowm
	mkdir -p $(DESTDIR)/bin
	cp sowm $(DESTDIR)/bin/sowm

uninstall:
	rm -f $(DESTDIR)/bin/sowm

clean:
	rm -f sowm *.o

.PHONY: install uninstall clean
