#!/bin/bash
export LD_LIBRARY_PATH=/usr/bin/lib/:$LD_LIBRARY_PATH
BinCount=5
HashCounts=(1 2 5 10)
HashCount=${HashCounts[3]}
Latencies=(1 10 100 1000) # ms
Latency=${Latencies[2]}
OneMB=1000000 # Bytes per second
#Bandwidths=(10000 100000 $OneMB $((10 * OneMB)) $((100 * OneMB)) $((1000 * OneMB)))
Bandwidths=($OneMB $((10 * OneMB)) $((100 * OneMB)) $((1000 * OneMB)))
BytesPerSec=${Bandwidths[2]}
Repetitions=5
file_id=$(date +%Y%m%d_%H%M)
FileName="exp_results_$file_id.txt"
SetSizes=(1024 4096 16384 65536 1048576) #2^10, 2^12, 2^14, 2^16, 2^20
Parties=(2 3 4 5 10)
HashFuncs=(sha512 sha3_512 blake2b_512 shake256_xof blake3_xof)
HashFunc=${HashFuncs[4]}
if [ -f $FileName ] ; then
    rm $FileName
fi
SetSize=${SetSizes[2]}
#SetSize=${SetSizes[0]}
#SetSize=${SetSizes[4]}
#PartyCount=20
#PartyCount=4
PartyCount=10
DomainSize=$((SetSize + 1))
output_file="MileStone5_Fig3_bandwidth_vs_latency_$file_id.txt"
pc=$((PartyCount + 1))
echo "Running for Set Size=$SetSize, Party Count=$pc, Hash Count=$HashCount, Hash Function=$HashFunc and Repetitions=$Repetitions">>$output_file
echo "Latency (ms),Bandwidth (Bytes/sec),Time (s)" >> $output_file

for BytesPerSec in ${Bandwidths[@]}
do
    for Latency in ${Latencies[@]}
    do
        #msg="Running for Set Size=$SetSize, Party Count=$PartyCount and Hash Function=$HashFunc"
        #echo $msg
        #echo $msg >> $FileName
        cmd="../delegated_mpsi -n $pc -k $SetSize -u $DomainSize -m $BinCount -s $HashCount -c $HashFunc  -l $Latency -b $BytesPerSec -r $Repetitions -f $FileName"
        echo $cmd
        TIME=$( { /usr/bin/time -f "%e" $cmd > /dev/null; } 2>&1 )
        #echo $TIME
        #echo "" >> $FileName
        #avg_time=$((TIME / Repetitions))
        avg_time=$(jq -n $TIME/$Repetitions)
        msg="$Latency,$BytesPerSec,$avg_time"
        echo $msg>>$output_file
    done
done
echo "Done!"