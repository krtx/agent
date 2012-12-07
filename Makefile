
CC = g++
CFLAGS = -Wall
OBJS = QuadProg++.o Array.o main.o
PROGRAM = main

all: $(PROGRAM)

$(PROGRAM): $(OBJS)
						$(CC) $(OBJS) -o $(PROGRAM)

clean:; rm -f *.o *~ $(PROGRAM)
