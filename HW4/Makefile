CC=g++
STD=-std=c++17

.PHONY : clean

comp: src/main.cpp obj/Parser.o obj/CFGBuilder.o src/TypeChecker.h src/IdentityOptimizer.h src/ArithmeticOptimizer.h src/SSAOptimizer.h src/DominatorSolver.h src/BetterSSAOptimizer.h src/ValueNumberOptimizer.h src/VectorOptimizer.h
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

