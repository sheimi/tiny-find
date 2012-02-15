CC=gcc
OBJS=filter.o tinyfind.o parser.o
TARGET=tinyfind
CFLAG = -Wall -g

tinyfind: filter.o tinyfind.o parser.o
	$(CC) $(OBJS) -o $(TARGET) 

tinyfind.o: tinyfind.c
	$(CC)  $(CFLAG) -c $<

filter.o: filter.c 
	$(CC) $(CFLAG) -c $<

parser.o: parser.c
	$(CC) $(CFLAG) -c $<

clean:
	rm -f *.o
	rm -f $(TARGET)
