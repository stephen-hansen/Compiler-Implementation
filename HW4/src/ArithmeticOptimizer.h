#ifndef _CS_441_ARITHMETIC_OPTIMIZER_H
#define _CS_441_ARITHMETIC_OPTIMIZER_H
#include <map>
#include "CFG.h"
#include "IdentityOptimizer.h"

class ArithmeticOptimizer : public IdentityOptimizer
{
   private:
      std::string adjustTemp(std::string reg) {
         return _temp_to_const.count(reg) ? _temp_to_const[reg] : reg;
      }
      void appendPrimitive(std::string lhs, std::string rhs, std::shared_ptr<PrimitiveStatement> ps) {
         // If the lhs is a temporary and rhs number, remove the statement and store new value internally
         // Otherwise, add the specified primitive
         if (isTemporary(lhs) && isNumber(rhs)) {
            _temp_to_const[lhs] = rhs;
         } else {
            _new_block->appendPrimitive(ps);
         }
      }
   protected:
      std::map<std::string, std::string> _temp_to_const;
   public:
      // Comment doesn't need adjustment
      void visit(AssignmentPrimitive& node) {
         std::string lhs = node.lhs();
         std::string rhs = adjustTemp(node.rhs());
         appendPrimitive(lhs, rhs, std::make_shared<AssignmentPrimitive>(lhs, rhs));
      }
      void visit(ArithmeticPrimitive& node) {
         // Update operands
         std::string op1 = adjustTemp(node.op1());
         std::string op2 = adjustTemp(node.op2());
         std::string lhs = node.lhs();
         char op = node.op();
         if (isNumber(op1) && isNumber(op2)) {
            // We can directly update the value
            std::string newval;
            unsigned int newval_num;
            unsigned int op1_num = std::stoul(op1);
            unsigned int op2_num = std::stoul(op2);
            if (op == '+') {
               newval_num = op1_num + op2_num;
            } else if (op == '-') {
               newval_num = op1_num - op2_num;
            } else if (op == '*') {
               newval_num = op1_num * op2_num;
            } else if (op == '/') {
               newval_num = op1_num / op2_num;
            } else if (op == '&') {
               newval_num = op1_num & op2_num;
            } else if (op == '|') {
               newval_num = op1_num | op2_num;
            } else {
               // Assume xor
               newval_num = op1_num ^ op2_num;
            }
            newval = std::to_string(newval_num);
            appendPrimitive(lhs, newval, std::make_shared<AssignmentPrimitive>(lhs, newval));
         } else {
            // Append block with adjusted ops
            _new_block->appendPrimitive(std::make_shared<ArithmeticPrimitive>(
                  lhs,
                  op1,
                  op,
                  op2));
         }
      }
      void visit(CallPrimitive& node) {
         std::string lhs = node.lhs();
         std::string codeaddr = adjustTemp(node.codeaddr());
         std::string receiver = adjustTemp(node.receiver());
         std::vector<std::string> args;
         for (auto & a : node.args()) {
            args.push_back(adjustTemp(a));
         }
         _new_block->appendPrimitive(std::make_shared<CallPrimitive>(
                  lhs,
                  codeaddr,
                  receiver,
                  args));
      }
      void visit(PhiPrimitive& node) {
         std::string lhs = node.lhs();
         std::vector<std::pair<std::string, std::string>> args;
         for (auto & a : node.args()) {
            args.push_back(std::make_pair(a.first, adjustTemp(a.second)));
         }
         _new_block->appendPrimitive(std::make_shared<PhiPrimitive>(lhs, args));
      }
      void visit(AllocPrimitive& node) {
         std::string lhs = node.lhs();
         std::string size = adjustTemp(node.size());
         _new_block->appendPrimitive(std::make_shared<AllocPrimitive>(lhs, size));
      }
      void visit(PrintPrimitive& node) {
         std::string val = adjustTemp(node.val());
         _new_block->appendPrimitive(std::make_shared<PrintPrimitive>(val));
      }
      void visit(GetEltPrimitive& node) {
         std::string lhs = node.lhs();
         std::string arr = adjustTemp(node.arr());
         std::string index = adjustTemp(node.index());
         _new_block->appendPrimitive(std::make_shared<GetEltPrimitive>(lhs, arr, index));
      }
      void visit(SetEltPrimitive& node) {
         std::string arr = adjustTemp(node.arr());
         std::string index = adjustTemp(node.index());
         std::string val = adjustTemp(node.val());
         _new_block->appendPrimitive(std::make_shared<SetEltPrimitive>(arr, index, val));
      }
      void visit(LoadPrimitive& node) {
         std::string lhs = node.lhs();
         std::string addr = adjustTemp(node.addr());
         _new_block->appendPrimitive(std::make_shared<LoadPrimitive>(lhs, addr));
      }
      void visit(StorePrimitive& node) {
         std::string addr = adjustTemp(node.addr());
         std::string val = adjustTemp(node.val());
         _new_block->appendPrimitive(std::make_shared<StorePrimitive>(addr, val));
      }
      void visit(LoadVectorPrimitive& node) {
         std::vector<std::string> args;
         for (const auto & v : node.vals()) {
            args.push_back(adjustTemp(v));
         }
         _new_block->appendPrimitive(std::make_shared<LoadVectorPrimitive>(adjustTemp(node.lhs()), args));
      }
      void visit(StoreVectorPrimitive& node) {
         std::vector<std::string> vals;
         for (const auto & v : node.vals()) {
            vals.push_back(adjustTemp(v));
         }
         _new_block->appendPrimitive(std::make_shared<StoreVectorPrimitive>(vals, adjustTemp(node.rhs())));
      }
      void visit(AddVectorPrimitive& node) {
         _new_block->appendPrimitive(std::make_shared<AddVectorPrimitive>(adjustTemp(node.lhs()), adjustTemp(node.op1()), adjustTemp(node.op2())));
      }
      void visit(SubtractVectorPrimitive& node) {
         _new_block->appendPrimitive(std::make_shared<SubtractVectorPrimitive>(adjustTemp(node.lhs()), adjustTemp(node.op1()), adjustTemp(node.op2())));
      }
      void visit(MultiplyVectorPrimitive& node) {
         _new_block->appendPrimitive(std::make_shared<MultiplyVectorPrimitive>(adjustTemp(node.lhs()), adjustTemp(node.op1()), adjustTemp(node.op2())));
      }
      void visit(DivideVectorPrimitive& node) {
         _new_block->appendPrimitive(std::make_shared<DivideVectorPrimitive>(adjustTemp(node.lhs()), adjustTemp(node.op1()), adjustTemp(node.op2())));
      }
      // FailControl doesn't need adjustment
      // JumpControl doesn't need adjustment
      void visit(IfElseControl& node) {
         std::string cond = adjustTemp(node.cond());
         std::string if_branch = node.if_branch();
         std::string else_branch = node.else_branch();
         _new_block->setControl(std::make_shared<IfElseControl>(cond, if_branch, else_branch));
      }
      void visit(RetControl& node) {
         std::string val = adjustTemp(node.val());
         _new_block->setControl(std::make_shared<RetControl>(val));
      }
      void visit(MethodCFG& node) {
         // Wipe map on each method
         // Each method has its own map
         _temp_to_const.clear();
         // Call parent method
         IdentityOptimizer::visit(node);
      }
};

#endif

