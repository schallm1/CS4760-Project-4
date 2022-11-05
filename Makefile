
all: oss child

oss: oss.o
	gcc -o oss oss.o

child: child.o
	gcc -o child child.o

oss.o: oss.c
	gcc -c oss.c

child.o: child.c
	gcc -c child.c


clean:
	rm -f *.o
	rm -f oss
	rm -f child
	rm -f logfile
