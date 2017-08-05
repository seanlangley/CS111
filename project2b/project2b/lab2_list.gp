#NAME: Sean Langley
#EMAIL: sean.langley22@gmail.com
#ID: 504 661 838

#! /usr/bin/gnuplot

set terminal png
set datafile separator ","
set style line 1 lc rgb 'blue' lt 1 lw 2 pt 7 pi -1 ps 1.5
set style line 2 lc rgb 'red' lt 1 lw 2 pt 7 pi -1 ps 1.5
set style line 2 lc rgb 'green' lt 1 lw 2 pt 7 pi -1 ps 1.5
set style line 2 lc rgb 'violet' lt 1 lw 2 pt 7 pi -1 ps 1.5
set pointintervalbox 3



#How many threads versus Operations per second
set title "List-1: Threads versus Operations per Second for Mutex/Spin"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Operations per second"
set logscale y 10
set yrange [0.01:]
set output 'lab2b_1.png'
plot \
    "<grep list-none-m,[0-9]*,1000,1, lab2b_list.csv" using ($2):(1000000000/$6) \
       title 'mutex' with linespoints ls 1, \
    "< grep 'list-none-s,[0-9]*,1000,1,' lab2b_list.csv" using ($2):(1000000000/$6) \
       title 'spin-lock' with linespoints ls 2
       

#Threads versus time spent waiting for a lock
set title "List-2: Mutex Wait Time"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Time (ns)"
set logscale y 10
set yrange [1000:]
set output 'lab2b_2.png'
plot \
     "<grep list-none-m,[0-9]*[2-9],1000,1, lab2b_list.csv" using ($2):($8) \
     	title 'Average wait-for-mutex time' with linespoints ls 1, \
     "< grep 'list-none-m,[0-9]*,1000,1,' lab2b_list.csv" using ($2):($7) \
        title 'Average time per operation' with linespoints ls 2



#Failures with yielding and with/without synchronization
set title "List-3: Threads that run with yielding without failure"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Successful Iterations"
set logscale y 10
set yrange [0.75:]
set output 'lab2b_3.png'
plot \
     "<grep list-id-none lab2b_list.csv" using ($2):($3) \
       title 'yield=id' with points lc rgb 'green', \
     "<grep list-id-m lab2b_list.csv" using ($2):($3) \
       title 'yield=id sync=m' with points lc rgb 'red', \
     "<grep list-id-s lab2b_list.csv" using ($2):($3) \
       title 'yield=id sync=s' with points lc rgb 'blue'


#Aggregated throughput (operations per second) versus threads with curves for each list number
set title "List-4: Aggregated throughput versus threads (fine grained with mutex)"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Operations per second"
set logscale y 10
set yrange [0.1:]
set output 'lab2b_4.png'
plot \
     "< grep -e list-none-m,[0-9]*,1000,1, lab2b_list.csv" using ($2):(1000000000/$6) \
     	title 'lists=1' with linespoints ls 1, \
     "< grep -e list-none-m,[0-9]*,1000,4, lab2b_list.csv" using ($2):(1000000000/$6) \
        title 'lists=4' with linespoints ls 2, \
     "< grep -e list-none-m,[0-9]*,1000,8, lab2b_list.csv" using ($2):(1000000000/$6) \
     	title 'lists=8' with linespoints ls 3, \
     "< grep -e list-none-m,[0-9]*,1000,16, lab2b_list.csv" using ($2):(1000000000/$6) \
     	title 'lists=16' with linespoints ls 4


#Aggregated throughput (operations per second) versus threads with curves for each list number
set title "List-5: Aggregated throughput versus threads (fine grained with spin)"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Operations per second"
set logscale y 10
set yrange [0.1:]
set output 'lab2b_5.png'
plot \
     "< grep -e list-none-s,[0-9]*,1000,1, lab2b_list.csv" using ($2):(1000000000/$6) \
     	title 'lists=1' with linespoints ls 1, \
     "< grep -e list-none-s,[0-9]*,1000,4, lab2b_list.csv" using ($2):(1000000000/$6) \
        title 'lists=4' with linespoints ls 2, \
     "< grep -e list-none-s,[0-9]*,1000,8, lab2b_list.csv" using ($2):(1000000000/$6) \
     	title 'lists=8' with linespoints ls 3, \
     "< grep -e list-none-s,[0-9]*,1000,16, lab2b_list.csv" using ($2):(1000000000/$6) \
     	title 'lists=16' with linespoints ls 4