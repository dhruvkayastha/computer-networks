
librsocket.a: rsocket.o
	ar -rcs librsocket.a rsocket.o

rsocket.o: rsocket.c rsocket.h
	gcc -Wall -c rsocket.c -lpthread

clean:
	rm rsocket.o librsocket.a user1