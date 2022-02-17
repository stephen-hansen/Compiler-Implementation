#ifndef _CS_441_VALUE_NUMBER_OPTIMIZER_H
#define _CS_441_VALUE_NUMBER_OPTIMIZER_H
#include <map>
#include <set>
#include <stack>
#include <vector>
#include "CFG.h"
#include "DominatorSolver.h"
#include "IdentityOptimizer.h"

// This will iterate dominator tree instead of literal tree
class ValueNumberOptimizer : public IdentityOptimizer
{
   private:
      bool _modified;
      std::map<std::pair<char, std::vector<std::string>>, std::string> _hashtable;
      std::stack<std::map<std::pair<char, std::vector<std::string>>, std::string>> _htstack;
      std::map<std::string, std::string> _vn;
      std::map<std::string, std::shared_ptr<DomTreeNode>> _domtree;
      std::set<std::string> _prunelabels;
      std::set<std::string> _ownedlabels;
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
            // Note that we could just say it has a definitive value number of
            // the right hand side. So let's do that.
            _vn[node.lhs()] = rhs;
            _hashtable[hash] = rhs;
            // Don't need modified expression, we can just inline it
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
            } else if (args[0] == "2") {
               // Replace with addition
               op = '+';
               args = { args[1], args[1] };
            } else if (args[1] == "2") {
               // Replace with addition
               op = '+';
               args = { args[0], args[0] };
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
         // Check if we have const expression and replace it
         if (op != '=' && isNumber(args[0]) && isNumber(args[1])) {
            std::string arg;
            if (op == '+') {
               arg = std::to_string(std::stoull(args[0]) + std::stoull(args[1]));
            } else if (op == '-') {
               arg = std::to_string(std::stoull(args[0]) - std::stoull(args[1]));
            } else if (op == '*') {
               arg = std::to_string(std::stoull(args[0]) * std::stoull(args[1]));
            } else if (op == '/') {
               arg = std::to_string(std::stoull(args[0]) / std::stoull(args[1]));
            } else if (op == '&') {
               arg = std::to_string(std::stoull(args[0]) & std::stoull(args[1]));
            } else if (op == '|') {
               arg = std::to_string(std::stoull(args[0]) | std::stoull(args[1]));
            } else {
               // Assume XOR
               arg = std::to_string(std::stoull(args[0]) ^ std::stoull(args[1]));
            }
            op = '=';
            // Might need to remap to value number
            arg = getVN(arg);
            args = { arg };
         }
         // Check if expr is in hash table
         std::pair<char, std::vector<std::string>> hash = std::make_pair(op, args);
         if (_hashtable.find(hash) != _hashtable.end()) {
            // In hash table
            // Map node's LHS to hash associated VN
            _vn[node.lhs()] = _hashtable[hash];
            // By default do NOT add primitive
         } else {
            // Build modified expression
            if (op == '=') {
               // Can just assign VN of RHS
               // Just map it
               _vn[node.lhs()] = args[0];
               _hashtable[hash] = args[0];
            } else {
               // Not in hash table
               // Add it
               _vn[node.lhs()] = node.lhs();
               _hashtable[hash] = node.lhs();
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
         std::vector<std::pair<std::string, std::string>> args = node.args();
         std::vector<std::pair<std::string, std::string>> new_args;
         std::vector<std::string> args_for_hash;
         for (const auto& arg : args) {
            // Check if label is still valid
            if (_domtree.find(arg.first) != _domtree.end()) {
               std::string vnarg = getVN(arg.second);
               args_for_hash.push_back(vnarg);
               new_args.push_back(std::make_pair(arg.first, vnarg));
            }
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
         std::string cond = getVN(node.cond());
         if (isNumber(cond)) {
            if (cond != "0") {
               // Go to if branch
               _new_block->setControl(std::make_shared<JumpControl>(node.if_branch()));
               // Prune else
               _prunelabels.insert(node.else_branch());
            } else {
               // Go to else branch
               _new_block->setControl(std::make_shared<JumpControl>(node.else_branch()));
               // Prune if
               _prunelabels.insert(node.if_branch());
            }
            // Re-run later
            _modified = true;
         } else {
            // Check if it is a tag check
            bool is_tagcheck = false;
            bool use_else = false;
            std::pair<char, std::vector<std::string>> hash;
            std::vector<std::string> args;
            std::string if_label = node.if_branch();
            std::string else_label = node.else_branch();
            // badpointer, badmethod, badfield, badnumber
            if (if_label.find("badpointer") != std::string::npos) {
               is_tagcheck = true;
               args = std::vector<std::string>({ cond, "badpointer" });
               use_else = true;
            } else if (else_label.find("badmethod") != std::string::npos) {
               is_tagcheck = true;
               args = std::vector<std::string>({ cond, "badmethod" });
            } else if (else_label.find("badfield") != std::string::npos) {
               is_tagcheck = true;
               args = std::vector<std::string>({ cond, "badfield" });
            } else if (else_label.find("badnumber") != std::string::npos) {
               is_tagcheck = true;
               args = std::vector<std::string>({ cond, "badnumber" });
            }
            if (is_tagcheck) {
               hash = std::make_pair('#', args);
               // Have we seen this tag check before
               if (_hashtable.find(hash) != _hashtable.end()) {
                  // Replace with jump
                  if (use_else) {
                     _new_block->setControl(std::make_shared<JumpControl>(else_label));
                     _prunelabels.insert(if_label);
                  } else {
                     _new_block->setControl(std::make_shared<JumpControl>(if_label));
                     _prunelabels.insert(else_label);
                  }
                  return; 
               } else {
                  // Just put in hash table with dummy value
                  _hashtable[hash] = cond;
               }
            }
            _new_block->setControl(std::make_shared<IfElseControl>(cond, node.if_branch(), node.else_branch()));
         }
      }
      void visit(RetControl& node) {
         _new_block->setControl(std::make_shared<RetControl>(getVN(node.val())));
      }
      void adjustChildPhi(std::shared_ptr<BasicBlock>& c) {
         std::string child_label = c->label();
         // DO NOT OPTIMIZE CHILD DIRECTLY
         // ADJUST PHI FUNC INPUTS IN BOTH _LABEL_TO_BLOCK AND C
         std::vector<std::shared_ptr<PrimitiveStatement>> primitives = c->primitives();
         for (auto& pr : primitives) {
            PhiPrimitive * p = dynamic_cast<PhiPrimitive*>(pr.get());
            if (p != nullptr) {
               std::vector<std::pair<std::string, std::string>> args = p->args();
               std::vector<std::pair<std::string, std::string>> new_args;
               for (const auto& arg : args) {
                  std::string vnarg = getVN(arg.second);
                  new_args.push_back(std::make_pair(arg.first, vnarg));
               }
               if (new_args != args) {
                  _modified = true;
                  p->setArgs(new_args);
               }
            }
         }
         primitives = _label_to_block[child_label]->primitives();
         for (auto& pr : primitives) {
            PhiPrimitive * p = dynamic_cast<PhiPrimitive*>(pr.get());
            if (p != nullptr) {
               std::vector<std::pair<std::string, std::string>> args = p->args();
               std::vector<std::pair<std::string, std::string>> new_args;
               for (const auto& arg : args) {
                  std::string vnarg = getVN(arg.second);
                  new_args.push_back(std::make_pair(arg.first, vnarg));
               }
               if (new_args != args) {
                  _modified = true;
                  p->setArgs(new_args);
               }
            }
         }
      }
      void optimizeChild(BasicBlock& node, std::shared_ptr<BasicBlock>& c) {
         std::string label = node.label();
         std::string child_label = c->label();
         if (_prunelabels.find(child_label) != _prunelabels.end()) {
            // Dead-code, prune
            return;
         }
         if (!_label_to_block.count(child_label)) {
            _label_to_block[child_label] = std::make_shared<BasicBlock>(child_label, c->params());
         }
         if (_ownedlabels.find(child_label) == _ownedlabels.end()) {
            addNewChild(_label_to_block[label], _label_to_block[child_label]);
            _ownedlabels.insert(child_label);
         } else {
            addExistingChild(_label_to_block[label], _label_to_block[child_label]);
         }
         adjustChildPhi(c);
      }
      void buildWeakChildConns(BasicBlock& node) {
         std::string label = node.label();
         // Can now build weak connections
         std::vector<std::weak_ptr<BasicBlock>> weak_children = node.weak_children();
         for (auto & wc : weak_children) {
            std::string child_label = wc.lock()->label();
            if (_prunelabels.find(child_label) != _prunelabels.end()) {
               // Dead-code, prune
               continue;
            }
            if (!_label_to_block.count(child_label)) {
               _label_to_block[child_label] = std::make_shared<BasicBlock>(child_label, wc.lock()->params());
            }
            if (_ownedlabels.find(child_label) == _ownedlabels.end()) {
               addNewChild(_label_to_block[label], _label_to_block[child_label]);
               _ownedlabels.insert(child_label);
            } else {
               addExistingChild(_label_to_block[label], _label_to_block[child_label]);
            }
            std::shared_ptr<BasicBlock> child = wc.lock();
            adjustChildPhi(child);
            // No recursion
         }
      }
      void optimizeChildren(BasicBlock& node) {
         std::string label = node.label();
         // Now optimize every child block recursively
         // Don't use _new_block here since recursion could change it
         // Create block first then visit
         std::vector<std::shared_ptr<BasicBlock>> children = node.children();
         for (auto & c : children) {
            optimizeChild(node, c);
         }
         buildWeakChildConns(node);
      }
      void visit(BasicBlock& node) {
         // This will do a copy on assignment
         _hashtable = _htstack.top();
         // No labels to prune (at the moment)
         _prunelabels.clear();
         // Optimize with current table
         optimizeBlock(node);
         // This will fix phi funcs of successors
         optimizeChildren(node); 
         // Need to traverse using dominator tree
         std::shared_ptr<DomTreeNode> domnode = _domtree[node.label()];
         std::vector<std::shared_ptr<DomTreeNode>> children = domnode->children();
         // Push the modified table onto the stack
         _htstack.push(_hashtable);
         std::vector<std::shared_ptr<DomTreeNode>> acceptable_children;
         // Only keep non-pruned blocks
         for (const auto& child : children) {
            if (_prunelabels.find(child->block()->label()) == _prunelabels.end()) {
               acceptable_children.push_back(child);
            }
         }
         for (const auto& child : acceptable_children) {
            std::shared_ptr<BasicBlock> block = child->block();
            block->accept(*this);
         }
         // Clean up hash table
         // Remove current level
         _htstack.pop();
      }
      void visit(MethodCFG& node) {
         _modified = true;
         while (_modified) {
            DominatorSolver ds;
            _domtree = ds.solveTree(std::make_shared<MethodCFG>(node));
            _modified = false;
            // Clear VN, hash table
            _prunelabels.clear();
            _ownedlabels.clear();
            _vn.clear();
            _hashtable.clear();
            _htstack.push(_hashtable);
            // Call parent method to optimize
            IdentityOptimizer::visit(node);
            _htstack.pop();
            node = *_new_method;
         }
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
            m->accept(*this);
            _new_class->appendMethod(_new_method);
         }
      }
      void visit(ProgramCFG& node) {
         std::shared_ptr<MethodCFG> main = node.main_method();
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

