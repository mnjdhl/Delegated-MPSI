#!/bin/sh

PartyCount=5
SetSize=65536 #2^16
DomainSize=131072 #2^17
BinCount=5
HashCount=6
Latency=2
BytesPerSec=3000 #Bandwidth
Repetitions=5
FileName="test1.txt"

iter_count=10
rm $FileName
for i in $(seq 1 $iter_count)
do
    echo "Running iteration $i"
    ../delegated_mpsi -n $PartyCount -k $SetSize -u $DomainSize -m $BinCount -s $HashCount -l $Latency -b $BytesPerSec -r $Repetitions -f $FileName
done