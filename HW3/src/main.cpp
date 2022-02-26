#include <iostream>
#include "ArithmeticOptimizer.h"
#include "TypeChecker.h"
#include "BetterSSAOptimizer.h"
#include "SSAOptimizer.h"
#include "ValueNumberOptimizer.h"
#include "CFGBuilder.h"
#include "Parser.h"

int main(int argc, char ** argv) {
   bool printAST = false, noSSA = false, noopt = false, simpleSSA = false, noVN = false;
   for (int i=0; i<argc; i++) {
      std::string arg = argv[i];
      if (arg == "-printAST") {
         printAST = true;
      } else if (arg == "-noSSA") {
         noSSA = true;
         // VN needs SSA to work correctly
         noVN = true;
      } else if (arg == "-noopt") {
         noopt = true;
      } else if (arg == "-simpleSSA") {
         simpleSSA = true;
      } else if (arg == "-noVN") {
         noVN = true;
      }
   }
   ProgramParser parser;
   TypeChecker checker;
   CFGBuilder builder;
   BetterSSAOptimizer better_ssa_optimizer;
   SSAOptimizer ssa_optimizer;
   ArithmeticOptimizer peephole_optimizer;
   ValueNumberOptimizer vn_optimizer;
   try {
      std::shared_ptr<ProgramDeclaration> progAST = parser.parse(std::cin);
      if (printAST) {
         std::cout << progAST->toString() << std::endl;
         return 0;
      }
      checker.check(progAST);
      std::shared_ptr<ProgramCFG> progCFG = builder.build(progAST);
      if (!noSSA) {
         if (simpleSSA) {
            progCFG = ssa_optimizer.optimize(progCFG);
         } else {
            progCFG = better_ssa_optimizer.optimize(progCFG);
         }
      }
      if (!noopt) {
         progCFG = peephole_optimizer.optimize(progCFG);
      }
      if (!noVN) {
         progCFG = vn_optimizer.optimize(progCFG);
      }
      std::cout << progCFG->toString() << std::endl;
      return 0;
   } catch (ParserException & p) {
      std::cerr << "Parser error:" << std::endl;
      std::cerr << p.info() << std::endl;
      return 1;
   } catch (TypeCheckerException & tc) {
      std::cerr << "Type checker error:" << std::endl;
      std::cerr << tc.info() << " : " << tc.line() << std::endl;
   }
   return 0;
}

