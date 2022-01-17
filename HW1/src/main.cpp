#include <iostream>
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
   try {
      std::shared_ptr<ProgramDeclaration> p = std::shared_ptr<ProgramDeclaration>(parser.parse(std::cin));
      if (printAST) {
         std::cout << p->toString() << std::endl;
         return 0;
      }
   } catch (ParserException & p) {
      std::cerr << "Parser error:" << std::endl;
      std::cerr << p.info() << std::endl;
      return 1;
   }
   return 0;
}

