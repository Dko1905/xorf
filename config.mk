# XORF version
VERSION = 1.0.0

# Customize to fit your system
PREFIX = /usr/local
INCS = 
LIBS = 

# Flags
MYCPPFLAGS = -DVERSION=\"$(VERSION)\" # C preprocessor
MYCFLAGS = -std=c99 -Wall -Wextra -pedantic -pthread \
           $(INCS) $(MYCPPFLAGS) $(CPPFLAGS) $(CFLAGS)
MYLDFLAGS = -pthread$(LIBS) $(LDFLAGS)
