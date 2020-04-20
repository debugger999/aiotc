CC = g++
AR:=ar
CFLAGS = -std=c++11
CFLAGS += -Wall -O2 
CFLAGS += -Wno-deprecated-declarations 
CFLAGS += -Wno-sign-compare -Wno-reorder
CFLAGS += -fPIC
#CFLAGS += -g

CFLAGS += -I../inc

LDFLAGS =
LDFLAGS += -llzma -ldl -lz -lm -lpthread -lrt

all:
	$(CC) $(CFLAGS) -c *.c
	$(AR) rcs ../lib/libcjson.a *.o 
clean:
	rm -f *.o ../lib/*

