#ifndef _CS441_PARSER_H
#define _CS441_PARSER_H
#include <exception>
#include <istream>
#include <string>
#include <vector>
#include "AST.h"

class ParserException : public std::exception
{
   private:
      std::string _info;
   public:
      ParserException(std::string info): _info(info) {}
};

class MethodDeclaration
{
   private:
      std::string _name;
      std::vector<std::string> _params;
      std::vector<std::string> _locals;
      std::vector<ASTStatement> _statements;
};

class ClassDeclaration
{
   private:
      std::string _name;
      std::vector<std::string> _fields;
      std::vector<MethodDeclaration> _methods;
};

class ProgramDeclaration
{
   private:
      std::vector<ClassDeclaration> _classes;
      std::vector<std::string> _main_locals;
      std::vector<ASTStatement> _main_statements;
};

class ProgramParser
{
   private:
      void advanceAndExpectChar(std::istream & input, char c, std::string addlInfo);
      ASTExpression parseExpr(std::istream & input);
      ASTStatement parseStmt(std::istream & input);
      MethodDeclaration parseMethod(std::istream & input);
      ClassDeclaration parseClass(std::istream & input);
   public:
      ProgramDeclaration parse(std::istream & input);
};

#endif
