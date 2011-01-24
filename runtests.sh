#!/bin/bash

echo "Running tests under Valgrind to check for memory leaks..."
./leakcheck.sh

echo "Running tests under gcov to check for test coverage..."
./coverage.sh

