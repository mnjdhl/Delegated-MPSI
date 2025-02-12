#!/bin/bash
export LD_LIBRARY_PATH=/usr/bin/lib/:$LD_LIBRARY_PATH
BinCount=5
HashCount=6
Latency=2
BytesPerSec=3000 #Bandwidth
Repetitions=5
file_id=$(date +%Y%m%d_%H%M)
FileName="exp_results_$file_id.txt"
SetSizes=(1024 4096 16384 65536)
Parties=(3 4 5 10)
HashFuncs=(sha512 sha3_512 blake2b_512 shake256_xof blake3_xof)
HashFunc=HashFuncs[4]
if [ -f $FileName ] ; then
    rm $FileName
fi
for SetSize in ${SetSizes[@]}
do
    DomainSize=$((SetSize + 1)) #131072 #2^17
    for PartyCount in ${Parties[@]}
    do
        msg="Running for Set Size=$SetSize, Party Count=$PartyCount and Hash Function=$HashFunc"
        echo $msg
        echo $msg >> $FileName
        ../delegated_mpsi -n $PartyCount -k $SetSize -u $DomainSize -m $BinCount -s $HashCount \
            -c $HashFunc  -l $Latency -b $BytesPerSec -r $Repetitions -f $FileName
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