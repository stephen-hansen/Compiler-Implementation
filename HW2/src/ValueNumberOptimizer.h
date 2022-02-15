#ifndef _CS_441_VALUE_NUMBER_OPTIMIZER_H
#define _CS_441_VALUE_NUMBER_OPTIMIZER_H
#include <map>
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
      std::map<std::string, std::string> _vn;
      std::string getVN(std::string rhs) {
         if (_vn.find(rhs) == _vn.end()) {
            _vn[rhs] = rhs;
         }
         return _vn[rhs];
      }
   public:
      // Comment doesn't need adjustment
      void visit(AssignmentPrimitive& node) {
         // TODO handle
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
         // TODO handle
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

      void visit(MethodCFG& node) {
         // Clear VN, hash table
         _vn.clear();
         _hashtable.clear();
         // Call parent method to optimize
         IdentityOptimizer::visit(node);
      }
};

#endif

