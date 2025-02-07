#!/bin/bash

BinCount=5
HashCount=6
Latency=2
BytesPerSec=3000 #Bandwidth
Repetitions=5
file_id=$(date +%Y%m%d_%H%M)
FileName="exp_results_$file_id.txt"
SetSizes=(1024 4096 16384 65536)
Parties=(3 4 5 10)
if [ -f $FileName ] ; then
    rm $FileName
fi
for SetSize in ${SetSizes[@]}
do
    DomainSize=$((SetSize + 1)) #131072 #2^17
    for PartyCount in ${Parties[@]}
    do
        msg="Running for SetSize=$SetSize and PartyCount=$PartyCount"
        echo $msg
        echo $msg >> $FileName
        ../delegated_mpsi -n $PartyCount -k $SetSize -u $DomainSize -m $BinCount -s $HashCount -l $Latency -b $BytesPerSec -r $Repetitions -f $FileName
        echo "" >> $FileName
    done
done

#PartyCount=5
#SetSize=65536 #2^16


#iter_count=10
#for i in $(seq 1 $iter_count)
#do
#    echo "Running iteration $i"
#    ../delegated_mpsi -n $PartyCount -k $SetSize -u $DomainSize -m $BinCount -s $HashCount -l $Latency -b $BytesPerSec -r $Repetitions -f $FileName
#done