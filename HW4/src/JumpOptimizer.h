#ifndef _CS_441_JUMP_OPTIMIZER_H
#define _CS_441_JUMP_OPTIMIZER_H
#include <map>
#include "IdentityOptimizer.h"

class JumpOptimizer : public IdentityOptimizer
{
   private:
      std::map<std::string, std::string> _jump_labels_to_prior;
      std::map<std::string, std::string> _prune_labels_to_prior;
   public:
      void visit(JumpControl& node) {
         IdentityOptimizer::visit(node);
         // Might need to prune branch later
         std::string label = node.branch();
         _jump_labels_to_prior[label] = _new_block->label();
      }
      void visit(IfElseControl& node) {
         IdentityOptimizer::visit(node);
         // Might need to prune branch later
         std::string if_label = node.if_branch();
         std::string else_label = node.else_branch();
         if (isNumber(node.cond())) {
            int cond = std::stoi(node.cond());
            if (cond) {
               _jump_labels_to_prior[if_label] = _new_block->label();
               _prune_labels_to_prior[else_label] = _new_block->label();
            } else {
               _jump_labels_to_prior[else_label] = _new_block->label();
               _prune_labels_to_prior[if_label] = _new_block->label();
            }
         }
      }
      void visit(BasicBlock& node) {
         // Optimize block and all children
         IdentityOptimizer::visit(node);
         // Check if we are a jump label
         if (_jump_labels_to_prior.find(node.label()) != _jump_labels_to_prior.end()) {
            // Check if we have only one predecessor
            std::shared_ptr<BasicBlock> block = _label_to_block[node.label()];
            if (block->predecessors().size() == 1) {
               // Merge blocks up (postorder)
               std::shared_ptr<BasicBlock> mergeIn = _label_to_block[_jump_labels_to_prior[node.label()]];
               // Merge in primitives
               for (const auto & p : block->primitives()) {
                  mergeIn->appendPrimitive(p);
               }
               // Merge in control
               mergeIn->setControl(block->control());
               // Set new children
               mergeIn->setChildren(block->children(), block->weak_children());
            }
         }
         // Check if we are a prune label
         if (_prune_labels_to_prior.find(node.label()) != _prune_labels_to_prior.end()) {
            // Check if we have only one predecessor
            std::shared_ptr<BasicBlock> block = _label_to_block[node.label()];
            if (block->predecessors().size() == 1 && block->predecessors()[0].lock()->label() == _prune_labels_to_prior[node.label()]) {
               // Remove block from children
               std::shared_ptr<BasicBlock> removeFrom = _label_to_block[_prune_labels_to_prior[node.label()]];
               std::vector<std::shared_ptr<BasicBlock>> children = removeFrom->children();
               std::vector<std::shared_ptr<BasicBlock>> newChildren;
               for (const auto & c : children) {
                  if (c->label() != node.label()) {
                     newChildren.push_back(c);
                  }
               }
               std::vector<std::weak_ptr<BasicBlock>> wchildren = removeFrom->weak_children();
               std::vector<std::weak_ptr<BasicBlock>> newWChildren;
               for (const auto & wc : wchildren) {
                  if (wc.lock()->label() != node.label()) {
                     newWChildren.push_back(wc);
                  }
               }
               // Set new children
               removeFrom->setChildren(newChildren, newWChildren);
            }
         }
      }
};

#endif

