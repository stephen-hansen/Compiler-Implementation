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

class MethodDeclaration
{
   private:
      std::string _name;
      std::vector<std::string> _params;
      std::vector<std::string> _locals;
      std::vector<std::shared_ptr<ASTStatement>> _statements;
   public:
      MethodDeclaration(std::string name, std::vector<std::string> params, std::vector<std::string> locals, std::vector<std::shared_ptr<ASTStatement>> statements):
         _name(name), _params(params), _locals(locals), _statements(statements) {}
      std::string toString() {
         std::string out = std::string("{\"type\":\"MethodDeclaration\",\"name\":\"") +
            _name + std::string("\",\"params\":[");
         int i = 0;
         for (auto & p : _params) {
            if (i > 0) {
               out += std::string(",");
            }
            out += std::string("\"") + p + std::string("\"");
            i++;
         }
         out += std::string("],\"locals\":[");
         i = 0;
         for (auto & l : _locals) {
            if (i > 0) {
               out += std::string(",");
            }
            out += std::string("\"") + l + std::string("\"");
            i++;
         }
         out += std::string("],\"statements\":[");
         i = 0;
         for (auto & s : _statements) {
            if (i > 0) {
               out += std::string(",");
            }
            out += s->toString();
            i++;
         }
         out += std::string("]}");
         return out;
      }
};

class ClassDeclaration
{
   private:
      std::string _name;
      std::vector<std::string> _fields;
      std::vector<std::shared_ptr<MethodDeclaration>> _methods;
   public:
      ClassDeclaration(std::string name, std::vector<std::string> fields, std::vector<std::shared_ptr<MethodDeclaration>> methods):
         _name(name), _fields(fields), _methods(methods) {}
      std::string toString() {
         std::string out = std::string("{\"type\":\"ClassDeclaration\",\"name\":\"") +
            _name + std::string("\",\"fields\":[");
         int i = 0;
         for (auto & f : _fields) {
            if (i > 0) {
               out += std::string(",");
            }
            out += std::string("\"") + f + std::string("\"");
            i++;
         }
         out += std::string("],\"methods\":[");
         i = 0;
         for (auto & m : _methods) {
            if (i > 0) {
               out += std::string(",");
            }
            out += m->toString();
            i++;
         }
         out += std::string("]}");
         return out;
      }
};

class ProgramDeclaration
{
   private:
      std::vector<std::shared_ptr<ClassDeclaration>> _classes;
      std::vector<std::string> _main_locals;
      std::vector<std::shared_ptr<ASTStatement>> _main_statements;
   public:
      ProgramDeclaration(std::vector<std::shared_ptr<ClassDeclaration>> classes, std::vector<std::string> main_locals, std::vector<std::shared_ptr<ASTStatement>> main_statements):
         _classes(classes), _main_locals(main_locals), _main_statements(main_statements) {}
      std::string toString() {
         std::string out = std::string("{\"type\":\"ProgramDeclaration\",\"classes\":[");
         int i = 0;
         for (auto & c : _classes) {
            if (i > 0) {
               out += std::string(",");
            }
            out += c->toString();
            i++;
         }
         out += std::string("],\"main_locals\":[");
         i = 0;
         for (auto & l : _main_locals) {
            if (i > 0) {
               out += std::string(",");
            }
            out += std::string("\"") + l + std::string("\"");
            i++;
         }
         out += std::string("],\"main_statements\":[");
         i = 0;
         for (auto & s : _main_statements) {
            if (i > 0) {
               out += std::string(",");
            }
            out += s->toString();
            i++;
         }
         out += std::string("]}");
         return out;
      }
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
