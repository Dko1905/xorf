.POSIX:

include config.mk

OBJECTS = main.o

all: options xorf

options:
	@echo xorf build options:
	@echo "CFLAGS  = $(MYCFLAGS)"
	@echo "LDFLAGS = $(MYLDFLAGS)"
	@echo "CC      = $(CC)"

.c.o:
	$(CC) $(MYCFLAGS) -c $<

xorf: $(OBJECTS)
	$(CC) $(OBJECTS) -o xorf $(MYLDFLAGS)

# Extra
clean:
	rm -f xorf $(OBJECTS)

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f xorf $(DESTDIR)$(PREFIX)/bin
	chmod +x $(DESTDIR)$(PREFIX)/bin/xorf

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/xorf

.PHONY: all options clean install uninstall
