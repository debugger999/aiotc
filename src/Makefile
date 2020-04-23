CC = g++
CFLAGS = -std=c++11
CFLAGS += -Wall -O2 -Wno-deprecated-declarations -Wno-sign-compare -Wno-reorder
CFLAGS += -g

CFLAGS += -I../inc
CFLAGS += -I../work/share/inc
CFLAGS += -I../work/slab/inc
CFLAGS += -I../work/libevent/inc
CFLAGS += -I../work/mongodb/inc
CFLAGS += -Istream -Irtsp -Igb28181 -Igat1400 -Iftp -Ihik/ehome -Ihik/sdk

SRC = main.cpp
SRC += master.cpp
SRC += platform.cpp
SRC += work.cpp
SRC += rest.cpp db.cpp misc.cpp shm.cpp pids.cpp
SRC += stream/stream.cpp

LDFLAGS = -L../work/share/lib
LDFLAGS += -L../work/cjson/lib
LDFLAGS += -L../work/slab/lib
LDFLAGS += -L../work/libevent/lib
LDFLAGS += -L../work/mongodb/lib
LDFLAGS += -lshare -lcjson -lslab
LDFLAGS += -lmongoc -lbson
LDFLAGS += -levent_pthreads -levent
LDFLAGS += -lcrypto -lssl -lresolv
LDFLAGS += -llzma -ldl -lz -lm -lpthread -lrt
LDFLAGS += -fPIE -Wl,-rpath,lib

all:
	$(CC) $(CFLAGS) $(SRC) $(LDFLAGS) -o aiotcProc

clean:
	rm -f aiotcProc

