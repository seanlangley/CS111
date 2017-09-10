#!/bin/bash

rm -rf csvfiles
mkdir csvfiles

cd csvfiles
for i in {0..22}
do

    wget http://web.cs.ucla.edu/classes/spring17/cs111/projects/P3B-test_$i.csv
    wget http://web.cs.ucla.edu/classes/spring17/cs111/projects/P3B-test_$i.err
done
    
