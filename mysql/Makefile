CFLAGS += -O3 -g -c -fPIC -I../../lemon/src
CFLAGS += `mysql_config --cflags`

LDFLAGS += -shared -rdynamic -L../../lemon -llemon

SRCS  = mysql.c
SRCS += cursor.c
SRCS += connection.c

OBJS = $(SRCS:.c=.o)
INCS := $(wildcard *.h)

all: mysql.so

mysql.so: $(OBJS) $(INCS) Makefile
	$(CC) $(LDFLAGS) $(OBJS) `mysql_config --libs` -o $@

%.o: %.c
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo CC $<

clean:
	rm $(OBJS) mysql.so
