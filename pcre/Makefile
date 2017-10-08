CFLAGS += -O3 -g -c -fPIC -I../../lemon/src

LDFLAGS += -shared -rdynamic -L../../lemon -llemon

SRCS  = pcre.c

OBJS = $(SRCS:.c=.o)
INCS := $(wildcard *.h)

all: pcre.so

pcre.so: $(OBJS) $(INCS) Makefile
	$(CC) $(LDFLAGS) $(OBJS) -lpcre -o $@

%.o: %.c
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo CC $<

clean:
	rm $(OBJS) pcre.so
