#ifndef _CS_441_CFGBUILDER_H
#define _CS_441_CFGBUILDER_H
#include <exception>
#include <string>
#include "AST.h"
#include "CFG.h"

class CFGBuilderException : public std::exception
{
   private:
      std::string _info;
   public:
      CFGBuilderException(std::string info): _info(info) {}
      std::string info() { return _info; }
};

class CFGBuilder
{
   private:
      size_t _temp_counter;
   public:
      CFGBuilder(): _temp_counter(0) {}
      size_t allocTemp() {
         return _temp_counter++;
      }
      CFG * buildCFG(ProgramDeclaration * program) {
         CFG * cfg = new CFG();
         program->toCFG(cfg->getRoot(), this);
         return cfg;
      }
};

#endif
