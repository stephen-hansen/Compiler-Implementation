#ifndef _CS_441_CFGBUILDER_H
#define _CS_441_CFGBUILDER_H
#include <exception>
#include <map>
#include <stack>
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

class CFGBuilder : public ASTVisitor
{
   private:
      unsigned long _temp_counter = 0;
      std::map<std::string, unsigned long> _field_to_table_offset;
      std::stack<std::string> _return_values;
      std::stack<std::shared_ptr<BasicBlock>> _blocks;
      std::string createTempVariable() {
         return toRegister(std::to_string(_temp_counter++));
      }
   public:
      void visit(UInt32Literal& node);
      void visit(VariableIdentifier& node);
      void visit(ArithmeticExpression& node); 
      void visit(CallExpression& node);
      void visit(FieldReadExpression& node);
      void visit(NewObjectExpression& node);
      void visit(ThisObjectExpression& node);
      void visit(AssignmentStatement& node);
      void visit(DontCareAssignmentStatement& node);
      void visit(FieldUpdateStatement& node);
      void visit(IfElseStatement& node);
      void visit(IfOnlyStatement& node);
      void visit(WhileStatement& node);
      void visit(ReturnStatement& node);
      void visit(PrintStatement& node);
      void visit(MethodDeclaration& node);
      void visit(ClassDeclaration& node);
      void visit(ProgramDeclaration& node);
};

#endif
