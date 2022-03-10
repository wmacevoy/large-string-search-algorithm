search : search.cpp
	c++ -o search --std=c++14 search.cpp

test : search
	./search
