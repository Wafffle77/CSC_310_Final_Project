CC 			 := g++
CFLAGS 		 := 

.PHONY: clean

a.out: main.o filesystem.o avl.cpp
	$(CC) -g -o $@ $(CFLAGS) $^

main.o: main.cpp
filesystem.o: filesystem.cpp

main.cpp: filesystem.h
filesystem.cpp: filesystem.h

clean:
	rm -vf -- a.out *.o

%.o: %.cpp
	$(CC) -g -c -o $@ $(CFLAGS) $^
