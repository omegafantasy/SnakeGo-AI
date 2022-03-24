ifeq ($(OS),Windows_NT)
	LD_LIB += -lws2_32
endif

main: main.cpp
	g++ main.cpp -o main -O3 -I. -std=c++11 $(LD_LIB)

