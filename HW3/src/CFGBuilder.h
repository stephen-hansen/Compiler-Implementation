#ifndef _CS_441_CFGBUILDER_H
#define _CS_441_CFGBUILDER_H
#include <exception>
#include <map>
#include <stack>
#include <string>
#include <utility>
#include "AST.h"
#include "CFG.h"

#define BADPOINTER "badpointer"
#define BADNUMBER "badnumber"
#define BADFIELD "badfield"
#define BADMETHOD "badmethod"
#define TEMP ""
#define INT "int"
#define VTBL "vtbl"
#define METHOD "methodptr"

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
      std::map<std::string, unsigned long> _name_counter;
      std::map<std::string, unsigned long> _class_name_to_alloc_size;
      std::map<std::string, unsigned long> _class_name_to_num_fields;
      std::map<std::string, unsigned long> _field_to_map_offset;
      std::map<std::string, unsigned long> _method_to_vtable_offset;
      std::stack<std::pair<std::string, std::string>> _return_values;
      std::stack<std::string> _input_values;
      std::shared_ptr<BasicBlock> _curr_block;
      std::shared_ptr<MethodCFG> _curr_method;
      std::shared_ptr<ClassCFG> _curr_class;
      std::shared_ptr<ProgramCFG> _curr_program;
      std::shared_ptr<ProgramDeclaration> _program_ast;
      void resetCounter(std::string name = "") {
         _name_counter[name] = 1;
      }
      std::string createName(std::string name = "") {
         if (!_name_counter.count(name)) {
            _name_counter[name] = 1;
         }
         return name + std::to_string(_name_counter[name]++);
      }
      std::string createTemp(std::string type) {
         std::string reg = toRegister(createName());
         _curr_class->setType(reg, type);
         return reg;
      }
      std::string createLabel() {
         return createName("l");
      }
      std::string setReturnName(std::string retType) {
         std::string ret = _input_values.top();
         _input_values.pop();
         if (ret == TEMP) {
            ret = createTemp(retType);
         }
         _return_values.push(std::make_pair(ret, retType));
         return ret;
      }
      void nonzeroCheck(std::string reg, std::string failLabel, std::string failMsg) {
         // Build failure block
         std::string failureBlockLabel = createName(failLabel);
         std::shared_ptr<BasicBlock> failureBlock = std::make_shared<BasicBlock>(failureBlockLabel);
         failureBlock->setControl(std::make_shared<FailControl>(failMsg)); 
         // Build success block
         std::string nextBlockLabel = createLabel();
         std::shared_ptr<BasicBlock> successBlock = std::make_shared<BasicBlock>(nextBlockLabel);
         // Curr block owns success block
         addNewChild(_curr_block, successBlock);
         // Curr block owns failure block
         addNewChild(_curr_block, failureBlock);
         _curr_block->setControl(std::make_shared<IfElseControl>(reg, nextBlockLabel, failureBlockLabel));
         // Remove current block and replace with success block
         _curr_block = successBlock;
      }

   public:
      void visit(UInt32Literal& node);
      void visit(VariableIdentifier& node);
      void visit(ArithmeticExpression& node); 
      void visit(CallExpression& node);
      void visit(FieldReadExpression& node);
      void visit(NewObjectExpression& node);
      void visit(ThisObjectExpression& node);
      void visit(NullObjectExpression& node);
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
      std::shared_ptr<ProgramCFG> build(std::shared_ptr<ProgramDeclaration> p);
};

#endif
