#!/bin/bash
export LD_LIBRARY_PATH=/usr/bin/lib/:$LD_LIBRARY_PATH
BinCount=5
HashCounts=(1 2 5 10)
HashCount=${HashCounts[2]}
Latencies=(1 10 100 1000) # ms
Latency=${Latencies[2]}
OneMB=1000000 # Bytes per second
Bandwidths=($OneMB $((10 * OneMB)) $((100 * OneMB)) $((1000 * OneMB)))
BytesPerSec=${Bandwidths[2]}
Repetitions=5
file_id=$(date +%Y%m%d_%H%M)
FileName="Parties_vs_SetSizes_$file_id.txt"
SetSizes=(1024 4096 16384 65536) #2^10, 2^12, 2^14, 2^16
Parties=(3 4 5 10)
HashFuncs=(sha512 sha3_512 blake2b_512 shake256_xof blake3_xof)
HashFunc=${HashFuncs[4]}
if [ -f $FileName ] ; then
    rm $FileName
fi
for SetSize in ${SetSizes[@]}
do
    DomainSize=$((SetSize + 1))
    for PartyCount in ${Parties[@]}
    do
        msg="Running for Set Size=$SetSize, Party Count=$PartyCount and Hash Function=$HashFunc"
        echo $msg
        echo $msg >> $FileName
        cmd="../delegated_mpsi -n $PartyCount -k $SetSize -u $DomainSize -m $BinCount -s $HashCount -c $HashFunc  -l $Latency -b $BytesPerSec -r $Repetitions -f $FileName"
        TIME=$( { /usr/bin/time -f "%e" $cmd > /dev/null; } 2>&1 )
        echo $TIME
        echo "" >> $FileName
    done
done
echo "Done!"