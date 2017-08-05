#NAME: Sean Langley
#EMAIL: sean.langley22@gmail.com
#ID: 504 661 838

CC=gcc
CFLAGS=-pthread -g -Wall

default: lab2_list.c
	$(CC) $(CFLAGS) -o lab2_list lab2_list.c SortedList.c
graphs:
	rm -rf *.png
	/usr/local/cs/bin/gnuplot lab2_list.gp

tests: 
	bash tests.sh
dist:
	tar -czvf lab2b-504661838.tar.gz *.c *.h Makefile *.csv *.png README *.out tests.sh
profile:
	rm -rf raw profile.out
	LD_PRELOAD=/u/ee/ugrad/seanl/pprof/lib/libprofiler.so.0 CPUPROFILE=raw ./lab2_list --threads=12 --iterations=1000 --sync=s
	/u/ee/ugrad/seanl/pprof/bin/pprof --list=thread_function lab2_list raw > profile.out
check:
	rm -rf error_report.txt
	bash P2B_check.sh 504661838 > error_report.txt
clean:
	rm -rf lab2b-504661838.tar.gz lab2_list *.png *.csv profile.out raw error_report.txt
