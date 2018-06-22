GCC=/usr/bin/g++

simplefs: shell.o fs.o disk.o
	$(GCC) shell.o fs.o disk.o -o simplefs

shell.o: shell.cpp
	$(GCC) -Wall shell.cpp -c -o shell.o -g

fs.o: fs.cpp fs.h
	$(GCC) -Wall fs.cpp -c -o fs.o -g

disk.o: disk.cpp disk.h
	$(GCC) -Wall disk.cpp -c -o disk.o -g

clean:
	rm simplefs disk.o fs.o shell.o

run20:
	./simplefs image.20 20

run5:
	./simplefs image.5 5

run200:
	./simplefs image.200 200
