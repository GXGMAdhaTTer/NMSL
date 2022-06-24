SRCS:= $(wildcard *.c)
OBJS:= $(patsubst %.c,%.o,$(SRCS))
CC:= gcc
main: $(OBJS)
	$(CC) $^ -o $@ -pthread -g -lssl -lmysqlclient
%.o:%.c
	$(CC) -c $^ -o $@ -g
clean:
	$(RM) $(OBJS) main
rebuild:clean main