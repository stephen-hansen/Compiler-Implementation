CC=g++
STD=-std=c++17

.PHONY : clean

bin/comp: src/main.cpp obj/CFGBuilder.o obj/Parser.o
	mkdir -p bin
	${CC} ${STD} -o bin/comp src/main.cpp obj/CFGBuilder.o obj/Parser.o

obj/CFGBuilder.o: src/AST.h src/CFG.h src/CFGBuilder.h src/CFGBuilder.cpp
	mkdir -p obj
	${CC} ${STD} -c -o obj/CFGBuilder.o src/CFGBuilder.cpp

obj/Parser.o: src/Parser.h src/Parser.cpp obj/CFGBuilder.o
	mkdir -p obj
	${CC} ${STD} -c -o obj/Parser.o src/Parser.cpp obj/CFGBuilder.o

clean:
	rm -f obj/*.o
	rm -f bin/comp

