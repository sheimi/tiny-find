CC=gcc
OBJS=filter.o main.o
TARGET=main
CFLAG = -Wall

main: filter.o main.o
	$(CC) $(OBJS) -o $(TARGET) 

main.o: main.c
	$(CC)  $(CFLAG) -c $<

filter.o: filter.c
	$(CC) $(CFLAG) -c $<

clean:
	rm -f *.o
	rm -f $(TARGET)
