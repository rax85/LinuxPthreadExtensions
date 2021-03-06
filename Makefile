##
# Makefile for the pthread extensions library.
#
# Author : Rakesh Iyer.
##

CC=gcc
COVOPTS=-fprofile-arcs -ftest-coverage
PROFOPTS=-pg
USE_PREFETCH=-DUSE_PREFETCH
USE_PREDICTOR_HINTS=-DUSE_PREDICTOR_HINTS
USE_SSE=-msse -msse2
# Use this line for debug builds.
# COPTS=-g -O0 -Wall -fpic -c $(COVOPTS) $(PROFOPTS) $(USE_PREFETCH) $(USE_PREDICTOR_HINTS)
# Use this line for non-debug builds.
COPTS=-O2 -s -c $(USE_PREFETCH) $(USE_PREDICTOR_HINTS) $(USE_SSE)
AR=ar
AROPTS=rcs


all : dynamiclib staticlib

dynamiclib : libpthreadext.so.1.0.1

staticlib : libpthreadext.a

libpthreadext.a : pthreadExtObjs
	$(AR) $(AROPTS) libpthreadext.a *.o

libpthreadext.so.1.0.1 : pthreadExtObjs
	$(CC) -shared -Wl,-soname,libpthreadext.so.1 -o libpthreadext.so.1.0.1 *.o -lc

pthreadExtObjs : sem.o threadpool.o mempool.o pcQueue.o tcpserver.o treemap.o arraylist.o fileio.o

threadpool.o : threadPool.c threadPool.h sem.o asmopt.h
	$(CC) $(COPTS) -o threadpool.o threadPool.c

sem.o : sem.c sem.h asmopt.h
	$(CC) $(COPTS) -o sem.o sem.c 

mempool.o : mempool.c mempool.h asmopt.h
	$(CC) $(COPTS) -o mempool.o mempool.c

pcQueue.o : pcQueue.c pcQueue.h sem.o mempool.o asmopt.h
	$(CC) $(COPTS) -o pcQueue.o pcQueue.c

rwlock.o : rwlock.c rwlock.h sem.o asmopt.h
	$(CC) $(COPTS) -o rwlock.o rwlock.c

treemap.o : treemap.c treemap.h mempool.o rwlock.o asmopt.h
	$(CC) $(COPTS) -o treemap.o treemap.c

arraylist.o : arraylist.c arraylist.h asmopt.h mempool.o rwlock.o
	$(CC) $(COPTS) -o arraylist.o arraylist.c 

tcpserver.o : pcQueue.o threadpool.o mempool.o tcpserver.c tcpserver.h asmopt.h
	$(CC) $(COPTS) -o tcpserver.o tcpserver.c

fileio.o : fileio.h fileio.c mempool.o
	$(CC) $(COPTS) -o fileio.o fileio.c

documentation : Doxyfile
	doxygen Doxyfile

Doxyfile :
	doxygen -g

test : test.c staticlib
	gcc -g -o test.out test.c libpthreadext.a -lpthread -lrt -lgcov -pg 

.PHONY clean : 
	rm -f *.a *.so.* *.o *.out *.gc*; rm -rf html; rm -rf latex; rm -f Doxyfile
