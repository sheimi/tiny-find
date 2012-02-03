CC=gcc
OBJS=filter.o main.o
TARGET=main


main: filter.o main.o
	$(CC) $(OBJS) -o $(TARGET) 

main.o: main.c
	$(CC) -c $<

filter.o: filter.c
	$(CC) -c $<

clean:
	rm -f *.o
	rm -f $(TARGET)
