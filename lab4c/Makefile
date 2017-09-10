CC=gcc
CFLAGS=-pthread -Wall -g -lm

default:
	$(CC) $(CFLAGS) -lmraa -o lab4c_tcp lab4c_tcp.c
	$(CC) $(CFLAGS) -lssl -lcrypto -lmraa -o lab4c_tls lab4c_tls.c

screen:
	screen -L /dev/cu.usbserial-AL005VGA 115200
dist:
	rm -rf lab4c-504661838.tar.gz
	tar -czf lab4c-504661838.tar.gz Makefile lab4c_tcp.c lab4c_tls.c README
clean:
	rm -rf lab4c-504661838.tar.gz lab4c_tcp lab4c_tls *~ screenlog.0
scp:
	scp -r ../lab4c root@192.168.1.20:~/
check:
	make dist
	bash P4C_check.sh 504661838