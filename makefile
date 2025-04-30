CC 			 := g++
CFLAGS 		 := 

.PHONY: clean

a.out: main.o filesystem.o avl.o customErrorClass.o
	$(CC) -g -o $@ $(CFLAGS) $^

main.o: main.cpp
filesystem.o: filesystem.cpp
avl.o: avl.cpp
customErrorClass.o: customErrorClass.cpp

main.cpp: filesystem.h
filesystem.cpp: filesystem.h avl.h customErrorClass.h
avl.cpp: avl.h
customErrorClass.cpp: customErrorClass.h

filesystem.h: types.h
avl.h: filesystem.h

clean:
	rm -vf -- a.out *.o

%.o: %.cpp
	$(CC) -g -c -o $@ $(CFLAGS) $^
