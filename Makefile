# Makefile

all: lab2

lab2: lab2.cpp
	g++ lab2.cpp libggfonts.a -Wall -lX11 -lGL -o lab2

clean: lab2
	rm lab2
