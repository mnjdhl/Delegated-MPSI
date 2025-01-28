#!/bin/sh
rm secret_sharing_simd.o approx_mpsi.o #test_secret_sharing.o
g++ -msse4.2 -c secret_sharing_simd.cpp  -o secret_sharing_simd.o
g++ -c approx_mpsi.cpp -o approx_mpsi.o
g++ -c test_secret_sharing.cpp -o test_secret_sharing.o -I/data/MPSI_Bay/googletest-1.15.2/googletest/include/gtest/
g++ -o test_secret_sharing secret_sharing_simd.o test_secret_sharing.o approx_mpsi.o -lgtest -lblake3
