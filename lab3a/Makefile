CC=gcc
CFLAGS=-Wall

default:
	$(CC) $(CFLAGS) -o lab3a lab3a.c
clean:
	rm lab3a
emacs:
	emacs -nw lab3a.c
dist:
	rm *.tar.gz
	tar -czvf lab3a-504661838.tar.gz lab3a.c README Makefile ext2_fs.h
check:
	make dist
	bash P3A_check.sh 504661838
csv:
	./lab3a ../trivial.img > output.csv
