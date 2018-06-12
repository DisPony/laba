#!/bin/bash
ls
pwd
cd /opt/
rm CMakeCache.txt
cmake .
make
./server
