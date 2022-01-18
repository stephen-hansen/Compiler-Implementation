CC=g++
STD=-std=c++17

.PHONY : clean

bin/comp: src/main.cpp obj/Parser.o
	mkdir -p bin
	${CC} ${STD} -o bin/comp src/main.cpp obj/Parser.o

obj/Parser.o: src/AST.h src/Parser.h src/Parser.cpp
	mkdir -p obj
	${CC} ${STD} -c -o obj/Parser.o src/Parser.cpp

clean:
	rm -f obj/*.o
	rm -f bin/comp

