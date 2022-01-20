#ifndef _CS_441_CFG_H
#define _CS_441_CFG_H
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

std::string toRegister(std::string name) {
   return std::string("%") + name;
}

class IRStatement
{
   public:
      virtual std::string toString() = 0;
};

class PrimitiveStatement : public IRStatement
{};

class ControlStatement : public IRStatement
{};

class Comment : public PrimitiveStatement
{
   private:
      std::string _text;
   public:
      Comment(std::string text): _text(text) {}
      std::string toString() override {
         return "# " + _text;
      }
};

class AssignmentPrimitive : public PrimitiveStatement
{
   private:
      std::string _lhs;
      std::string _rhs;
   public:
      AssignmentPrimitive(std::string lhs, std::string rhs): _lhs(lhs), _rhs(rhs) {}
      std::string toString() override {
         return _lhs + " = " + _rhs;
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
      std::string toString() override {
         return _lhs + " = " + _op1 + " " + _op + " " + _op2;
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
      std::string toString() override {
         std::stringstream buf;
         buf << _lhs << " = call(" << _codeaddr << ", " << _receiver;
         for (auto & arg : _args) {
            buf << ", " << arg;
         }
         buf << ")";
         return buf.str();
      }
};

class PhiPrimitive : public PrimitiveStatement
{
   private:
      std::string _lhs;
      std::vector<std::pair<std::string, std::string>> _args;
   public:
      PhiPrimitive(std::string lhs, std::vector<std::pair<std::string, std::string>> args): _lhs(lhs), _args(args) {}
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
};

class AllocPrimitive : public PrimitiveStatement
{
   private:
      std::string _lhs;
      std::string _size;
   public:
      AllocPrimitive(std::string lhs, std::string size): _lhs(lhs), _size(size) {}
      std::string toString() override {
         return _lhs + " = alloc(" + _size + ")";
      }
};

class PrintPrimitive : public PrimitiveStatement
{
   private:
      std::string _val;
   public:
      PrintPrimitive(std::string val): _val(val) {}
      std::string toString() override {
         return "print(" + _val + ")";
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
      std::string toString() override {
         return _lhs + " = getelt(" + _arr + ", " + _index + ")";
      }
};

class SetEltPrimitive : public PrimitiveStatement
{
   private:
      std::string _lhs;
      std::string _arr;
      std::string _index;
      std::string _val;
   public:
      SetEltPrimitive(std::string lhs, std::string arr, std::string index, std::string val):
         _lhs(lhs),
         _arr(arr),
         _index(index),
         _val(val) {}
      std::string toString() override {
         return _lhs + " = setelt(" + _arr + ", " + _index + ", " + _val + ")";
      }
};

class LoadPrimitive : public PrimitiveStatement
{
   private:
      std::string _lhs;
      std::string _addr;
   public:
      LoadPrimitive(std::string lhs, std::string addr): _lhs(lhs), _addr(addr) {}
      std::string toString() override {
         return _lhs + " = load(" + _addr + ")";
      }
};

class StorePrimitive : public PrimitiveStatement
{
   private:
      std::string _addr;
      std::string _val;
   public:
      StorePrimitive(std::string addr, std::string val): _addr(addr), _val(val) {}
      std::string toString() override {
         return "store(" + _addr + ", " + _val + ")";
      }
};

#define NOT_A_POINTER "NotAPointer"
#define NOT_A_NUMBER "NotANumber"
#define NO_SUCH_FIELD "NoSuchField"
#define NO_SUCH_METHOD "NoSuchMethod"

class FailPrimitive : public PrimitiveStatement
{
   private:
      std::string _message;
   public:
      FailPrimitive(std::string message): _message(message) {}
      std::string toString() override {
         return "fail " + _message;
      }
};

class JumpControl : public ControlStatement
{
   private:
      std::string _branch;
   public:
      JumpControl(std::string branch): _branch(branch) {}
      std::string toString() override {
         return "jump " + _branch;
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
      std::string toString() override {
         return "if " + _cond + " then " + _if_branch + " else " + _else_branch;
      }
};

class RetControl : public ControlStatement
{
   private:
      std::string _val;
   public:
      RetControl(std::string val): _val(val) {}
      std::string toString() override {
         return "ret " + _val;
      }
};

class BasicBlock
{
   private:
      std::string _label;
      std::vector<std::string> _params;
      std::vector<std::shared_ptr<PrimitiveStatement>> _primitives;
      std::shared_ptr<ControlStatement> _control;
      std::vector<std::shared_ptr<BasicBlock>> _children;
      std::vector<std::weak_ptr<BasicBlock>> _weak_children;
   public:
      BasicBlock(std::string label): _label(label) {}
      void appendPrimitive(std::shared_ptr<PrimitiveStatement> ps) {
         _primitives.push_back(ps);
      }
      void setControl(std::shared_ptr<ControlStatement> c) {
         _control = c;
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
               buf << param;
               i++;
            }
            buf << ")";
         }
         buf << ":\n";
         for (auto & p : _primitives) {
            buf << "  " << p->toString() << "\n";
         }
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
};

class MethodCFG
{
   private:
      std::shared_ptr<BasicBlock> _first_block;
   public:
      std::string toString() {
         return _first_block->toStringRecursive();
      }
};

class ClassCFG
{
   private:
      std::string _name;
      std::string _vtbl_name;
      std::string _fields_name;
      unsigned long _alloc_size;
      std::vector<std::shared_ptr<MethodCFG>> _methods;
      std::vector<std::string> _vtable;
      std::vector<unsigned long> _field_table;
   public:
      std::string dataString() {
         std::stringstream buf;
         // Write vtbl
         buf << "global array " << _vtbl_name << ": { ";
         int i = 0;
         for (auto & m : _vtable) {
            if (i > 0) {
               buf << ", ";
            }
            buf << m;
            i++; 
         }
         buf << " }\n";
         // Write fields
         buf << "global array " << _fields_name << ": { ";
         i = 0;
         for (auto & f : _field_table) {
            if (i > 0) {
               buf << ", ";
            }
            buf << f;
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
};

class ProgramCFG
{
   private:
      std::shared_ptr<MethodCFG> _main_method;
      std::map<std::string, std::shared_ptr<ClassCFG>> _classes;
   public:
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
};

#endif
