#NAME: Sean Langley
#EMAIL: sean.langley22@gmail.com
#ID: 504 661 838

#!/bin/sh

rm lab2b_list.csv
touch lab2b_list.csv


#Running with yielding and no synchronization to check failure
threads=(1 4 8 12 16)
iterations=(1 2 4 8 16)
iterations1=(10 20 40 80)

for i in ${threads[@]}
do
    for j in ${iterations[@]}
    do
	echo "./lab2_list --threads=$i --iterations=$j --lists=4 --yield=id"
	./lab2_list --threads=$i --iterations=$j --lists=4 --yield=id >> lab2b_list.csv
    done
done

#Running with yielding and synchronization to check success
iterations=(10 20 40 80)
for i in ${threads[@]}
do
    for j in ${iterations[@]}
    do
	echo "./lab2_list --threads=$i --iterations=$j --lists=4 --yield=id --sync=m"
	./lab2_list --threads=$i --iterations=$j --lists=4 --yield=id --sync=m >> lab2b_list.csv
    done
done
for i in ${threads[@]}
do
    for j in ${iterations[@]}
    do
	echo "./lab2_list --threads=$i --iterations=$j --lists=4 --yield=id --sync=s"
	./lab2_list --threads=$i --iterations=$j --lists=4 --yield=id --sync=s >> lab2b_list.csv
    done
done	

#Running with 1000 iterations and synchronization
threads=( 1 2 4 8 12 16 24)
iterations=1000

for i in ${threads[@]}
do    
    echo "./lab2_list --threads=$i --iterations=1000 --sync=m"
    ./lab2_list --threads=$i --iterations=$iterations --sync=m >> lab2b_list.csv
done


for i in ${threads[@]}
do    
    echo "./lab2_list --threads=$i --iterations=1000 --sync=s"
    ./lab2_list --threads=$i --iterations=$iterations --sync=s >> lab2b_list.csv
done

#Running with sublists to measure performance enhancement
threads=(1 2 4 8 12)
lists=(1 4 8 16)
for i in ${threads[@]}
do
    for j in ${lists[@]}
    do
	echo "./lab2_list --threads=$i --lists=$j --iterations=1000 --sync=m"
	./lab2_list --threads=$i --lists=$j --iterations=1000 --sync=m >> lab2b_list.csv
    done
done

for i in ${threads[@]}
do
    for j in ${lists[@]}
    do
	echo "./lab2_list --threads=$i --lists=$j --iterations=1000 --sync=s"
	./lab2_list --threads=$i --lists=$j --iterations=1000 --sync=s >> lab2b_list.csv
    done
done






    
