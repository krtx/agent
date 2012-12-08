
CC = g++
CFLAGS = -Wall -lboost_program_options
OBJS = QuadProg++.o Array.o kernel.o main.o
PROGRAM = main

all: $(PROGRAM)

$(PROGRAM): $(OBJS)
						$(CC) $(CFLAGS) $(OBJS) -o $(PROGRAM)

clean:; rm -f *.o *~ $(PROGRAM)
