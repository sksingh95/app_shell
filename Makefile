CC = gcc
INC_DIR = ./code/

SRCS = ./code/app_shell.c

OBJS := $(SRCS:%.c=%.o)

app_shell :$(OBJS) main.o 
	$(CC) $(OBJS) main.o -o app_shell
	
%.o : %.c
	$(CC) -c -I $(INC_DIR) $^ -o $@
	
clean:
	rm *.o
	rm ./code/*.o
	rm app_shell