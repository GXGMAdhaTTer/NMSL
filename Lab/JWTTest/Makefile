SRCS:=$(wildcard *.c)
EXES:=$(patsubst %.c,%,$(SRCS))

all:$(EXES)

%:%.c
	gcc $^ -o $@ -pthread -g -lssl -lmysqlclient -ll8w8jwt -lmbedcrypto -lmbedx509

.PHONY:clean rebuild
clean:
	$(RM) $(EXES)
rebuild:clean all