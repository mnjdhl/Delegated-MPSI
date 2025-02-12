#!/bin/sh
rm delegated_mpsi secret_sharing_simd.o approx_mpsi.o Channels.o FullMesh.o #test_secret_sharing.o
g++ -c hash_funcs.cpp -o hash_funcs.o -std=c++17 -g
g++ -msse4.2 -c secret_sharing_simd.cpp  -o secret_sharing_simd.o -std=c++17 -g
g++ -c Channels.cpp -o Channels.o -std=c++17 -g
g++ -c FullMesh.cpp -o FullMesh.o -std=c++17 -g
g++ -c Set.cpp -o Set.o -std=c++17 -g -I/usr/lib/include/
g++ -c approx_mpsi.cpp -o approx_mpsi.o -std=c++17 -g -I/usr/lib/include/

# -L/usr/lib/x86_64-linux-gnu/
g++ -o delegated_mpsi main.cpp secret_sharing_simd.o approx_mpsi.o Channels.o FullMesh.o Set.o hash_funcs.o -g -lblake3 -lboost_program_options -lssl3 -lcrypto -lsodium -I/usr/lib/include/ -L/usr/bin/lib/ -std=c++17
#-L/data/MPSI_Bay/boost_1_87_0/stage/lib/
#g++ -c test_secret_sharing.cpp -o test_secret_sharing.o
#For test...
#g++ -o test_secret_sharing secret_sharing_simd.o test_secret_sharing.o
