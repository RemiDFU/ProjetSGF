GCC=/usr/bin/gcc

all: shell.o fs.o disk.o
	$(GCC) shell.o fileSystem.o disk.o -o sgf

shell.o: shell.c
	$(GCC) -Wall shell.c -c -o shell.o -g

fs.o: fileSystem.c fileSystem.h
	$(GCC) -Wall fileSystem.c -c -o fileSystem.o -g

disk.o: disk.c disk.h
	$(GCC) -Wall disk.c -c -o disk.o -g

clean:
	rm sgf disk.o fileSystem.o shell.o