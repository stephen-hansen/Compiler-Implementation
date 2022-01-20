#ifndef _CS_441_CFGBUILDER_H
#define _CS_441_CFGBUILDER_H
#include <exception>
#include <map>
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

// TODO define all visit methods

class CFGBuilder : public ASTVisitor
{
   private:
      unsigned long _temp_counter = 0;
      std::map<std::string, unsigned long> _field_to_table_offset;
   public:
      virtual void visit(UInt32Literal& node) = 0;
      virtual void visit(VariableIdentifier& node) = 0;
      virtual void visit(ArithmeticExpression& node) = 0;
      virtual void visit(CallExpression& node) = 0;
      virtual void visit(FieldReadExpression& node) = 0;
      virtual void visit(NewObjectExpression& node) = 0;
      virtual void visit(ThisObjectExpression& node) = 0;
      virtual void visit(AssignmentStatement& node) = 0;
      virtual void visit(DontCareAssignmentStatement& node) = 0;
      virtual void visit(FieldUpdateStatement& node) = 0;
      virtual void visit(IfElseStatement& node) = 0;
      virtual void visit(IfOnlyStatement& node) = 0;
      virtual void visit(WhileStatement& node) = 0;
      virtual void visit(ReturnStatement& node) = 0;
      virtual void visit(PrintStatement& node) = 0;
      virtual void visit(MethodDeclaration& node) = 0;
      virtual void visit(ClassDeclaration& node) = 0;
      virtual void visit(ProgramDeclaration& node) = 0;
};

#endif
