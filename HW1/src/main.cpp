#include <iostream>
#include "CFGBuilder.h"
#include "IdentityOptimizer.h"
#include "Parser.h"

int main(int argc, char ** argv) {
   bool printAST = false, noSSA = false, noopt = false;
   for (int i=0; i<argc; i++) {
      std::string arg = argv[i];
      if (arg == "-printAST") {
         printAST = true;
      } else if (arg == "-noSSA") {
         noSSA = true;
      } else if (arg == "-noopt") {
         noopt = true;
      }
   }
   ProgramParser parser;
   CFGBuilder builder;
   IdentityOptimizer optimizer;
   try {
      std::shared_ptr<ProgramDeclaration> progAST = parser.parse(std::cin);
      if (printAST) {
         std::cout << progAST->toString() << std::endl;
         return 0;
      }
      std::shared_ptr<ProgramCFG> progCFG = builder.build(progAST);
      if (noopt) {
         std::cout << progCFG->toString() << std::endl;
         return 0;
      }
      std::shared_ptr<ProgramCFG> optimizedCFG = optimizer.optimize(progCFG);
      std::cout << optimizedCFG->toString() << std::endl;
      return 0;
   } catch (ParserException & p) {
      std::cerr << "Parser error:" << std::endl;
      std::cerr << p.info() << std::endl;
      return 1;
   }
   return 0;
}

