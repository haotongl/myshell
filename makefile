all: main.c myshell.o parse.o common.h tries.o builtin.o job.o
	gcc main.c myshell.o parse.o builtin.o tries.o job.o -o myshell
myshell.o: myshell.c myshell.h common.h builtin.o tries.o
	gcc -c myshell.c -o myshell.o
parse.o: parse.c parse.h common.h
	gcc -c parse.c -o parse.o
job.o: job.h job.c common.h myshell.c
	gcc -c job.c -o job.o
tries.o: tries.c tries.o common.h
	gcc -c tries.c -o tries.o
builtin.o: builtin.c builtin.h common.h
	gcc -c builtin.c -o builtin.o
clean:
	rm -rf *.o
