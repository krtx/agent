
CC = g++
CFLAGS = -Wall -O2 -lboost_program_options
OBJS = QuadProg++.o Array.o main.o
PROGRAM = main

all: $(PROGRAM)

$(PROGRAM): $(OBJS)
						$(CC) $(CFLAGS) $(OBJS) -o $(PROGRAM)

clean:; rm -f *.o *~ $(PROGRAM)
