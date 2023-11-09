CC = gcc
CFLAGS = -std=c99 -Wall -pedantic -fpic

all: mxarr.o libmxarr.so mxarr_wrap.c mxarr.py _mxarr.so

mxarr.o: mxarr.c mxarr.h
	$(CC) $(CFLAGS) -c mxarr.c -o mxarr.o

libmxarr.so: mxarr.o
	$(CC) $(CFLAGS) -shared mxarr.o -o libmxarr.so

mxarr_wrap.c: mxarr.i
	swig -python -o mxarr_wrap.c mxarr.i

mxarr.py: mxarr_wrap.c

mxarr_wrap.o: mxarr_wrap.c
	$(CC) $(CFLAGS) -c mxarr_wrap.c -I/usr/include/python3.10

_mxarr.so: mxarr_wrap.o libmxarr.so
	$(CC) $(CFLAGS) -shared mxarr_wrap.o -o _mxarr.so -L. -lmxarr -lm

clean:
	rm -f mxarr.o libmxarr.so mxarr_wrap.c mxarr.py mxarr_wrap.o _mxarr.so
