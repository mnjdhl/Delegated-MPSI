#!/bin/bash
#Running feather parties (number of clients) vs setsize
export LD_LIBRARY_PATH=/usr/bin/lib/:$LD_LIBRARY_PATH

Repetitions=5
file_id=$(date +%Y%m%d_%H%M)
FileName="Feather_Parties_vs_SetSizes_$file_id.txt"
SetSizes=(1024 4096 16384 65536) #2^10, 2^12, 2^14, 2^16
Parties=(2 3 4 5 10)

#exp_nums=1  #Number of experiments
exp_nums=$Repetitions
#client_nums=2   #Number of clients
pub_mode_bitsize=40 #Public mod bitsize
#max_set_size=1024   #Max set size
#table_len=30    #Table length
bucket_max_load=100 #Bucket max load
#intersect_size=1    #Intersection size
x_size=201  #X size
output_file="MileStone9_Feather_Parties_vs_SetSizes_h_$HashCount_$file_id.txt"
#echo "Running for Pub Mode Bit Size=$pub_mode_bitsize, Bucket Max Load=$bucket_max_load, Intersection Size=$intersect_size, X Size=$x_size and Repetitions=$exp_nums">>$output_file
echo "Running for Pub Mode Bit Size=$pub_mode_bitsize, Bucket Max Load=$bucket_max_load, X Size=$x_size and Repetitions=$exp_nums">>$output_file

echo "Set Size (Max), Intersection Size, Pary Count (Number of clients), Table Length, Time (s)" >> $output_file
for max_set_size in ${SetSizes[@]}
do
    intersect_size=$max_set_size
    for client_nums in ${Parties[@]}
    do
        #h=4*c/d
        #table_len=$(( (4*max_set_size)/bucket_max_load ))
        table_len=$(( (((4*max_set_size)/bucket_max_load )*9)/10))
        cmd="../test -n $exp_nums -c $client_nums -u $pub_mode_bitsize -k $max_set_size -m $table_len -s $bucket_max_load -i $intersect_size -x $x_size"
        echo $cmd
        TIME=$( { /usr/bin/time -f "%e" $cmd > /dev/null; } 2>&1 )
        avg_time=$(jq -n $TIME/$exp_nums)
        msg="$max_set_size, $intersect_size, $client_nums, $table_len, $avg_time"
        echo $msg>>$output_file
    done
done
echo "Done!"



