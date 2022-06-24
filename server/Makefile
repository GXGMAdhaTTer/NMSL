SRCS:= $(wildcard *.c)
OBJS:= $(patsubst %.c,%.o,$(SRCS))
CC:= gcc
server: $(OBJS)
	$(CC) $^ -o $@ -pthread -g -lssl -lmysqlclient -lcrypt -ll8w8jwt -lmbedcrypto -lmbedx509
%.o:%.c
	$(CC) -c $^ -o $@ -g
clean:
	$(RM) $(OBJS) server
rebuild:clean server