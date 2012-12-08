
CC := g++
CFLAGS := -Wall -O2 -lboost_program_options
SRCS := Array.cc QuadProg++.cc main.cpp
OBJS := $(shell echo $(SRCS) | sed -e 's/.cc\|.cpp/.o/g')
DEPS := $(shell echo $(SRCS) | sed -e 's/.cc\|.cpp/.d/g')

PROGRAM := main

all: $(PROGRAM)

-include $(DEPS)

$(PROGRAM): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.cpp
	$(CC) $(CFLAGS) -c -MMD -MP $<

%.o: %.cc
	$(CC) $(CFLAGS) -c -MMD -MP $<

clean:; rm -f *~ $(OBJS) $(DEPS) $(PROGRAM)
