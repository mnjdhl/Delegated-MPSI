#!/bin/sh
rm blake3*.o libblake3.so*
gcc -shared -O3 -fPIC -o libblake3.so blake3.c blake3_dispatch.c blake3_portable.c \
    blake3_sse2_x86-64_unix.S blake3_sse41_x86-64_unix.S blake3_avx2_x86-64_unix.S \
    blake3_avx512_x86-64_unix.S
sudo cp libblake3.so /usr/lib/
sudo cp blake3.h /usr/include/
