#ip address -- 192.168.2.15
#password -- nai9a1pq


CC=gcc
CFLAGS=-Wall -lmraa -g -lm -pthread

default:
	export TZ=America/Los_Angeles
	$(CC) $(CFLAGS) -o lab4b lab4b.c
screen:
	screen -L /dev/cu.usbserial-AL005VGA 115200
kill:
	screen -X -S screen -X -S 4209.ttys001.Seans-MacBook-Air-2 kill
scp_home:
	scp -r ../lab4b/ root@192.168.0.2:~/
scp_edu:
	scp -r ../lab4b/ root@131.179.55.7:~/
check:
	make dist
	bash P4B_check.sh 504661838 > error_report.txt
dist:
	rm -rf lab4b-504661838.tar.gz
	tar -czvf lab4b-504661838.tar.gz Makefile lab4b.c README
clean:
	rm -rf *~ lab4b