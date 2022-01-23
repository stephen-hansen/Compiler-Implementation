CC=g++
STD=-std=c++17 -g

.PHONY : clean

comp: src/main.cpp obj/Parser.o obj/CFGBuilder.o src/IdentityOptimizer.h
	${CC} ${STD} -o comp src/main.cpp obj/Parser.o obj/CFGBuilder.o

obj/CFGBuilder.o: src/AST.h src/CFG.h src/CFGBuilder.h src/CFGBuilder.cpp
	mkdir -p obj
	${CC} ${STD} -c -o obj/CFGBuilder.o src/CFGBuilder.cpp

obj/Parser.o: src/AST.h src/Parser.h src/Parser.cpp
	mkdir -p obj
	${CC} ${STD} -c -o obj/Parser.o src/Parser.cpp

clean:
	rm -f obj/*.o
	rm -f comp

