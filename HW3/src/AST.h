#ifndef _CS441_AST_H
#define _CS441_AST_H
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <sstream>
#include <vector>

#define MAXARGS 6

// Forward declare visitor
class ASTVisitor;

class ASTNode
{
   public:
      virtual std::string toString() = 0;
      virtual void accept(ASTVisitor& v) = 0;
};

class ASTExpression : public ASTNode
{
   public:
      virtual std::string toSourceString() = 0;
};

class ASTStatement : public ASTNode
{
   public:
      virtual std::string toSourceString() = 0;
};

// Forward declare all specific nodes here
class UInt32Literal;
class VariableIdentifier;
class ArithmeticExpression;
class CallExpression;
class FieldReadExpression;
class NewObjectExpression;
class NullObjectExpression;
class ThisObjectExpression;
class AssignmentStatement;
class DontCareAssignmentStatement;
class FieldUpdateStatement;
class IfElseStatement;
class IfOnlyStatement;
class WhileStatement;
class ReturnStatement;
class PrintStatement;
class MethodDeclaration;
class ClassDeclaration;
class ProgramDeclaration;

// Declare visitor here
class ASTVisitor
{
   public:
      virtual void visit(UInt32Literal& node) = 0;
      virtual void visit(VariableIdentifier& node) = 0;
      virtual void visit(ArithmeticExpression& node) = 0;
      virtual void visit(CallExpression& node) = 0;
      virtual void visit(FieldReadExpression& node) = 0;
      virtual void visit(NewObjectExpression& node) = 0;
      virtual void visit(ThisObjectExpression& node) = 0;
      virtual void visit(NullObjectExpression& node) = 0;
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

class UInt32Literal : public ASTExpression
{
   private:
      uint32_t _val;
   public:
      UInt32Literal(uint32_t val): _val(val) {}
      std::string toString() override {
         return std::string("{\"type\":\"UInt32Literal\",\"val\":\"") + std::to_string(_val) + std::string("\"}");
      }
      std::string toSourceString() override {
         return std::to_string(_val);
      }
      void accept(ASTVisitor& v) override {
         v.visit(*this);
      }
      uint32_t val() { return _val; }
};

class VariableIdentifier : public ASTExpression
{
   private:
      std::string _name;
   public:
      VariableIdentifier(std::string name): _name(name) {}
      std::string toString() override {
         return std::string("{\"type\":\"VariableIdentifier\",\"name\":\"") + _name + std::string("\"}");
      }
      std::string toSourceString() override {
         return _name; 
      }
      void accept(ASTVisitor& v) override {
         v.visit(*this);
      }
      std::string name() { return _name; }
};

class ArithmeticExpression : public ASTExpression
{
   private:
      char _op;
      std::shared_ptr<ASTExpression> _e1;
      std::shared_ptr<ASTExpression> _e2;
   public:
      ArithmeticExpression(char op, std::shared_ptr<ASTExpression> e1, std::shared_ptr<ASTExpression> e2): _op(op), _e1(e1), _e2(e2) {}
      std::string toString() override {
         return std::string("{\"type\":\"ArithmeticExpression\",\"op\":\"") + std::string(1,_op) +
            std::string("\",\"e1\":") + _e1->toString() + std::string(",\"e2\":") + _e2->toString() +
            std::string("}");
      }
      std::string toSourceString() override {
         return std::string("(") + _e1->toSourceString() + " " + _op + " " + _e2->toSourceString() + ")";
      }
      void accept(ASTVisitor& v) override {
         v.visit(*this);
      }
      char op() { return _op; }
      std::shared_ptr<ASTExpression> e1() { return _e1; }
      std::shared_ptr<ASTExpression> e2() { return _e2; }
};

class CallExpression : public ASTExpression
{
   private:
      std::shared_ptr<ASTExpression> _obj;
      std::string _method;
      std::vector<std::shared_ptr<ASTExpression>> _params;
   public:
      CallExpression(std::shared_ptr<ASTExpression> obj, std::string method, std::vector<std::shared_ptr<ASTExpression>> params): _obj(obj), _method(method), _params(params) {}
      std::string toString() override {
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
      std::string toSourceString() override {
         std::stringstream buf;
         buf << "^" << _obj->toSourceString();
         buf << "." << _method;
         buf << "(";
         int i = 0;
         for (auto & p : _params) {
            if (i > 0) {
               buf << ",";
            }
            buf << p->toSourceString();
            i++;
         }
         buf << ")";
         return buf.str();
      }
      void accept(ASTVisitor& v) override {
         v.visit(*this);
      }
      std::shared_ptr<ASTExpression> obj() { return _obj; }
      std::string method() { return _method; }
      std::vector<std::shared_ptr<ASTExpression>> params() { return _params; }
};

class FieldReadExpression : public ASTExpression
{
   private:
      std::shared_ptr<ASTExpression> _obj;
      std::string _field;
   public:
      FieldReadExpression(std::shared_ptr<ASTExpression> obj, std::string field): _obj(obj), _field(field) {}
      std::string toString() override {
         return std::string("{\"type\":\"FieldReadExpression\",\"obj\":") + _obj->toString() +
            std::string(",\"field\":\"") + _field + std::string("\"}");
      }
      std::string toSourceString() override {
         return std::string("&") + _obj->toSourceString() + "." + _field;
      }
      void accept(ASTVisitor& v) override {
         v.visit(*this);
      }
      std::shared_ptr<ASTExpression> obj() { return _obj; }
      std::string field() { return _field; }
};

class NewObjectExpression : public ASTExpression
{
   private:
      std::string _class_name;
   public:
      NewObjectExpression(std::string class_name): _class_name(class_name) {}
      std::string toString() override {
         return std::string("{\"type\":\"NewObjectExpression\",\"class_name\":\"") + _class_name +
            std::string("\"}");
      }
      std::string toSourceString() override {
         return std::string("@") + _class_name;
      }
      void accept(ASTVisitor& v) override {
         v.visit(*this);
      }
      std::string class_name() { return _class_name; }
};

class ThisObjectExpression : public ASTExpression
{
   public:
      std::string toString() override {
         return std::string("{\"type\":\"ThisObjectExpression\"}");
      }
      std::string toSourceString() override {
         return "this";
      }
      void accept(ASTVisitor& v) override {
         v.visit(*this);
      }
};

class NullObjectExpression : public ASTExpression
{
   private:
      std::string _class_name;
   public:
      NullObjectExpression(std::string class_name): _class_name(class_name) {}
      std::string toString() override {
         return std::string("{\"type\":\"NullObjectExpression\",\"class_name\":\"") + _class_name +
            std::string("\"}");
      }
      std::string toSourceString() override {
         return std::string("null:") + _class_name;
      }
      void accept(ASTVisitor& v) override {
         v.visit(*this);
      }
      std::string class_name() { return _class_name; }
};

class AssignmentStatement : public ASTStatement
{
   private:
      std::string _variable;
      std::shared_ptr<ASTExpression> _val;
   public:
      AssignmentStatement(std::string variable, std::shared_ptr<ASTExpression> val): _variable(variable), _val(val) {}
      std::string toString() override {
         return std::string("{\"type\":\"AssignmentStatement\",\"variable\":\"") + _variable +
            std::string("\",\"val\":") + _val->toString() + std::string("}");
      }
      std::string toSourceString() override {
         return _variable + " = " + _val->toSourceString();
      }
      void accept(ASTVisitor& v) override {
         v.visit(*this);
      }
      std::string variable() { return _variable; }
      std::shared_ptr<ASTExpression> val() { return _val; }
};

class DontCareAssignmentStatement : public ASTStatement
{
   private:
      std::shared_ptr<ASTExpression> _val;
   public:
      DontCareAssignmentStatement(std::shared_ptr<ASTExpression> val): _val(val) {}
      std::string toString() override {
         return std::string("{\"type\":\"DontCareAssignmentStatement\",\"val\":") + _val->toString() +
            std::string("}");
      }
      std::string toSourceString() override {
         return "_ = " + _val->toSourceString();
      }
      void accept(ASTVisitor& v) override {
         v.visit(*this);
      }
      std::shared_ptr<ASTExpression> val() { return _val; }
};

class FieldUpdateStatement : public ASTStatement
{
   private:
      std::shared_ptr<ASTExpression> _obj;
      std::string _field;
      std::shared_ptr<ASTExpression> _val;
   public:
      FieldUpdateStatement(std::shared_ptr<ASTExpression> obj, std::string field, std::shared_ptr<ASTExpression> val): _obj(obj), _field(field), _val(val) {}
      std::string toString() override {
         return std::string("{\"type\":\"FieldUpdateStatement\",\"obj\":") + _obj->toString() +
            std::string(",\"field\":\"") + _field + std::string("\",\"val\":") + _val->toString() +
            std::string("}");
      }
      std::string toSourceString() override {
         return std::string("!") + _obj->toSourceString() + "." + _field +
            " = " + _val->toSourceString();
      }
      void accept(ASTVisitor& v) override {
         v.visit(*this);
      }
      std::shared_ptr<ASTExpression> obj() { return _obj; }
      std::string field() { return _field; }
      std::shared_ptr<ASTExpression> val() { return _val; }
};

class IfElseStatement : public ASTStatement
{
   private:
      std::shared_ptr<ASTExpression> _cond;
      std::vector<std::shared_ptr<ASTStatement>> _if_statements;
      std::vector<std::shared_ptr<ASTStatement>> _else_statements;
   public:
      IfElseStatement(std::shared_ptr<ASTExpression> cond, std::vector<std::shared_ptr<ASTStatement>> if_statements, std::vector<std::shared_ptr<ASTStatement>> else_statements): _cond(cond), _if_statements(if_statements), _else_statements(else_statements) {}
      std::string toString() override {
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
      std::string toSourceString() override {
         return std::string("if ") + _cond->toSourceString() + ": { ... } else { ... }";
      }
      void accept(ASTVisitor& v) override {
         v.visit(*this);
      }
      std::shared_ptr<ASTExpression> cond() { return _cond; }
      std::vector<std::shared_ptr<ASTStatement>> if_statements() { return _if_statements; }
      std::vector<std::shared_ptr<ASTStatement>> else_statements() { return _else_statements; }
};

class IfOnlyStatement : public ASTStatement
{
   private:
      std::shared_ptr<ASTExpression> _cond;
      std::vector<std::shared_ptr<ASTStatement>> _statements;
   public:
      IfOnlyStatement(std::shared_ptr<ASTExpression> cond, std::vector<std::shared_ptr<ASTStatement>> statements): _cond(cond), _statements(statements) {}
      std::string toString() override {
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
      std::string toSourceString() override {
         return std::string("ifonly ") + _cond->toSourceString() + ": { ... }";
      }
      void accept(ASTVisitor& v) override {
         v.visit(*this);
      }
      std::shared_ptr<ASTExpression> cond() { return _cond; }
      std::vector<std::shared_ptr<ASTStatement>> statements() { return _statements; }
};

class WhileStatement : public ASTStatement
{
   private:
      std::shared_ptr<ASTExpression> _cond;
      std::vector<std::shared_ptr<ASTStatement>> _statements;
   public:
      WhileStatement(std::shared_ptr<ASTExpression> cond, std::vector<std::shared_ptr<ASTStatement>> statements): _cond(cond), _statements(statements) {}
      std::string toString() override {
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
      std::string toSourceString() override {
         return std::string("while ") + _cond->toSourceString() + ": { ... }";
      }
      void accept(ASTVisitor& v) override {
         v.visit(*this);
      }
      std::shared_ptr<ASTExpression> cond() { return _cond; }
      std::vector<std::shared_ptr<ASTStatement>> statements() { return _statements; }
};

class ReturnStatement : public ASTStatement
{
   private:
      std::shared_ptr<ASTExpression> _val;
   public:
      ReturnStatement(std::shared_ptr<ASTExpression> val): _val(val) {}
      std::string toString() override {
         return std::string("{\"type\":\"ReturnStatement\",\"val\":") + _val->toString() +
            std::string("}");
      }
      std::string toSourceString() override {
         return std::string("return ") + _val->toSourceString();
      }
      void accept(ASTVisitor& v) override {
         v.visit(*this);
      }
      std::shared_ptr<ASTExpression> val() { return _val; }
};

class PrintStatement : public ASTStatement
{
   private:
      std::shared_ptr<ASTExpression> _val;
   public:
      PrintStatement(std::shared_ptr<ASTExpression> val): _val(val) {}
      std::string toString() override {
         return std::string("{\"type\":\"PrintStatement\",\"val\":") + _val->toString() +
            std::string("}");
      }
      std::string toSourceString() override {
         return std::string("print ") + _val->toSourceString();
      }
      void accept(ASTVisitor& v) override {
         v.visit(*this);
      }
      std::shared_ptr<ASTExpression> val() { return _val; }
};

class MethodDeclaration : public ASTNode
{
   private:
      std::string _name;
      std::string _return_type;
      std::vector<std::pair<std::string, std::string>> _params;
      std::vector<std::pair<std::string, std::string>> _locals;
      std::vector<std::shared_ptr<ASTStatement>> _statements;
   public:
      MethodDeclaration(std::string name, std::string return_type, std::vector<std::pair<std::string, std::string>> params, std::vector<std::pair<std::string, std::string>> locals, std::vector<std::shared_ptr<ASTStatement>> statements):
         _name(name), _return_type(return_type), _params(params), _locals(locals), _statements(statements) {}
      std::string toString() override {
         std::string out = std::string("{\"type\":\"MethodDeclaration\",\"name\":\"") +
            _name + std::string("\",\"return_type\":\"") + _return_type + std::string("\",\"params\":[");
         int i = 0;
         for (auto & p : _params) {
            if (i > 0) {
               out += std::string(",");
            }
            out += std::string("[\"") + p.first + std::string("\",\"") + p.second + std::string("\"]");
            i++;
         }
         out += std::string("],\"locals\":[");
         i = 0;
         for (auto & l : _locals) {
            if (i > 0) {
               out += std::string(",");
            }
            out += std::string("[\"") + l.first + std::string("\",\"") + l.second + std::string("\"]");
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
      void accept(ASTVisitor& v) override {
         v.visit(*this);
      }
      std::string name() { return _name; }
      std::string return_type() { return _return_type; }
      std::vector<std::pair<std::string, std::string>> params() { return _params; }
      std::vector<std::pair<std::string, std::string>> locals() { return _locals; }
      std::vector<std::shared_ptr<ASTStatement>> statements() { return _statements; }
};

class ClassDeclaration : public ASTNode
{
   private:
      std::string _name;
      std::map<std::string, std::string> _fields;
      std::map<std::string, std::shared_ptr<MethodDeclaration>> _methods;
   public:
      ClassDeclaration(std::string name, std::map<std::string, std::string> fields, std::map<std::string, std::shared_ptr<MethodDeclaration>> methods):
         _name(name), _fields(fields), _methods(methods) {}
      std::string toString() override {
         std::string out = std::string("{\"type\":\"ClassDeclaration\",\"name\":\"") +
            _name + std::string("\",\"fields\":[");
         int i = 0;
         for (auto & f : _fields) {
            if (i > 0) {
               out += std::string(",");
            }
            out += std::string("[\"") + f.first + std::string("\",\"") + f.second + std::string("\"]");
            i++;
         }
         out += std::string("],\"methods\":[");
         i = 0;
         for (auto & m : _methods) {
            if (i > 0) {
               out += std::string(",");
            }
            out += m.second->toString();
            i++;
         }
         out += std::string("]}");
         return out;
      }
      void accept(ASTVisitor& v) override {
         v.visit(*this);
      }
      std::string name() { return _name; }
      std::map<std::string, std::string> fields() { return _fields; }
      std::map<std::string, std::shared_ptr<MethodDeclaration>> methods() { return _methods; }
};

class ProgramDeclaration : public ASTNode
{
   private:
      std::map<std::string, std::shared_ptr<ClassDeclaration>> _classes;
      std::vector<std::pair<std::string, std::string>> _main_locals;
      std::vector<std::shared_ptr<ASTStatement>> _main_statements;
   public:
      ProgramDeclaration(std::map<std::string, std::shared_ptr<ClassDeclaration>> classes, std::vector<std::pair<std::string, std::string>> main_locals, std::vector<std::shared_ptr<ASTStatement>> main_statements):
         _classes(classes), _main_locals(main_locals), _main_statements(main_statements) {}
      std::string toString() override {
         std::string out = std::string("{\"type\":\"ProgramDeclaration\",\"classes\":[");
         int i = 0;
         for (auto & c : _classes) {
            if (i > 0) {
               out += std::string(",");
            }
            out += c.second->toString();
            i++;
         }
         out += std::string("],\"main_locals\":[");
         i = 0;
         for (auto & l : _main_locals) {
            if (i > 0) {
               out += std::string(",");
            }
            out += std::string("[\"") + l.first + std::string("\",\"") + l.second + std::string("\"]");
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
      void accept(ASTVisitor& v) override {
         v.visit(*this);
      }
      std::map<std::string, std::shared_ptr<ClassDeclaration>> classes() { return _classes; }
      std::vector<std::pair<std::string, std::string>> main_locals() { return _main_locals; }
      std::vector<std::shared_ptr<ASTStatement>> main_statements() { return _main_statements; }
};

#endif
