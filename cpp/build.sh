#!/bin/sh
rm secret_sharing_simd.o approx_mpsi.o #test_secret_sharing.o
g++ -msse4.2 -c secret_sharing_simd.cpp  -o secret_sharing_simd.o -lblake3
g++ -c approx_mpsi.cpp -o approx_mpsi.o
g++ -o main main.cpp secret_sharing_simd.o approx_mpsi.o -lblake3 -lboost_program_options -I/usr/lib/include/ -L/usr/bin/lib/
#-L/data/MPSI_Bay/boost_1_87_0/stage/lib/
#g++ -c test_secret_sharing.cpp -o test_secret_sharing.o
#For test...
#g++ -o test_secret_sharing secret_sharing_simd.o test_secret_sharing.o
