#!/bin/bash
export LD_LIBRARY_PATH=/usr/bin/lib/:$LD_LIBRARY_PATH
BinCount=5
#HashCounts=(1 2 5 10)
HashCounts=(1 10)
HashCount=${HashCounts[2]}
Latencies=(1 10 100 1000) # ms
#Latency=${Latencies[2]}
Latency=0
OneMB=1000000 # Bytes per second
Bandwidths=($OneMB $((10 * OneMB)) $((100 * OneMB)) $((1000 * OneMB)))
#BytesPerSec=${Bandwidths[2]}
BytesPerSec=0
Repetitions=5
file_id=$(date +%Y%m%d_%H%M)
FileName="exp_results_$file_id.txt"
SetSizes=(1024 4096 16384 65536) #2^10, 2^12, 2^14, 2^16
Parties=(2 3 4 5 10)
HashFuncs=(shake128_xof shake256_xof blake3_xof)
HashFunc=${HashFuncs[4]}
if [ -f $FileName ] ; then
    rm $FileName
fi
output_file="MileStone5_HashFuncs_$file_id.txt"
echo "Running for Latency=$Latency, Bandwidth=$BytesPerSec and Repetitions=$Repetitions">>$output_file
echo "Set Size, Pary Count, Hash Function, Hash Count, Time (s)" >> $output_file
for SetSize in ${SetSizes[@]}
do
    DomainSize=$((SetSize + 1))
    for PartyCount in ${Parties[@]}
    do
        pc=$((PartyCount + 1))
        for HashFunc in ${HashFuncs[@]}
        do
            for HashCount in ${HashCounts[@]}
            do
                cmd="../delegated_mpsi -n $pc -k $SetSize -u $DomainSize -m $BinCount -s $HashCount -c $HashFunc  -l $Latency -b $BytesPerSec -r $Repetitions -f $FileName"
                echo $cmd
                TIME=$( { /usr/bin/time -f "%e" $cmd > /dev/null; } 2>&1 )
                NTIME=$(echo $TIME | tr -d -c 0-9.)
                echo "Prev TIME $TIME , Time Duration $NTIME , Repetitions $Repetitions"
                avg_time=$(jq -n $NTIME/$Repetitions)
                msg="$SetSize, $pc, $HashFunc, $HashCount, $avg_time"
                echo $msg>>$output_file
            done
        done
    done
done
echo "Done!"