CFLAGS += -O3 -g -c -fPIC -I../../lemon/src

LDFLAGS += -shared -L../../lemon -llemon
LDFLAGS += -lpcre

SRCS  = pcre.c

OBJS = $(SRCS:.c=.o)
INCS := $(wildcard *.h)

all: pcre.so

pcre.so: $(OBJS) $(INCS) Makefile
	cc $(LDFLAGS) $(OBJS) -o $@


%.o: %.c
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo CC $<

clean:
	rm $(OBJS) pcre.so
