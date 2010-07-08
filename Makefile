##
# Makefile for the pthread extensions library.
#
# Author : Rakesh Iyer.
##

CC=gcc
COPTS=-g -Wall -fpic -c
AR=ar
AROPTS=rcs


all : dynamiclib staticlib

dynamiclib : libpthreadext.so.1.0.1

staticlib : libpthreadext.a

libpthreadext.a : pthreadExtObjs
	$(AR) $(AROPTS) libpthreadext.a *.o

libpthreadext.so.1.0.1 : pthreadExtObjs
	$(CC) -shared -Wl,-soname,libpthreadext.so.1 -o libpthreadext.so.1.0.1 *.o -lc

pthreadExtObjs : sem.o threadpool.o

threadpool.o : threadPool.c threadPool.h sem.o
	$(CC) $(COPTS) -o threadPool.o threadPool.c

sem.o : sem.c sem.h
	$(CC) $(COPTS) -o sem.o sem.c 

documentation : Doxyfile
	doxygen Doxyfile

Doxyfile :
	doxygen -g

.PHONY clean : 
	rm -f *.a *.so.* *.o; rm -rf html; rm -rf latex; rm -f Doxyfile
