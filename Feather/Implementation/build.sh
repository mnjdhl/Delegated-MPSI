#!/bin/sh

rm *.o
rm test

g++ -g -c Rand.cpp -c Hashtable.cpp -c Polynomial.cpp -c Server.cpp -c Client.cpp -I/usr/lib/include/ -I/home/ws3/mpsi_bay/Feather/deps/

g++ -g Rand.o Hashtable.o Polynomial.o Server.o Client.o test.cpp  -o test -I/usr/lib/include/ -I/home/ws3/mpsi_bay/Feather/deps/ -lboost_program_options -L/usr/bin/lib/  -lntl -lgmpxx -lgmp -lcryptopp
