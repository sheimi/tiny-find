CC=gcc
OBJS=filter.o tinyfind.o parser.o
TARGET=tinyfind
CFLAGS = -Wall
ifneq ($(DEBUG),)
  CFLAGS += -g -DDEBUG
else
  CFLAGS += -O2
endif
tinyfind: filter.o tinyfind.o parser.o
	$(CC) $(OBJS) -o $(TARGET) 

tinyfind.o: tinyfind.c
	$(CC)  $(CFLAGS) -c $<

filter.o: filter.c 
	$(CC) $(CFLAGS) -c $<

parser.o: parser.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o
	rm -f $(TARGET)
