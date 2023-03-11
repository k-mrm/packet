CC = gcc
LD = ld

CFLAGS += -Wall -Wextra -I ../ -Og
OBJS += ../net.o

.PHONY: target clean

target: $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) *.out $(OBJS) $(TARGET)
