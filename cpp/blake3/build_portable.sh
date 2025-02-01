#!/bin/sh

rm blake3*.o libblake3.so*
gcc -shared -O3 -fPIC -o libblake3.so -DBLAKE3_NO_SSE2 -DBLAKE3_NO_SSE41 -DBLAKE3_NO_AVX2 \
    -DBLAKE3_NO_AVX512 blake3.c blake3_dispatch.c blake3_portable.c
sudo cp libblake3.so /usr/lib/
sudo cp blake3.h /usr/include/
