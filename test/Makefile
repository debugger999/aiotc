CC = g++
CFLAGS = -std=c++11
CFLAGS += -Wall -O2 -Wno-deprecated-declarations -Wno-sign-compare -Wno-reorder
#CFLAGS += -g

CFLAGS += -I../inc

SRC = test.cpp
LDFLAGS =
LDFLAGS += -llzma -ldl -lz -lm -lpthread -lrt
LDFLAGS += -fPIE -Wl,-rpath,lib

all:
	$(CC) $(CFLAGS) $(SRC) $(LDFLAGS) -o test

clean:
	rm -f test

