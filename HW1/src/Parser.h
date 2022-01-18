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
      std::shared_ptr<ASTExpression> parseExpr(std::istream & input);
      std::shared_ptr<ASTStatement> parseStmt(std::istream & input);
      std::shared_ptr<MethodDeclaration> parseMethod(std::istream & input);
      std::shared_ptr<ClassDeclaration> parseClass(std::istream & input);
      std::shared_ptr<ProgramDeclaration> parse(std::istream & input);
};

#endif
