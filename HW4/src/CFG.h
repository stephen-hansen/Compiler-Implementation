#ifndef _CS_441_CFG_H
#define _CS_441_CFG_H
#include <algorithm>
#include <map>
#include <memory>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// Forward declare visitor
class CFGVisitor;

inline std::string toRegister(std::string name) {
   return std::string("%") + name;
}

inline std::string toName(std::string reg) {
   std::string name = reg;
   if (reg.length() > 0 && reg[0] == '%') {
      name.erase(0, 1);
   }
   return name;
}

inline std::string toGlobal(std::string name) {
   return std::string("@") + name;
}

inline std::string toVtable(std::string name) {
   return std::string("vtbl") + name;
}

inline std::string toFieldMap(std::string name) {
   return std::string("fields") + name;
}

inline std::string toMethodName(std::string class_name, std::string method_name) {
   return method_name + class_name;
}

inline bool isTemporary(std::string reg) {
   bool alldigits = true;
   for (size_t i=1; i<reg.length(); i++) {
      if (!std::isdigit(reg[i])) {
         alldigits = false;
         break;
      }
   }
   return reg[0] == '%' && alldigits;
}

inline bool isVariable(std::string reg) {
   bool allalpha = true;
   for (size_t i=1; i<reg.length(); i++) {
      if (!std::isalpha(reg[i])) {
         allalpha = false;
         break;
      }
   }
   return reg[0] == '%' && allalpha;
}

inline bool isNumber(std::string reg) {
   bool alldigit = true;
   for (size_t i=0; i<reg.length(); i++) {
      if (!std::isdigit(reg[i])) {
         alldigit = false;
         break;
      }
   }
   return alldigit;
}

inline bool isGlobal(std::string reg) {
   return reg[0] == '@';
}

class IRStatement
{
   public:
      virtual std::string toString() = 0;
      virtual void accept(CFGVisitor& v) = 0;
};

class PrimitiveStatement : public IRStatement
{};

class ControlStatement : public IRStatement
{};

// Forward declare all specific nodes here
class Comment;
class AssignmentPrimitive;
class ArithmeticPrimitive;
class CallPrimitive;
class PhiPrimitive;
class AllocPrimitive;
class PrintPrimitive;
class GetEltPrimitive;
class SetEltPrimitive;
class LoadPrimitive;
class StorePrimitive;
class FailControl;
class JumpControl;
class IfElseControl;
class RetControl;
class BasicBlock;
class MethodCFG;
class ClassCFG;
class ProgramCFG;

// Declare visitor here
class CFGVisitor
{
   public:
      virtual void visit(Comment& node) = 0;
      virtual void visit(AssignmentPrimitive& node) = 0;
      virtual void visit(ArithmeticPrimitive& node) = 0;
      virtual void visit(CallPrimitive& node) = 0;
      virtual void visit(PhiPrimitive& node) = 0;
      virtual void visit(AllocPrimitive& node) = 0;
      virtual void visit(PrintPrimitive& node) = 0;
      virtual void visit(GetEltPrimitive& node) = 0;
      virtual void visit(SetEltPrimitive& node) = 0;
      virtual void visit(LoadPrimitive& node) = 0;
      virtual void visit(StorePrimitive& node) = 0;
      virtual void visit(FailControl& node) = 0;
      virtual void visit(JumpControl& node) = 0;
      virtual void visit(IfElseControl& node) = 0;
      virtual void visit(RetControl& node) = 0;
      virtual void visit(BasicBlock& node) = 0;
      virtual void visit(MethodCFG& node) = 0;
      virtual void visit(ClassCFG& node) = 0;
      virtual void visit(ProgramCFG& node) = 0;
};

class Comment : public PrimitiveStatement
{
   private:
      std::string _text;
   public:
      Comment(std::string text): _text(text) {}
      std::string text() { return _text; }
      std::string toString() override {
         return "# " + _text;
      }
      void accept(CFGVisitor& v) override {
         v.visit(*this);
      }
};

class AssignmentPrimitive : public PrimitiveStatement
{
   private:
      std::string _lhs;
      std::string _rhs;
   public:
      AssignmentPrimitive(std::string lhs, std::string rhs): _lhs(lhs), _rhs(rhs) {}
      std::string lhs() { return _lhs; }
      std::string rhs() { return _rhs; }
      std::string toString() override {
         return _lhs + " = " + _rhs;
      }
      void accept(CFGVisitor& v) override {
         v.visit(*this);
      }
};

class ArithmeticPrimitive : public PrimitiveStatement
{
   private:
      std::string _lhs;
      std::string _op1;
      char _op;
      std::string _op2;
   public:
      ArithmeticPrimitive(std::string lhs, std::string op1, char op, std::string op2): _lhs(lhs), _op1(op1), _op(op), _op2(op2) {}
      std::string lhs() { return _lhs; }
      std::string op1() { return _op1; }
      char op() { return _op; }
      std::string op2() { return _op2; }
      std::string toString() override {
         return _lhs + " = " + _op1 + " " + _op + " " + _op2;
      }
      void accept(CFGVisitor& v) override {
         v.visit(*this);
      }
};

class CallPrimitive : public PrimitiveStatement
{
   private:
      std::string _lhs;
      std::string _codeaddr;
      std::string _receiver;
      std::vector<std::string> _args;
   public:
      CallPrimitive(std::string lhs, std::string codeaddr, std::string receiver, std::vector<std::string> args):
         _lhs(lhs),
         _codeaddr(codeaddr),
         _receiver(receiver),
         _args(args) {}
      std::string lhs() { return _lhs; }
      std::string codeaddr() { return _codeaddr; }
      std::string receiver() { return _receiver; }
      std::vector<std::string> args() { return _args; }
      std::string toString() override {
         std::stringstream buf;
         buf << _lhs << " = call(" << _codeaddr << ", " << _receiver;
         for (auto & arg : _args) {
            buf << ", " << arg;
         }
         buf << ")";
         return buf.str();
      }
      void accept(CFGVisitor& v) override {
         v.visit(*this);
      }
};

class PhiPrimitive : public PrimitiveStatement
{
   private:
      std::string _lhs;
      std::vector<std::pair<std::string, std::string>> _args;
   public:
      PhiPrimitive(std::string lhs, std::vector<std::pair<std::string, std::string>> args): _lhs(lhs), _args(args) {}
      std::string lhs() { return _lhs; }
      std::vector<std::pair<std::string, std::string>> args() { return _args; }
      std::string toString() override {
         std::stringstream buf;
         buf << _lhs << " = phi(";
         int i = 0;
         for (auto & arg : _args) {
            if (i > 0) {
               buf << ", ";
            }
            buf << arg.first << ", " << arg.second;
            i++;
         }
         buf << ")";
         return buf.str();
      }
      void setArgs(std::vector<std::pair<std::string, std::string>> args) { _args = args; }
      void accept(CFGVisitor& v) override {
         v.visit(*this);
      }
};

class AllocPrimitive : public PrimitiveStatement
{
   private:
      std::string _lhs;
      std::string _size;
   public:
      AllocPrimitive(std::string lhs, std::string size): _lhs(lhs), _size(size) {}
      std::string lhs() { return _lhs; }
      std::string size() { return _size; }
      std::string toString() override {
         return _lhs + " = alloc(" + _size + ")";
      }
      void accept(CFGVisitor& v) override {
         v.visit(*this);
      }
};

class PrintPrimitive : public PrimitiveStatement
{
   private:
      std::string _val;
   public:
      PrintPrimitive(std::string val): _val(val) {}
      std::string val() { return _val; }
      std::string toString() override {
         return "print(" + _val + ")";
      }
      void accept(CFGVisitor& v) override {
         v.visit(*this);
      }
};

class GetEltPrimitive : public PrimitiveStatement
{
   private:
      std::string _lhs;
      std::string _arr;
      std::string _index;
   public:
      GetEltPrimitive(std::string lhs, std::string arr, std::string index): _lhs(lhs), _arr(arr), _index(index) {}
      std::string lhs() { return _lhs; }
      std::string arr() { return _arr; }
      std::string index() { return _index; }
      std::string toString() override {
         return _lhs + " = getelt(" + _arr + ", " + _index + ")";
      }
      void accept(CFGVisitor& v) override {
         v.visit(*this);
      }
};

class SetEltPrimitive : public PrimitiveStatement
{
   private:
      std::string _arr;
      std::string _index;
      std::string _val;
   public:
      SetEltPrimitive(std::string arr, std::string index, std::string val):
         _arr(arr),
         _index(index),
         _val(val) {}
      std::string arr() { return _arr; }
      std::string index() { return _index; }
      std::string val() { return _val; }
      std::string toString() override {
         return std::string("setelt(") + _arr + ", " + _index + ", " + _val + ")";
      }
      void accept(CFGVisitor& v) override {
         v.visit(*this);
      }
};

class LoadPrimitive : public PrimitiveStatement
{
   private:
      std::string _lhs;
      std::string _addr;
   public:
      LoadPrimitive(std::string lhs, std::string addr): _lhs(lhs), _addr(addr) {}
      std::string lhs() { return _lhs; }
      std::string addr() { return _addr; }
      std::string toString() override {
         return _lhs + " = load(" + _addr + ")";
      }
      void accept(CFGVisitor& v) override {
         v.visit(*this);
      }
};

class StorePrimitive : public PrimitiveStatement
{
   private:
      std::string _addr;
      std::string _val;
   public:
      StorePrimitive(std::string addr, std::string val): _addr(addr), _val(val) {}
      std::string addr() { return _addr; }
      std::string val() { return _val; }
      std::string toString() override {
         return "store(" + _addr + ", " + _val + ")";
      }
      void accept(CFGVisitor& v) override {
         v.visit(*this);
      }
};

#define NOT_A_POINTER "NotAPointer"
#define NOT_A_NUMBER "NotANumber"
#define NO_SUCH_FIELD "NoSuchField"
#define NO_SUCH_METHOD "NoSuchMethod"

class FailControl : public ControlStatement
{
   private:
      std::string _message;
   public:
      FailControl(std::string message): _message(message) {}
      std::string message() { return _message; }
      std::string toString() override {
         return "fail " + _message;
      }
      void accept(CFGVisitor& v) override {
         v.visit(*this);
      }
};

class JumpControl : public ControlStatement
{
   private:
      std::string _branch;
   public:
      JumpControl(std::string branch): _branch(branch) {}
      std::string branch() { return _branch; }
      std::string toString() override {
         return "jump " + _branch;
      }
      void accept(CFGVisitor& v) override {
         v.visit(*this);
      }
};

class IfElseControl : public ControlStatement
{
   private:
      std::string _cond;
      std::string _if_branch;
      std::string _else_branch;
   public:
      IfElseControl(std::string cond, std::string if_branch, std::string else_branch):
         _cond(cond),
         _if_branch(if_branch),
         _else_branch(else_branch) {}
      std::string cond() { return _cond; }
      std::string if_branch() { return _if_branch; }
      std::string else_branch() { return _else_branch; }
      std::string toString() override {
         return "if " + _cond + " then " + _if_branch + " else " + _else_branch;
      }
      void accept(CFGVisitor& v) override {
         v.visit(*this);
      }
};

class RetControl : public ControlStatement
{
   private:
      std::string _val;
   public:
      RetControl(std::string val): _val(val) {}
      std::string val() { return _val; }
      std::string toString() override {
         return "ret " + _val;
      }
      void accept(CFGVisitor& v) override {
         v.visit(*this);
      }
};

class BasicBlock
{
   private:
      std::string _label;
      std::vector<std::string> _params;
      std::vector<std::shared_ptr<PrimitiveStatement>> _primitives;
      std::shared_ptr<ControlStatement> _control = std::make_shared<RetControl>("0");
      std::vector<std::shared_ptr<BasicBlock>> _children;
      std::vector<std::weak_ptr<BasicBlock>> _weak_children;
      std::vector<std::weak_ptr<BasicBlock>> _predecessors;
      bool _unreachable = false;
   public:
      BasicBlock(std::string label): _label(label) {}
      std::string label() { return _label; }
      std::vector<std::string> params() { return _params; }
      void set_params(std::vector<std::string> params) { _params = params; }
      std::vector<std::shared_ptr<PrimitiveStatement>> primitives() { return _primitives; }
      std::shared_ptr<ControlStatement> control() { return _control; }
      std::vector<std::shared_ptr<BasicBlock>> children() { return _children; }
      std::vector<std::weak_ptr<BasicBlock>> weak_children() { return _weak_children; }
      std::vector<std::weak_ptr<BasicBlock>> predecessors() { return _predecessors; }
      BasicBlock(std::string label, std::vector<std::string> params): _label(label), _params(params) {}
      bool isUnreachable() {
         return _unreachable;
      }
      void setUnreachable(bool u) {
         _unreachable = u;
      }
      void insertPrimitive(std::shared_ptr<PrimitiveStatement> ps) {
         _primitives.insert(_primitives.begin(), ps);
      }
      void appendPrimitive(std::shared_ptr<PrimitiveStatement> ps) {
         _primitives.push_back(ps);
      }
      void removePrimitive(std::shared_ptr<PrimitiveStatement> ps) {
         _primitives.erase(std::remove(_primitives.begin(), _primitives.end(), ps), _primitives.end());
      }
      void setControl(std::shared_ptr<ControlStatement> c) {
         _control = c;
      }
      void addPredecessor(std::weak_ptr<BasicBlock> b) {
         _predecessors.push_back(b);
      }
      // Push any children created with NEW inside the current scope
      void addNewChild(std::shared_ptr<BasicBlock> b) {
         _children.push_back(b);
      }
      // Push any existing children
      // Needed to break cyclic references in destructor
      // Also helps for traversal purposes to avoid repeated blocks
      void addExistingChild(std::weak_ptr<BasicBlock> b) {
         _weak_children.push_back(b);
      }
      std::string toString() {
         std::stringstream buf;
         buf << _label;
         if (_params.size() > 0) {
            buf << "(";
            int i = 0;
            for (auto & param : _params) {
               if (i > 0) {
                  buf << ", ";
               }
               buf << toName(param);
               i++;
            }
            buf << ")";
         }
         buf << ":\n";
         for (auto & p : _primitives) {
            buf << "  " << p->toString() << "\n";
         }
         buf << "  " << _control->toString() << "\n";
         return buf.str();
      }
      std::string toStringRecursive() {
         std::stringstream buf;
         buf << toString();
         for (auto & next : _children) {
            buf << next->toStringRecursive();
         }
         return buf.str();
      }
      void accept(CFGVisitor& v) {
         v.visit(*this);
      }
};

inline void addNewChild(std::shared_ptr<BasicBlock> & b1, std::shared_ptr<BasicBlock> & b2) {
   b1->addNewChild(b2);
   b2->addPredecessor(b1);
}

inline void addExistingChild(std::shared_ptr<BasicBlock> & b1, std::shared_ptr<BasicBlock> & b2) {
   b1->addExistingChild(b2);
   b2->addPredecessor(b1);
}

class MethodCFG
{
   private:
      std::shared_ptr<BasicBlock> _first_block;
      std::vector<std::string> _variables;
      std::map<std::string, std::string> _var_to_type;
   public:
      MethodCFG(std::shared_ptr<BasicBlock> first_block, std::vector<std::string> variables,
            std::map<std::string, std::string> var_to_type):
         _first_block(first_block),
         _variables(variables),
         _var_to_type(var_to_type) {}
      std::string toString() {
         return _first_block->toStringRecursive();
      }
      void accept(CFGVisitor& v) {
         v.visit(*this);
      }
      std::shared_ptr<BasicBlock> first_block() { return _first_block; }
      std::vector<std::string> variables() { return _variables; }
      std::map<std::string, std::string> var_to_type() { return _var_to_type; }
      void setType(std::string v, std::string t) { _var_to_type[v] = t; }
      std::string getType(std::string v) { return _var_to_type[v]; }
};

class ClassCFG
{
   private:
      std::string _name;
      std::vector<std::shared_ptr<MethodCFG>> _methods;
      std::vector<std::string> _vtable;
      std::map<std::string, unsigned long> _field_table;
      std::map<std::string, std::string> _field_to_type;
   public:
      ClassCFG(std::string name, std::vector<std::string> vtable, std::map<std::string, unsigned long> field_table,
            std::map<std::string, std::string> field_to_type):
         _name(name),
         _vtable(vtable),
         _field_table(field_table),
         _field_to_type(field_to_type) {}
      std::string dataString() {
         std::stringstream buf;
         // Write vtbl
         buf << "global array " << toVtable(_name) << ": { ";
         int i = 0;
         for (auto & m : _vtable) {
            if (i > 0) {
               buf << ", ";
            }
            buf << m;
            i++; 
         }
         buf << " }\n";
         return buf.str();
      }
      std::string toString() {
         std::stringstream buf;
         // Just write out all of the method code
         for (auto & m : _methods) {
            buf << m->toString();
         }
         return buf.str();
      }
      std::string name() { return _name; }
      std::vector<std::shared_ptr<MethodCFG>> methods() { return _methods; }
      std::vector<std::string> vtable() { return _vtable; }
      std::map<std::string, unsigned long> field_table() { return _field_table; }
      std::map<std::string, std::string> field_to_type() { return _field_to_type; }
      void setType(std::string f, std::string t) { _field_to_type[f] = t; }
      std::string getType(std::string f) { return _field_to_type[f]; }
      void appendMethod(std::shared_ptr<MethodCFG> m) {
         _methods.push_back(m);
      }
      void accept(CFGVisitor& v) {
         v.visit(*this);
      }
};

class ProgramCFG
{
   private:
      std::shared_ptr<MethodCFG> _main_method;
      std::map<std::string, std::shared_ptr<ClassCFG>> _classes;
   public:
      ProgramCFG(std::shared_ptr<MethodCFG> main_method): _main_method(main_method) {}
      std::string toString() {
         std::stringstream buf;
         // Write data (vtbl, fields)
         buf << "data:\n";
         for (auto & kv : _classes) {
            buf << kv.second->dataString();
         }
         // Write code
         buf << "code:\n\n";
         // Write class methods
         for (auto & kv : _classes) {
            buf << kv.second->toString();
            buf << "\n";
         }
         // Write main method
         buf << _main_method->toString();
         return buf.str();
      }
      void appendClass(std::shared_ptr<ClassCFG> c) {
         _classes[c->name()] = c;
      }
      void accept(CFGVisitor& v) {
         v.visit(*this);
      }
      std::shared_ptr<MethodCFG> main_method() { return _main_method; }
      std::map<std::string, std::shared_ptr<ClassCFG>> classes() { return _classes; }
};

#endif
