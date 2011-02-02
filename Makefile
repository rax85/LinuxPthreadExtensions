##
# Makefile for the pthread extensions library.
#
# Author : Rakesh Iyer.
##

CC=gcc
COVOPTS=-fprofile-arcs -ftest-coverage
PROFOPTS=-pg
COPTS=-g -O2 -Wall -fpic -c $(COVOPTS) $(PROFOPTS)
AR=ar
AROPTS=rcs


all : dynamiclib staticlib

dynamiclib : libpthreadext.so.1.0.1

staticlib : libpthreadext.a

libpthreadext.a : pthreadExtObjs
	$(AR) $(AROPTS) libpthreadext.a *.o

libpthreadext.so.1.0.1 : pthreadExtObjs
	$(CC) -shared -Wl,-soname,libpthreadext.so.1 -o libpthreadext.so.1.0.1 *.o -lc

pthreadExtObjs : sem.o threadpool.o mempool.o pcQueue.o tcpserver.o treemap.o

threadpool.o : threadPool.c threadPool.h sem.o
	$(CC) $(COPTS) -o threadpool.o threadPool.c

sem.o : sem.c sem.h
	$(CC) $(COPTS) -o sem.o sem.c 

mempool.o : mempool.c mempool.h
	$(CC) $(COPTS) -o mempool.o mempool.c

pcQueue.o : pcQueue.c pcQueue.h sem.o mempool.o
	$(CC) $(COPTS) -o pcQueue.o pcQueue.c

rwlock.o : rwlock.c rwlock.h
	$(CC) $(COPTS) -o rwlock.o rwlock.c

treemap.o : treemap.c treemap.h mempool.o rwlock.o
	$(CC) $(COPTS) -o treemap.o treemap.c

tcpserver.o : pcQueue.o threadpool.o mempool.o tcpserver.c tcpserver.h
	$(CC) $(COPTS) -o tcpserver.o tcpserver.c

documentation : Doxyfile
	doxygen Doxyfile

Doxyfile :
	doxygen -g

test : test.c staticlib
	gcc -g -o test.out test.c libpthreadext.a -lpthread -lrt -lgcov 

.PHONY clean : 
	rm -f *.a *.so.* *.o *.out *.gc*; rm -rf html; rm -rf latex; rm -f Doxyfile
