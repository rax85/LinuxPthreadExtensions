#!/bin/bash

make test
valgrind --leak-check=full --track-origins=yes -v ./test.out

