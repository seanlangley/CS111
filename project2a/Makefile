CC=gcc
CFLAGS=-pthread -g -Wall

build: lab2_add.c lab2_list.c
	$(CC) $(CFLAGS) -o lab2_add lab2_add.c
	$(CC) $(CFLAGS) -o lab2_list lab2_list.c SortedList.c
graphs:
	rm -rf *.png
	/usr/local/cs/bin/gnuplot lab2_add.gp
	/usr/local/cs/bin/gnuplot lab2_list.gp

tests: 
	bash test.sh
dist:
	tar -czvf lab2a-504661838.tar.gz *.c *.h Makefile *.csv *.png README
check:
	./P2A_check.sh 504661838
clean:
	rm -rf lab2a-504661838.tar.gz lab2_add lab2_list *.png *.csv
