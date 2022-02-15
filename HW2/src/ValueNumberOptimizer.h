#ifndef _CS_441_VALUE_NUMBER_OPTIMIZER_H
#define _CS_441_VALUE_NUMBER_OPTIMIZER_H
#include <map>
#include <stack>
#include <vector>
#include "CFG.h"
#include "DominatorSolver.h"
#include "IdentityOptimizer.h"

// TODO helper which iterates direct children for phi node adjustment

// This will iterate dominator tree instead of literal tree
class ValueNumberOptimizer : public IdentityOptimizer
{
   private:
      std::map<std::pair<char, std::vector<std::string>>, std::string> _hashtable;
      std::stack<std::map<std::pair<char, std::vector<std::string>>, std::string>> _htstack;
      std::map<std::string, std::string> _vn;
      std::map<std::string, std::shared_ptr<DomTreeNode>> _domtree;
      std::string getVN(std::string rhs) {
         if (_vn.find(rhs) == _vn.end()) {
            _vn[rhs] = rhs;
         }
         return _vn[rhs];
      }
   public:
      // Comment doesn't need adjustment
      void visit(AssignmentPrimitive& node) {
         // Check if expr is in hash table
         std::string rhs = getVN(node.rhs());
         std::vector<std::string> arg = { rhs };
         std::pair<char, std::vector<std::string>> hash = std::make_pair('=', arg);
         if (_hashtable.find(hash) != _hashtable.end()) {
            // In hash table
            // Map node's LHS to hash associated VN
            _vn[node.lhs()] = _hashtable[hash];
            // By default do NOT add primitive
         } else {
            // Not in hash table
            // Add it
            _vn[node.lhs()] = node.lhs();
            _hashtable[hash] = node.lhs();
            // Build modified expression
            _new_block->appendPrimitive(std::make_shared<AssignmentPrimitive>(
                     node.lhs(),
                     rhs));
         } 
      }
      void visit(ArithmeticPrimitive& node) {
         char op = node.op();
         std::string op1 = getVN(node.op1());
         std::string op2 = getVN(node.op2());
         std::vector<std::string> args = { op1, op2 };
         // Can we simplify expr?
         // If op is +, *, |, &, ^, args are commutative, sort them for consistency
         if (op == '+' || op == '*' || op == '|' || op == '&' || op == '^') {
            std::sort(args.begin(), args.end());
         }
         if (op == '+') {
            // Addition identities
            if (args[0] == "0") {
               // Direct assignment x = 0 + y
               op = '=';
               args = { args[1] };
            } else if (args[1] == "0") {
               // Direct assignment x = y + 0
               op = '=';
               args = { args[0] };
            } else if (args[0] == args[1]) {
               // x = y + y = 2 * y
               op = '*';
               args = { "2", args[0] };
               std::sort(args.begin(), args.end());
            }
         } else if (op == '-') {
            // Subtraction identities
            if (args[1] == "0") {
               // Direct assignment x = y - 0
               op = '=';
               args = { args[0] };
            } else if (args[0] == args[1]) {
               // x = y - y = 0
               op = '=';
               args = { "0" };
            }
         } else if (op == '*') {
            if (args[0] == "1") {
               // Direct assignment x = 1 * y
               op = '=';
               args = { args[1] };
            } else if (args[1] == "1") {
               // Direct assignment x = y * 1
               op = '=';
               args = { args[0] };
            } else if (args[0] == "0" || args[1] == "0") {
               // Direct assignment x = 0 * y = 0 = y * 0
               op = '=';
               args = { "0" };
            }
         } else if (op == '/') {
            // Cannot simplify 0 / y if y might be 0
            if (args[1] == "1") {
               // Direct assignment x = y / 1
               op = '=';
               args = { args[0] };
            }
            // Cannot simplify y / y if y might be 0
         }
         // Check if expr is in hash table
         std::pair<char, std::vector<std::string>> hash = std::make_pair(op, args);
         if (_hashtable.find(hash) != _hashtable.end()) {
            // In hash table
            // Map node's LHS to hash associated VN
            _vn[node.lhs()] = _hashtable[hash];
            // By default do NOT add primitive
         } else {
            // Not in hash table
            // Add it
            _vn[node.lhs()] = node.lhs();
            _hashtable[hash] = node.lhs();
            // Build modified expression
            if (op == '=') {
               _new_block->appendPrimitive(std::make_shared<AssignmentPrimitive>(
                        node.lhs(),
                        args[0]));
            } else {
               _new_block->appendPrimitive(std::make_shared<ArithmeticPrimitive>(
                  node.lhs(),
                  args[0],
                  op,
                  args[1]));
            }
         } 
      }
      void visit(CallPrimitive& node) {
         std::vector<std::string> args = node.args();
         std::vector<std::string> new_args;
         for (const auto& arg : args) {
            new_args.push_back(getVN(arg));
         }
         _new_block->appendPrimitive(std::make_shared<CallPrimitive>(
                  node.lhs(),
                  getVN(node.codeaddr()),
                  getVN(node.receiver()),
                  new_args));
      }
      void visit(PhiPrimitive& node) {
         // TODO handle
         std::vector<std::pair<std::string, std::string>> args = node.args();
         std::vector<std::pair<std::string, std::string>> new_args;
         std::vector<std::string> args_for_hash;
         for (const auto& arg : args) {
            std::string vnarg = getVN(arg.second);
            args_for_hash.push_back(vnarg);
            new_args.push_back(std::make_pair(arg.first, vnarg));
         }
         std::sort(args_for_hash.begin(), args_for_hash.end());
         std::pair<char, std::vector<std::string>> hash = std::make_pair('p', args_for_hash);
         // Is Phi meaningless?
         if (std::all_of(new_args.begin(), new_args.end(),
                  [&] (std::pair<std::string, std::string> p) { return p.second == new_args[0].second; })) {
            // All values have same value number
            // Replace lhs value number with the value number
            _vn[node.lhs()] = new_args[0].second;
            // Phi removed (not part of new CFG)
         } else if (_hashtable.find(hash) != _hashtable.end()) {
            // In hash table already
            _vn[node.lhs()] = _hashtable[hash];
            // Phi removed (not part of new CFG)
         } else {
            // Still need phi
            _new_block->appendPrimitive(std::make_shared<PhiPrimitive>(node.lhs(), new_args));
            // Phi's VN is itself
            _vn[node.lhs()] = node.lhs();
            // Add to hash table
            _hashtable[hash] = node.lhs();
         }
      }
      void visit(AllocPrimitive& node) {
         _new_block->appendPrimitive(std::make_shared<AllocPrimitive>(node.lhs(), getVN(node.size())));
      }
      void visit(PrintPrimitive& node) {
         _new_block->appendPrimitive(std::make_shared<PrintPrimitive>(getVN(node.val())));
      }
      void visit(GetEltPrimitive& node) {
         _new_block->appendPrimitive(std::make_shared<GetEltPrimitive>(node.lhs(), getVN(node.arr()), getVN(node.index())));
      }
      void visit(SetEltPrimitive& node) {
         _new_block->appendPrimitive(std::make_shared<SetEltPrimitive>(
                  getVN(node.arr()),
                  getVN(node.index()),
                  getVN(node.val())));
      }
      void visit(LoadPrimitive& node) {
         _new_block->appendPrimitive(std::make_shared<LoadPrimitive>(node.lhs(), getVN(node.addr())));
      }
      void visit(StorePrimitive& node) {
         _new_block->appendPrimitive(std::make_shared<StorePrimitive>(getVN(node.addr()), getVN(node.val())));
      }
      void visit(FailControl& node) {
         _new_block->setControl(std::make_shared<FailControl>(node.message()));
      }
      void visit(JumpControl& node) {
         _new_block->setControl(std::make_shared<JumpControl>(node.branch()));
      }
      void visit(IfElseControl& node) {
         _new_block->setControl(std::make_shared<IfElseControl>(getVN(node.cond()), node.if_branch(), node.else_branch()));
      }
      void optimizeChild(BasicBlock& node, std::shared_ptr<BasicBlock>& c) {
         std::string label = node.label();
         std::string child_label = c->label();
         if (!_label_to_block.count(child_label)) {
            _label_to_block[child_label] = std::make_shared<BasicBlock>(child_label, c->params());
         }
         addNewChild(_label_to_block[label], _label_to_block[child_label]);
         // DO NOT OPTIMIZE CHILD DIRECTLY
         // TODO ADJUST PHI FUNC INPUTS
      }
      void visit(BasicBlock& node) {
         // This will do a copy on assignment
         _hashtable = _htstack.top();
         // Optimize with current table
         optimizeBlock(node);
         optimizeChildren(node); 
         // TODO directly update children phi nodes
         // Need to traverse using dominator tree
         std::shared_ptr<DomTreeNode> domnode = _domtree[node.label()];
         std::vector<std::shared_ptr<DomTreeNode>> children = domnode->children();
         // Push the modified table onto the stack
         _htstack.push(_hashtable);
         for (const auto& child : children) {
            std::shared_ptr<BasicBlock> block = child->block();
            block->accept(*this);
         }
         // Clean up hash table
         // Remove current level
         _htstack.pop();
      }
      void visit(MethodCFG& node) {
         // Clear VN, hash table
         _vn.clear();
         _hashtable.clear();
         // Call parent method to optimize
         IdentityOptimizer::visit(node);
      }
      void visit(ClassCFG& node) {
         std::string name = node.name();
         std::vector<std::string> vtable = node.vtable();
         std::vector<unsigned long> field_table = node.field_table();
         // Create new class
         _new_class = std::make_shared<ClassCFG>(name, vtable, field_table);
         // Optimize every method
         std::vector<std::shared_ptr<MethodCFG>> methods = node.methods();
         for (auto & m : methods) {
            DominatorSolver ds;
            _domtree = ds.solveTree(m);
            m->accept(*this);
            _new_class->appendMethod(_new_method);
         }
      }
      void visit(ProgramCFG& node) {
         std::shared_ptr<MethodCFG> main = node.main_method();
         DominatorSolver ds;
         _domtree = ds.solveTree(main);
         // Optimize main
         main->accept(*this);
         // Set main method
         _new_prog = std::make_shared<ProgramCFG>(_new_method);
         // Get classes
         std::map<std::string, std::shared_ptr<ClassCFG>> classes = node.classes();
         // Optimize each and append
         for (auto & kv : classes) {
            kv.second->accept(*this);
            _new_prog->appendClass(_new_class);
         }
      }
};

#endif

