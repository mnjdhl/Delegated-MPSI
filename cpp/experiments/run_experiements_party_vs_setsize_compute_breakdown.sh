#!/bin/bash
export LD_LIBRARY_PATH=/usr/bin/lib/:$LD_LIBRARY_PATH
BinCount=5
#HashCounts=(1 2 5 10)
HashCounts=(1 5 10)
#HashCount=${HashCounts[0]}
Latencies=(1 10 100 1000) # ms
Latency=${Latencies[2]}
OneMB=1000000 # Bytes per second
Bandwidths=($OneMB $((10 * OneMB)) $((100 * OneMB)) $((1000 * OneMB)))
BytesPerSec=${Bandwidths[2]}
Repetitions=5
file_id=$(date +%Y%m%d_%H%M)
FileName="Compute_Breakdown_Parties_vs_SetSizes_$file_id.txt"
SetSizes=(1024 4096 16384 65536) #2^10, 2^12, 2^14, 2^16
Parties=(2 3 4 5 10)
HashFuncs=(sha512 sha3_512 blake2b_512 shake256_xof blake3_xof)
HashFunc=${HashFuncs[4]}
if [ -f $FileName ] ; then
    rm $FileName
fi

Latency=0
BytesPerSec=0
output_file="MileStone10_Fig2_Parties_vs_SetSizes_h_$HashCount_$file_id.txt"
echo "Running for Bandwith=$BytesPerSec, Latency=$Latency, Hash Function=$HashFunc and Repetitions=$Repetitions">>$output_file
echo "Set Size, Pary Count, Hash Count, Time (s)" >> $output_file
for HashCount in ${HashCounts[@]}
do
    for SetSize in ${SetSizes[@]}
    do
        DomainSize=$((SetSize + 1))
        for PartyCount in ${Parties[@]}
        do
            pc=$((PartyCount + 2))
            #msg="Running for Set Size=$SetSize, Party Count=$PartyCount and Hash Function=$HashFunc"
            cmd="../delegated_mpsi -n $pc -k $SetSize -u $DomainSize -m $BinCount -s $HashCount -c $HashFunc  -l $Latency -b $BytesPerSec -r $Repetitions -f $FileName -t 1"
            echo $cmd
            TIME=$( { /usr/bin/time -f "%e" $cmd > /dev/null; } 2>&1 )
            avg_time=$(jq -n $TIME/$Repetitions)
            msg="$SetSize, $PartyCount, $HashCount, $avg_time"
            echo $msg>>$output_file
        done
    done
done
echo "Done!"