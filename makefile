CC = gcc


CFLAGS = -Wall -g


TARGET = shell


SRCS = shell_program.c


OBJS = $(SRCS:.c=.o)


all: $(TARGET)


$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)


%.o: %.c
	$(CC) $(CFLAGS) -c $<


clean:
	rm -f $(OBJS) $(TARGET)


run: $(TARGET)
	./$(TARGET)
