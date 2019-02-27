all: build shell

build:
	g++ -std=c++11 -L/usr/include -lreadline main.cpp shelpers.cpp

shell:
	./a.out

