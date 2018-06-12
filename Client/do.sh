#!/bin/bash
ls
pwd
cd /opt/
rm CMakeCache.txt
cmake .
make
./client server 4097 /opt/haarcascade/cascade.xml
