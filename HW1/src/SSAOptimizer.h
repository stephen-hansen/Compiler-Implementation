#ifndef _CS_441_SSA_OPTIMIZER_H
#define _CS_441_SSA_OPTIMIZER_H
#include <map>
#include "CFG.h"
#include "IdentityOptimizer.h"

class SSAOptimizer : public IdentityOptimizer
{
   private:
      // LHS adjustments must be called before RHS adjustments
      // LHS will replace variable with + current version, but will increment current
      // version prior to replacement
      std::string adjustLHSVariable(std::string reg) {
         if (isVariable(reg)) {
            if (!_global_counters.count(reg)) {
               _global_counters[reg] = 0;
            } else {
               _global_counters[reg] += 1;
            }
            // Update set counter to new global counter
            _set_counter[reg] = _global_counters[reg];
            return reg + std::to_string(_global_counters[reg]);
         }
         return reg;
      }
      // Replace variable registers with variable + current version for variable
      std::string adjustRHSVariable(std::string reg) {
         if (isVariable(reg)) {
            if (!_set_counter.count(reg)) {
               _set_counter[reg] = 0;
            }
            return reg + std::to_string(_set_counter[reg]);
         }
         return reg;
      }
   protected:
      // Map block label to counters mapping variable name to final version at end of block
      std::map<std::string, std::map<std::string, unsigned int>> _label_to_post_counters;
      std::map<std::string, std::map<std::string, unsigned int>> _label_to_pre_counters;
      // Map var x to next unused version #
      // Useful if creating a new version #
      std::map<std::string, unsigned int> _global_counters;
      // Map var x to most recently used version #
      // May be different from global # in regard to nested expressions
      std::map<std::string, unsigned int> _set_counter;
      std::map<std::string, std::shared_ptr<MethodCFG>> _label_to_method;
   public:
      // Comment doesn't need adjustment
      void visit(AssignmentPrimitive& node) {
         std::string rhs = adjustRHSVariable(node.rhs());
         std::string lhs = adjustLHSVariable(node.lhs());
         _new_block->appendPrimitive(std::make_shared<AssignmentPrimitive>(lhs, rhs));
      }
      void visit(ArithmeticPrimitive& node) {
         // Update operands
         std::string op1 = adjustRHSVariable(node.op1());
         std::string op2 = adjustRHSVariable(node.op2());
         char op = node.op();
         std::string lhs = adjustLHSVariable(node.lhs());
         _new_block->appendPrimitive(std::make_shared<ArithmeticPrimitive>(
                  lhs,
                  op1,
                  op,
                  op2));
      }
      void visit(CallPrimitive& node) {
         std::string codeaddr = adjustRHSVariable(node.codeaddr());
         std::string receiver = adjustRHSVariable(node.receiver());
         std::vector<std::string> args;
         for (auto & a : node.args()) {
            args.push_back(adjustRHSVariable(a));
         }
         std::string lhs = adjustLHSVariable(node.lhs());
         _new_block->appendPrimitive(std::make_shared<CallPrimitive>(
                  lhs,
                  codeaddr,
                  receiver,
                  args));
      }
      void visit(PhiPrimitive& node) {
         // Might not need to do anything here
         std::vector<std::pair<std::string, std::string>> args;
         for (auto & a : node.args()) {
            args.push_back(std::make_pair(a.first, adjustRHSVariable(a.second)));
         }
         std::string lhs = adjustLHSVariable(node.lhs());
         _new_block->appendPrimitive(std::make_shared<PhiPrimitive>(lhs, args));
      }
      void visit(AllocPrimitive& node) {
         std::string size = adjustRHSVariable(node.size());
         std::string lhs = adjustLHSVariable(node.lhs());
         _new_block->appendPrimitive(std::make_shared<AllocPrimitive>(lhs, size));
      }
      void visit(PrintPrimitive& node) {
         std::string val = adjustRHSVariable(node.val());
         _new_block->appendPrimitive(std::make_shared<PrintPrimitive>(val));
      }
      void visit(GetEltPrimitive& node) {
         std::string arr = adjustRHSVariable(node.arr());
         std::string index = adjustRHSVariable(node.index());
         std::string lhs = adjustLHSVariable(node.lhs());
         _new_block->appendPrimitive(std::make_shared<GetEltPrimitive>(lhs, arr, index));
      }
      void visit(SetEltPrimitive& node) {
         std::string arr = adjustRHSVariable(node.arr());
         std::string index = adjustRHSVariable(node.index());
         std::string val = adjustRHSVariable(node.val());
         _new_block->appendPrimitive(std::make_shared<SetEltPrimitive>(arr, index, val));
      }
      void visit(LoadPrimitive& node) {
         std::string addr = adjustRHSVariable(node.addr());
         std::string lhs = adjustLHSVariable(node.lhs());
         _new_block->appendPrimitive(std::make_shared<LoadPrimitive>(lhs, addr));
      }
      void visit(StorePrimitive& node) {
         std::string addr = adjustRHSVariable(node.addr());
         std::string val = adjustRHSVariable(node.val());
         _new_block->appendPrimitive(std::make_shared<StorePrimitive>(addr, val));
      }
      // FailControl doesn't need adjustment
      // JumpControl doesn't need adjustment
      void visit(IfElseControl& node) {
         std::string cond = adjustRHSVariable(node.cond());
         std::string if_branch = node.if_branch();
         std::string else_branch = node.else_branch();
         _new_block->setControl(std::make_shared<IfElseControl>(cond, if_branch, else_branch));
      }
      void visit(RetControl& node) {
         std::string val = adjustRHSVariable(node.val());
         _new_block->setControl(std::make_shared<RetControl>(val));
      }
      void optimizeChildren(BasicBlock& node) {
         std::string label = node.label();
         std::vector<std::shared_ptr<BasicBlock>> children = node.children();
         for (auto & c : children) {
            // Reset set counter to parent's counter
            // Represents incoming values of variables
            _set_counter = _label_to_post_counters[label];
            optimizeChild(node, c);
         }
         buildWeakChildConns(node);
      }
      void visit(BasicBlock& node) {
         _label_to_method[node.label()] = _new_method;
         std::vector<std::string> variables = _new_method->variables();
         std::map<std::string, unsigned int> pre_counters;
         // Check if the block will need phi node later
         if (node.predecessors().size() > 1) {
            // YES - go through and increment every variable by 1
            for (auto & v : variables) {
               adjustLHSVariable(toRegister(v));
            }
         }
         std::string label = node.label();
         // Map counters at start of block to label
         _label_to_pre_counters[label] = _set_counter;
         // Optimize block like before
         optimizeBlock(node);
         // Store final counters in a map using copy on assignment
         _label_to_post_counters[label] = _set_counter;
         // Optimize children
         optimizeChildren(node);
      }
      void visit(MethodCFG& node) {
         // Reset global counters on new method
         _global_counters.clear();
         _set_counter.clear();
         // Grab the method variables
         std::vector<std::string> variables = node.variables();
         // Call parent method to add in the versioning
         IdentityOptimizer::visit(node);
      }
      void visit(ProgramCFG& node) {
         // Optimize program
         IdentityOptimizer::visit(node);
         // Post-process phi statements
         for (auto & kv : _label_to_block) {
            std::string label = kv.first;
            std::shared_ptr<BasicBlock> block = kv.second;
            std::vector<std::string> newParams;
            // Dumb hack to make parameters first "version"
            for (auto & p : block->params()) {
               if (isVariable(p)) {
                  p += "0";
               }
               newParams.push_back(p);
            }
            block->set_params(newParams);
            if (block->predecessors().size() > 1) {
               for (auto & v : _label_to_method[label]->variables()) {
                  std::string reg = toRegister(v);
                  std::string lhs = reg + std::to_string(_label_to_pre_counters[label][reg]);
                  std::vector<std::pair<std::string, std::string>> phi_args;
                  for (auto & p : block->predecessors()) {
                     std::string pred_label = p.lock()->label();
                     std::string pred_var = reg + std::to_string(_label_to_post_counters[pred_label][reg]);
                     phi_args.push_back(std::make_pair(pred_label, pred_var));
                  }
                  // Insert phi at start
                  block->insertPrimitive(std::make_shared<PhiPrimitive>(lhs, phi_args));
               }
            }
         }
      }
};

#endif

