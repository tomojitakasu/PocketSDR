cc = g++

CPPFLAGS = -O3 -Wno-narrowing -g

all: CyAPI.a

CyAPI.a: CyAPI.o
	ar r $@ CyAPI.o

clean:
	rm -f *.o

