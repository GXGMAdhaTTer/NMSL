SRCS:= $(wildcard *.c)
OBJS:= $(patsubst %.c,%.o,$(SRCS))
CC:= gcc
client: $(OBJS)
	$(CC) $^ -o $@ -pthread -g -lssl -lmysqlclient -lcrypt
%.o:%.c
	$(CC) -c $^ -o $@ -g
clean:
	$(RM) $(OBJS) client
rebuild:clean client