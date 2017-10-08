CFLAGS += -O3 -g -c -fPIC -I../../lemon/src
CFLAGS += `mysql_config --cflags`

LDFLAGS += -shared -L../../lemon -llemon
LDFLAGS += `mysql_config --libs`

SRCS  = mysql.c
SRCS += cursor.c
SRCS += connection.c

OBJS = $(SRCS:.c=.o)
INCS := $(wildcard *.h)

all: mysql.so

mysql.so: $(OBJS) $(INCS) Makefile
	cc $(LDFLAGS) $(OBJS) -o $@


%.o: %.c
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo CC $<

clean:
	rm $(OBJS) mysql.so
