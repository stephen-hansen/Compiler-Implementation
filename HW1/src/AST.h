#ifndef _CS441_AST_H
#define _CS441_AST_H
#include <string>
#include <vector>

#define MAXARGS 6

class ASTNode
{};

class ASTExpression : public ASTNode
{};

class ASTStatement : public ASTNode
{};

class UInt32Literal : public ASTExpression
{
   private:
      uint32_t _val;
   public:
      UInt32Literal(uint32_t val): _val(val) {}
};

class VariableIdentifier : public ASTExpression
{
   private:
      std::string _name;
   public:
      VariableIdentifier(std::string name): _name(name) {}
};

class ArithmeticExpression : public ASTExpression
{
   private:
      char _op;
      ASTExpression _expr[2];
   public:
      ArithmeticExpression(char op, ASTExpression e1, ASTExpression e2): _op(op), _expr({e1, e2}) {}
};

class CallExpression : public ASTExpression
{
   private:
      ASTExpression _obj;
      std::string _method;
      std::vector<ASTExpression> _params;
   public:
      CallExpression(ASTExpression obj, std::string method, std::vector<ASTExpression> params): _obj(obj), _method(method), _params(params) {}
};

class FieldReadExpression : public ASTExpression
{
   private:
      ASTExpression _obj;
      std::string _field;
   public:
      FieldReadExpression(ASTExpression obj, std::string field): _obj(obj), _field(field) {}
};

class NewObjectExpression : public ASTExpression
{
   private:
      std::string _class_name;
   public:
      NewObjectExpression(std::string class_name): _class_name(class_name) {}
};

class ThisObject : public ASTExpression
{};

class AssignmentStatement : public ASTStatement
{
   private:
      std::string _variable;
      ASTExpression _val;
};

class DontCareAssignmentStatement : public ASTStatement
{
   private:
      ASTExpression _val;
};

class FieldUpdateStatement : public ASTStatement
{
   private:
      ASTExpression _obj;
      std::string _field;
      ASTExpression _val;
};

class IfElseStatement : public ASTStatement
{
   private:
      ASTExpression _cond;
      std::vector<ASTStatement> _if_statements;
      std::vector<ASTStatement> _else_statements;
};

class IfOnlyStatement : public ASTStatement
{
   private:
      ASTExpression _cond;
      std::vector<ASTStatement> _statements;
};

class WhileStatement : public ASTStatement
{
   private:
      ASTExpression _cond;
      std::vector<ASTStatement> _statements;
};

class ReturnStatement : public ASTStatement
{
   private:
      ASTExpression _val;
};

class PrintStatement : public ASTStatement
{
   private:
      ASTExpression _val;
};

#endif
