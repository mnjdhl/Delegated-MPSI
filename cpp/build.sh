#!/bin/sh
rm delegated_mpsi secret_sharing_simd.o approx_mpsi.o Channels.o FullMesh.o #test_secret_sharing.o
g++ -msse4.2 -c secret_sharing_simd.cpp  -o secret_sharing_simd.o -std=c++17 -g
g++ -c Channels.cpp -o Channels.o -std=c++17 -g
g++ -c FullMesh.cpp -o FullMesh.o -std=c++17 -g
g++ -c Set.cpp -o Set.o -std=c++17 -g
g++ -c approx_mpsi.cpp -o approx_mpsi.o -std=c++17 -g
g++ -o delegated_mpsi main.cpp secret_sharing_simd.o approx_mpsi.o Channels.o FullMesh.o Set.o -g -lblake3 -lboost_program_options -I/usr/lib/include/ -L/usr/bin/lib/ -std=c++17
#-L/data/MPSI_Bay/boost_1_87_0/stage/lib/
#g++ -c test_secret_sharing.cpp -o test_secret_sharing.o
#For test...
#g++ -o test_secret_sharing secret_sharing_simd.o test_secret_sharing.o
