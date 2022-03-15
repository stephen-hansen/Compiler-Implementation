#ifndef _CS_441_BETTER_SSA_OPTIMIZER_H
#define _CS_441_BETTER_SSA_OPTIMIZER_H
#include <map>
#include <set>
#include "CFG.h"
#include "DominatorSolver.h"
#include "SSAOptimizer.h"

// Helper class to determine phi node placements
class BetterSSAHelper : public CFGVisitor
{
   protected:
      std::string _curr_label;
      std::map<std::string, std::set<std::string>> _curr_df;
      std::map<std::string, std::set<std::string>> _var_to_blocks;
      std::set<std::string> _globals;
      std::set<std::string> _varkill;
      std::map<std::string, std::set<std::string>> _label_to_phi_variables;


      void updateGlobalsAndBlocks(std::vector<std::string> rhs) {
         for (const auto& r : rhs) {
            if (isVariable(r) && (_varkill.find(r) == _varkill.end())) {
               _globals.insert(r);
            }
         }
      }
      void updateGlobalsAndBlocks(std::vector<std::string> lhs, std::vector<std::string> rhs) {
         updateGlobalsAndBlocks(rhs);
         for (const auto& l : lhs) {
            if (isVariable(l)) {
               _varkill.insert(l);
               if (_var_to_blocks.find(l) == _var_to_blocks.end()) {
                  _var_to_blocks[l] = std::set<std::string>({});
               }
               _var_to_blocks[l].insert(_curr_label);
            }
         }
      }
   public:
      // Do nothing on comment
      void visit(Comment& node) {}
      void visit(AssignmentPrimitive& node) {
         updateGlobalsAndBlocks({ node.lhs() }, { node.rhs() });
      }
      void visit(ArithmeticPrimitive& node) {
         updateGlobalsAndBlocks({ node.lhs() }, { node.op1(), node.op2() });
      }
      void visit(CallPrimitive& node) {
         std::vector<std::string> rhs;
         std::vector<std::string> temp = { node.codeaddr(), node.receiver() };
         std::vector<std::string> args = node.args();
         rhs.reserve(temp.size() + args.size());
         rhs.insert(rhs.end(), temp.begin(), temp.end());
         rhs.insert(rhs.end(), args.begin(), args.end());
         updateGlobalsAndBlocks({ node.lhs() }, rhs);
      }
      // Do nothing on phi (should not have any)
      void visit(PhiPrimitive& node) {}
      void visit(AllocPrimitive& node) {
         updateGlobalsAndBlocks({ node.lhs() }, { node.size() });
      }
      void visit(PrintPrimitive& node) {
         updateGlobalsAndBlocks({ node.val() });
      }
      void visit(GetEltPrimitive& node) {
         updateGlobalsAndBlocks({ node.lhs() }, { node.arr(), node.index() });
      }
      void visit(SetEltPrimitive& node) {
         updateGlobalsAndBlocks({ node.arr(), node.index(), node.val() });
      }
      void visit(LoadPrimitive& node) {
         updateGlobalsAndBlocks({ node.lhs() }, { node.addr() });
      }
      void visit(StorePrimitive& node) {
         updateGlobalsAndBlocks({ node.addr(), node.val() });
      }
      void visit(LoadVectorPrimitive& node) {
         updateGlobalsAndBlocks({ node.lhs() }, node.vals());
      }
      void visit(StoreVectorPrimitive& node) {
         updateGlobalsAndBlocks(node.vals(), { node.rhs() });
      }
      void visit(AddVectorPrimitive& node) {
         updateGlobalsAndBlocks({ node.lhs() }, { node.op1(), node.op2() });
      }
      void visit(SubtractVectorPrimitive& node) {
         updateGlobalsAndBlocks({ node.lhs() }, { node.op1(), node.op2() });
      }
      void visit(MultiplyVectorPrimitive& node) {
         updateGlobalsAndBlocks({ node.lhs() }, { node.op1(), node.op2() });
      }
      void visit(DivideVectorPrimitive& node) {
         updateGlobalsAndBlocks({ node.lhs() }, { node.op1(), node.op2() });
      }
      // No update needed for fail control
      void visit(FailControl& node) {}
      // No update needed for jump control
      void visit(JumpControl& node) {}
      void visit(IfElseControl& node) {
         updateGlobalsAndBlocks({ node.cond() });
      }
      void visit(RetControl& node) {
         updateGlobalsAndBlocks({ node.val() });
      }
      void visit(BasicBlock& node) {
         // Empty set
         _varkill = std::set<std::string>({});
         _curr_label = node.label();
         // Create varkill, update globals, blocks
         std::vector<std::shared_ptr<PrimitiveStatement>> primitives = node.primitives();
         for (auto & p : primitives) {
            p->accept(*this);
         }
         node.control()->accept(*this);
         for (auto & c : node.children()) {
            c->accept(*this);
         }
      }
      void visit(MethodCFG& node) {
         // Initialize to empty set
         _globals = std::set<std::string>({});
         // for each block in the main method
         std::shared_ptr<BasicBlock> first_block = node.first_block();
         first_block->accept(*this);
         // Globals, Blocks are set up now
         for (const auto & x : _globals) {
            if (_var_to_blocks.find(x) != _var_to_blocks.end()) {
               std::set<std::string> worklist = _var_to_blocks[x];
               while (worklist.size() > 0) {
                  std::set<std::string> next_worklist = std::set<std::string>({});
                  for (const auto & b : worklist) {
                     for (const auto & d : _curr_df[b]) {
                        // If d has no phi-func for x
                        if (_label_to_phi_variables.find(d) == _label_to_phi_variables.end()) {
                           _label_to_phi_variables[d] = std::set<std::string>({});
                        }
                        if (_label_to_phi_variables[d].find(x) == _label_to_phi_variables[d].end()) {
                           _label_to_phi_variables[d].insert(x);
                           next_worklist.insert(d);
                        }
                     }
                  }
                  worklist = next_worklist;
               }
            }
         }
      }
      void visit(ClassCFG& node) {
         std::vector<std::shared_ptr<MethodCFG>> methods = node.methods();
         for (auto & m : methods) {
            DominatorSolver ds;
            _curr_df = ds.solveDF(m);
            m->accept(*this);
         }
      }
      void visit(ProgramCFG& node) {
         std::shared_ptr<MethodCFG> main = node.main_method();
         DominatorSolver ds;
         _curr_df = ds.solveDF(main);
         main->accept(*this);
         // Get classes
         std::map<std::string, std::shared_ptr<ClassCFG>> classes = node.classes();
         for (auto & kv : classes) {
            kv.second->accept(*this);
         }
      }
      std::map<std::string, std::set<std::string>> get_phi_variables(ProgramCFG& p) {
         p.accept(*this);
         return _label_to_phi_variables;
      }
};



class BetterSSAOptimizer : public SSAOptimizer
{
   protected:
      std::map<std::string, std::set<std::string>> _label_to_phi_variables;
   public:
      void visit(BasicBlock& node) {
         // Set params to version "zero"
         for (auto & p : node.params()) {
            if (isVariable(p) && !_global_counters.count(p)) {
               _global_counters[p] = 0;
            }
         }
         std::string label = node.label();
         _label_to_method[label] = _new_method;
         std::vector<std::string> variables = _new_method->variables();
         std::map<std::string, unsigned int> pre_counters;
         // Check if the block will need phi node later
         if (_label_to_phi_variables.find(label) != _label_to_phi_variables.end()) {
            std::set<std::string> phi_vars = _label_to_phi_variables[label];
            for (auto & v : phi_vars) {
               // Increment by 1, we will add phi in later
               adjustLHSVariable(v);
            }
         }
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
         // Call parent method to add in the versioning, set _new_method
         IdentityOptimizer::visit(node);
         // Post-process phi statements
         for (auto & kv : _label_to_block) {
            std::string label = kv.first;
            std::shared_ptr<BasicBlock> block = kv.second;
            std::vector<std::string> newParams;
            // Dumb hack to make parameters first "version"
            for (auto & p : block->params()) {
               if (isVariable(p)) {
                  std::string ogtype = _new_method->getType(p);
                  p += "0";
                  _new_method->setType(p, ogtype);
               }
               newParams.push_back(p);
            }
            block->set_params(newParams);
            if (_label_to_phi_variables.find(label) != _label_to_phi_variables.end()) {
               std::set<std::string> phi_vars = _label_to_phi_variables[label];
               for (auto & v : phi_vars) {
                  std::string reg = v;
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
      void visit(ProgramCFG& node) {
         // Load in phi statement locations with helper
         BetterSSAHelper helper;
         _label_to_phi_variables = helper.get_phi_variables(node);
         // Optimize program
         IdentityOptimizer::visit(node);
      }
};

#endif

