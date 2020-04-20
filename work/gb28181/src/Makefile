CC = g++
AR:=ar
CFLAGS = -std=c++11
CFLAGS += -Wall -O2 
CFLAGS += -Wno-deprecated-declarations 
CFLAGS += -Wno-sign-compare -Wno-reorder
CFLAGS += -fPIC
#CFLAGS += -g

SRC = master.cpp 
SRC +=
LDFLAGS =
LDFLAGS += -llzma -ldl -lz -lm -lpthread -lrt

all:
	$(CC) $(CFLAGS) -c *.cpp
	$(AR) rcs ../lib/libgb28181Server.a *.o 
clean:
	rm -f *.o ../lib/*

