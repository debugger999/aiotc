CC = g++
CFLAGS = -std=c++11
CFLAGS += -Wall -O2 -Wno-deprecated-declarations -Wno-sign-compare -Wno-reorder
CFLAGS += -g

CFLAGS += -I../inc
CFLAGS += -I../work/share/inc
CFLAGS += -I../work/slab/inc
CFLAGS += -I../work/libevent/inc
CFLAGS += -I../work/mongodb/inc
CFLAGS += -I../work/rtsp/inc
CFLAGS += -I../work/preview/inc
CFLAGS += -Ialg/face
CFLAGS += -Istream -Irtsp -Igb28181 -Igat1400 -Iftp -Ihik/ehome -Ihik/sdk

SRC = main.cpp
SRC += master.cpp
SRC += platform.cpp
SRC += work.cpp
SRC += decode.cpp
SRC += record.cpp
SRC += preview.cpp
SRC += msg.cpp
SRC += rest.cpp db.cpp 
SRC += misc.cpp shm.cpp pids.cpp
SRC += stream/stream.cpp
SRC += rtsp/rtsp.cpp
SRC += hik/ehome/ehome.cpp
SRC += alg/face/face.cpp

LDFLAGS = -L../work/share/lib
LDFLAGS += -L../work/cjson/lib
LDFLAGS += -L../work/slab/lib
LDFLAGS += -L../work/libevent/lib
LDFLAGS += -L../work/mongodb/lib
LDFLAGS += -L../work/rtsp/lib
LDFLAGS += -L../work/lib
LDFLAGS += -L../work/preview/lib
LDFLAGS += -lplayer -lliveMedia -lgroupsock -lBasicUsageEnvironment -lUsageEnvironment
LDFLAGS += -lshare -lcjson -lslab -lpreview
LDFLAGS += -lmongoc -lbson
LDFLAGS += -levent_pthreads -levent
LDFLAGS += -lcrypto -lssl -lresolv
LDFLAGS += -llzma -ldl -lz -lm -lpthread -lrt
LDFLAGS += -fPIE -Wl,-rpath,lib

all:
	$(CC) $(CFLAGS) $(SRC) $(LDFLAGS) -o aiotcProc

clean:
	rm -f aiotcProc

