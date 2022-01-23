#ifndef _CS_441_IDENTITY_OPTIMIZER_H
#define _CS_441_IDENTITY_OPTIMIZER_H
#include "CFG.h"

// Build new CFG identical to previous doing a graph traversal
// Intent is to inherit in actual optimizers, and only change
// functions that matter (so that you don't have to override
// for every primitive/control)
class IdentityOptimizer : public CFGVisitor
{
   protected:
      std::shared_ptr<ProgramCFG> _new_prog;
      std::shared_ptr<MethodCFG> _new_method;
      std::shared_ptr<ClassCFG> _new_class;
      std::shared_ptr<BasicBlock> _new_block;
      std::map<std::string, std::shared_ptr<BasicBlock>> _label_to_block;
   public:
      // Intentionally call appendPrimitive on each visit to add a copy
      // If we overwrite, we can specify exactly what gets appended
      void visit(Comment& node) {
         _new_block->appendPrimitive(std::make_shared<Comment>(node.text()));
      }
      void visit(AssignmentPrimitive& node) {
         _new_block->appendPrimitive(std::make_shared<AssignmentPrimitive>(node.lhs(), node.rhs()));
      }
      void visit(ArithmeticPrimitive& node) {
         _new_block->appendPrimitive(std::make_shared<ArithmeticPrimitive>(
                  node.lhs(),
                  node.op1(),
                  node.op(),
                  node.op2()));
      }
      void visit(CallPrimitive& node) {
         _new_block->appendPrimitive(std::make_shared<CallPrimitive>(
                  node.lhs(),
                  node.codeaddr(),
                  node.receiver(),
                  node.args()));
      }
      void visit(PhiPrimitive& node) {
         _new_block->appendPrimitive(std::make_shared<PhiPrimitive>(node.lhs(), node.args()));
      }
      void visit(AllocPrimitive& node) {
         _new_block->appendPrimitive(std::make_shared<AllocPrimitive>(node.lhs(), node.size()));
      }
      void visit(PrintPrimitive& node) {
         _new_block->appendPrimitive(std::make_shared<PrintPrimitive>(node.val()));
      }
      void visit(GetEltPrimitive& node) {
         _new_block->appendPrimitive(std::make_shared<GetEltPrimitive>(node.lhs(), node.arr(), node.index()));
      }
      void visit(SetEltPrimitive& node) {
         _new_block->appendPrimitive(std::make_shared<SetEltPrimitive>(
                  node.lhs(),
                  node.arr(),
                  node.index(),
                  node.val()));
      }
      void visit(LoadPrimitive& node) {
         _new_block->appendPrimitive(std::make_shared<LoadPrimitive>(node.lhs(), node.addr()));
      }
      void visit(StorePrimitive& node) {
         _new_block->appendPrimitive(std::make_shared<StorePrimitive>(node.addr(), node.val()));
      }
      void visit(FailControl& node) {
         _new_block->setControl(std::make_shared<FailControl>(node.message()));
      }
      void visit(JumpControl& node) {
         _new_block->setControl(std::make_shared<JumpControl>(node.branch()));
      }
      void visit(IfElseControl& node) {
         _new_block->setControl(std::make_shared<IfElseControl>(node.cond(), node.if_branch(), node.else_branch()));
      }
      void visit(RetControl& node) {
         _new_block->setControl(std::make_shared<RetControl>(node.val()));
      }
      void visit(BasicBlock& node) {
         std::string label = node.label();
         // Assume incoming node is already copied into _label_to_block
         _new_block = _label_to_block[label];
         // Optimize each primitive
         std::vector<std::shared_ptr<PrimitiveStatement>> primitives = node.primitives();
         for (auto & p : primitives) {
            p->accept(*this);
         }
         // Optimize control
         node.control()->accept(*this);
         // Now optimize every child block recursively
         // Don't use _new_block here since recursion could change it
         // Create block first then visit
         std::vector<std::shared_ptr<BasicBlock>> children = node.children();
         for (auto & c : children) {
            std::string child_label = c->label();
            _label_to_block[child_label] = std::make_shared<BasicBlock>(child_label, c->params());
            _label_to_block[label]->addNewChild(_label_to_block[child_label]);
            c->accept(*this);
         }
         // At end, we assume that entire tree has been parsed
         // Can now build weak connections
         std::vector<std::weak_ptr<BasicBlock>> weak_children = node.weak_children();
         for (auto & wc : weak_children) {
            std::string child_label = wc.lock()->label();
            _label_to_block[label]->addExistingChild(_label_to_block[child_label]);
         }
      }
      void visit(MethodCFG& node) {
         std::shared_ptr<BasicBlock> first_block = node.first_block();
         std::string label = first_block->label();
         std::vector<std::string> params = first_block->params();
         // Make first block and new method
         _label_to_block[label] = std::make_shared<BasicBlock>(label, params);
         _new_method = std::make_shared<MethodCFG>(_label_to_block[label]);
         // Optimize first block
         first_block->accept(*this);
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
      std::shared_ptr<ProgramCFG> optimize(std::shared_ptr<ProgramCFG> p) {
         // Optimize program
         p->accept(*this);
         // Return optimized program
         return _new_prog;
      }
};

#endif

