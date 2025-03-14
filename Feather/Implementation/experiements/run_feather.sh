#!/bin/sh
#Script to run Feather experiments
export LD_LIBRARY_PATH=/usr/bin/lib/:$LD_LIBRARY_PATH

exp_nums=1  #Number of experiments
client_nums=2   #Number of clients
pub_mode_bitsize=40 #Public mod bitsize
max_set_size=1024   #Max set size
table_len=30    #Table length
bucket_max_load=100 #Bucket max load
intersect_size=1    #Intersection size
x_size=201  #X size
h=0
for [[ $h -le 10 ]]
do
    cmd="../test -n $exp_nums -c $client_nums -u $pub_mode_bitsize -k $max_set_size -m $table_len -s $bucket_max_load -i $intersect_size -x $x_size"
    echo $cmd
    TIME=$( { /usr/bin/time -f "%e" $cmd > /dev/null; } 2>&1 )
    echo "Total time it took was $TIME"
    #NTIME=$(echo $TIME | tr -d -c 0-9.)
    #echo "Prev TIME $TIME , Time Duration $NTIME , Repetitions $Repetitions"
    #avg_time=$(jq -n $NTIME/$Repetitions)
    h=$(( h + 1))
done