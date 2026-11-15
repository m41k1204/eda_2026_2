CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra

all: test bench

test: test.cpp mvbt.h bptree.h disk.h
	$(CXX) $(CXXFLAGS) test.cpp -o test

bench: bench.cpp mvbt.h bptree.h disk.h
	$(CXX) $(CXXFLAGS) bench.cpp -o bench

check: test
	./test

results: bench
	./bench
	python3 plots.py

clean:
	rm -f test bench

.PHONY: all check results clean
