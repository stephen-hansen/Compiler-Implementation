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
      std::string info() { return _info; }
};

class ProgramParser
{
   public:
      int skipChars(std::istream & input, std::string chars);
      int skipWhitespace(std::istream & input);
      int skipWhitespaceAndNewlines(std::istream & input);
      void advanceAndExpectChar(std::istream & input, char c, std::string addlInfo);
      void advanceAndExpectWord(std::istream & input, std::string c, std::string addlInfo);
      ASTExpression * parseExpr(std::istream & input);
      ASTStatement * parseStmt(std::istream & input);
      MethodDeclaration * parseMethod(std::istream & input);
      ClassDeclaration * parseClass(std::istream & input);
      ProgramDeclaration * parse(std::istream & input);
};

#endif
