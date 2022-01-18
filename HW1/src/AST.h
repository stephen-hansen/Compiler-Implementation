#ifndef _CS441_AST_H
#define _CS441_AST_H
#include <memory>
#include <string>
#include <vector>

#define MAXARGS 6

class ASTNode
{
   public:
      virtual std::string toString() = 0;
};

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
      std::string toString() {
         return std::string("{\"type\":\"UInt32Literal\",\"val\":\"") + std::to_string(_val) + std::string("\"}");
      }
};

class VariableIdentifier : public ASTExpression
{
   private:
      std::string _name;
   public:
      VariableIdentifier(std::string name): _name(name) {}
      std::string toString() {
         return std::string("{\"type\":\"VariableIdentifier\",\"name\":\"") + _name + std::string("\"}");
      }
};

class ArithmeticExpression : public ASTExpression
{
   private:
      char _op;
      std::shared_ptr<ASTExpression> _e1;
      std::shared_ptr<ASTExpression> _e2;
   public:
      ArithmeticExpression(char op, ASTExpression * e1, ASTExpression * e2): _op(op), _e1(e1), _e2(e2) {}
      std::string toString() {
         return std::string("{\"type\":\"ArithmeticExpression\",\"op\":\"") + std::string(1,_op) +
            std::string("\",\"e1\":") + _e1->toString() + std::string(",\"e2\":") + _e2->toString() +
            std::string("}");
      }
};

class CallExpression : public ASTExpression
{
   private:
      std::shared_ptr<ASTExpression> _obj;
      std::string _method;
      std::vector<std::shared_ptr<ASTExpression>> _params;
   public:
      CallExpression(ASTExpression * obj, std::string method, std::vector<std::shared_ptr<ASTExpression>> params): _obj(obj), _method(method), _params(params) {}
      std::string toString() {
         std::string out = std::string("{\"type\":\"CallExpression\",\"obj\":") + _obj->toString() +
            std::string(",\"method\":\"") + _method + std::string("\",\"params\":[");
         int i = 0;
         for (auto & e : _params) {
            if (i > 0) {
               out += std::string(",");
            }
            out += e->toString();
            i++;
         }
         out += std::string("]}");
         return out;
      }
};

class FieldReadExpression : public ASTExpression
{
   private:
      std::shared_ptr<ASTExpression> _obj;
      std::string _field;
   public:
      FieldReadExpression(ASTExpression * obj, std::string field): _obj(obj), _field(field) {}
      std::string toString() {
         return std::string("{\"type\":\"FieldReadExpression\",\"obj\":") + _obj->toString() +
            std::string(",\"field\":\"") + _field + std::string("\"}");
      }
};

class NewObjectExpression : public ASTExpression
{
   private:
      std::string _class_name;
   public:
      NewObjectExpression(std::string class_name): _class_name(class_name) {}
      std::string toString() {
         return std::string("{\"type\":\"NewObjectExpression\",\"class_name\":\"") + _class_name +
            std::string("\"}");
      }
};

class ThisObjectExpression : public ASTExpression
{
   public:
      std::string toString() {
         return std::string("{\"type\":\"ThisObjectExpression\"}");
      }
};

class AssignmentStatement : public ASTStatement
{
   private:
      std::string _variable;
      std::shared_ptr<ASTExpression> _val;
   public:
      AssignmentStatement(std::string variable, ASTExpression * val): _variable(variable), _val(val) {}
      std::string toString() {
         return std::string("{\"type\":\"AssignmentStatement\",\"variable\":\"") + _variable +
            std::string("\",\"val\":") + _val->toString() + std::string("}");
      }
};

class DontCareAssignmentStatement : public ASTStatement
{
   private:
      std::shared_ptr<ASTExpression> _val;
   public:
      DontCareAssignmentStatement(ASTExpression * val): _val(val) {}
      std::string toString() {
         return std::string("{\"type\":\"DontCareAssignmentStatement\",\"val\":") + _val->toString() +
            std::string("}");
      }
};

class FieldUpdateStatement : public ASTStatement
{
   private:
      std::shared_ptr<ASTExpression> _obj;
      std::string _field;
      std::shared_ptr<ASTExpression> _val;
   public:
      FieldUpdateStatement(ASTExpression * obj, std::string field, ASTExpression * val): _obj(obj), _field(field), _val(val) {}
      std::string toString() {
         return std::string("{\"type\":\"FieldUpdateStatement\",\"obj\":") + _obj->toString() +
            std::string(",\"field\":\"") + _field + std::string("\",\"val\":") + _val->toString() +
            std::string("}");
      }
};

class IfElseStatement : public ASTStatement
{
   private:
      std::shared_ptr<ASTExpression> _cond;
      std::vector<std::shared_ptr<ASTStatement>> _if_statements;
      std::vector<std::shared_ptr<ASTStatement>> _else_statements;
   public:
      IfElseStatement(ASTExpression * cond, std::vector<std::shared_ptr<ASTStatement>> if_statements, std::vector<std::shared_ptr<ASTStatement>> else_statements): _cond(cond), _if_statements(if_statements), _else_statements(else_statements) {}
      std::string toString() {
         std::string out = std::string("{\"type\":\"IfElseStatement\",\"cond\":") + _cond->toString() +
            std::string(",\"if_statements\":[");
         int i = 0;
         for (auto & e : _if_statements) {
            if (i > 0) {
               out += std::string(",");
            }
            out += e->toString();
            i++;
         }
         out += std::string("],\"else_statements\":[");
         i = 0;
         for (auto & e : _else_statements) {
            if (i > 0) {
               out += std::string(",");
            }
            out += e->toString();
            i++;
         }
         out += std::string("]}");
         return out;
      }
};

class IfOnlyStatement : public ASTStatement
{
   private:
      std::shared_ptr<ASTExpression> _cond;
      std::vector<std::shared_ptr<ASTStatement>> _statements;
   public:
      IfOnlyStatement(ASTExpression * cond, std::vector<std::shared_ptr<ASTStatement>> statements): _cond(cond), _statements(statements) {}
      std::string toString() {
         std::string out = std::string("{\"type\":\"IfOnlyStatement\",\"cond\":") + _cond->toString() +
            std::string(",\"statements\":[");
         int i = 0;
         for (auto & e : _statements) {
            if (i > 0) {
               out += std::string(",");
            }
            out += e->toString();
            i++;
         }
         out += std::string("]}");
         return out;
      }
};

class WhileStatement : public ASTStatement
{
   private:
      std::shared_ptr<ASTExpression> _cond;
      std::vector<std::shared_ptr<ASTStatement>> _statements;
   public:
      WhileStatement(ASTExpression * cond, std::vector<std::shared_ptr<ASTStatement>> statements): _cond(cond), _statements(statements) {}
      std::string toString() {
         std::string out = std::string("{\"type\":\"WhileStatement\",\"cond\":") + _cond->toString() +
            std::string(",\"statements\":[");
         int i = 0;
         for (auto & e : _statements) {
            if (i > 0) {
               out += std::string(",");
            }
            out += e->toString();
            i++;
         }
         out += std::string("]}");
         return out;
      }
};

class ReturnStatement : public ASTStatement
{
   private:
      std::shared_ptr<ASTExpression> _val;
   public:
      ReturnStatement(ASTExpression * val): _val(val) {}
      std::string toString() {
         return std::string("{\"type\":\"ReturnStatement\",\"val\":") + _val->toString() +
            std::string("}");
      }
};

class PrintStatement : public ASTStatement
{
   private:
      std::shared_ptr<ASTExpression> _val;
   public:
      PrintStatement(ASTExpression * val): _val(val) {}
      std::string toString() {
         return std::string("{\"type\":\"PrintStatement\",\"val\":") + _val->toString() +
            std::string("}");
      }
};

class MethodDeclaration : public ASTNode
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

class ClassDeclaration : public ASTNode
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

class ProgramDeclaration : public ASTNode
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

#endif
