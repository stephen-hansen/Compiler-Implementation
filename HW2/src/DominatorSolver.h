#ifndef _CS_441_DOMINATOR_SOLVER_H
#define _CS_441_DOMINATOR_SOLVER_H
#include <algorithm>
#include <map>
#include <string>
#include <set>
#include "CFG.h"

class DominatorSolver
{
   private:
      void updateBlockmap(std::map<std::string, std::shared_ptr<BasicBlock>> & blockmap,
            std::shared_ptr<BasicBlock> block) {
         blockmap[block->label()] = block;
         for (const auto& child : block->children()) {
            updateBlockmap(blockmap, child);
         }
      }
      
   public:
      // generate map of labels to blocks
      std::map<std::string, std::shared_ptr<BasicBlock>> solveBlockmap(std::shared_ptr<MethodCFG> m) {
         std::map<std::string, std::shared_ptr<BasicBlock>> blockmap;
         updateBlockmap(blockmap, m->first_block());
         return blockmap;
      }
      // generate Dom(n) for a given method
      std::map<std::string, std::set<std::string>> solveDom(
            std::map<std::string, std::shared_ptr<BasicBlock>> blockmap,
            std::shared_ptr<MethodCFG> m) {
         std::shared_ptr<BasicBlock> root = m->first_block();
         // Run the iterative dominator solver as suggested in slides
         // ID "0" is the root block (first block)
         std::map<std::string, std::set<std::string>> dom;
         // Set of all labels
         std::set<std::string> N;
         for (const auto& kv : blockmap) {
            N.insert(kv.first);
         }
         for (const auto& kv : blockmap) {
            dom[kv.first] = N;
         }
         // Root "0" has {"0"}
         dom[root->label()] = std::set<std::string>({ root->label() });
         bool changed = true;
         while (changed) {
            changed = false;
            for (const auto& kv : blockmap) {
               if (kv.first != root->label()) {
                  // Default temp to N so that the first intersection passes
                  std::set<std::string> temp = N;
                  for (const auto& pred : kv.second->predecessors()) {
                     std::set<std::string> domj = dom[pred.lock()->label()];
                     std::set<std::string> intersect;
                     std::set_intersection(temp.begin(), temp.end(), domj.begin(), domj.end(),
                           std::inserter(intersect, intersect.begin()));
                     temp = intersect;
                  }
                  // Union in the label itself
                  temp.insert(kv.first);
                  if (temp != dom[kv.first]) {
                     dom[kv.first] = temp;
                     changed = true;
                  }
               }
            }
         }
         return dom;
      }
      // Generate IDom(n) for a given method
      std::map<std::string, std::string> solveIDom(
            std::map<std::string, std::set<std::string>> dom,
            std::shared_ptr<MethodCFG> m) {
         std::map<std::string, std::string> idom;
         std::shared_ptr<BasicBlock> root = m->first_block();
         for (const auto& kv : dom) {
            std::string b = kv.first;
            // Root block has no idom
            if (b != root->label()) {
               std::set<std::string> dominators = kv.second;
               dominators.erase(b);
               // Find the IDom
               // If n = IDom(b), then Dom(b) - b = Dom(n)
               for (const auto& kv2 : dom) {
                  std::string n = kv2.first;
                  // Don't compare block to itself
                  if (b != n && dominators == kv2.second) {
                     // Found IDom
                     idom[b] = n;
                     break;
                  }
               }
            }
         }
         return idom;
      }
      // Generate DF (Dominance Frontiers)
      std::map<std::string, std::set<std::string>> solveDF(
            std::map<std::string, std::string> idom,
            std::map<std::string, std::shared_ptr<BasicBlock>> blockmap) {
         std::map<std::string, std::set<std::string>> DF;
         // Initialize all to empty set
         for (const auto& kv : blockmap) {
            DF[kv.first] = std::set<std::string>({});
         }
         for (const auto& kv : blockmap) {
            std::string n = kv.first;
            std::shared_ptr<BasicBlock> block = kv.second;
            if (block->predecessors().size() > 1) {
               for (const auto& p : block->predecessors()) {
                  std::string runner = p.lock()->label();
                  while (runner != idom[n]) {
                     DF[runner].insert(n);
                     runner = idom[runner];
                  }
               }
            }
         }
         return DF;
      }

      std::map<std::string, std::set<std::string>> solveDF(std::shared_ptr<MethodCFG> method) {
         std::map<std::string, std::shared_ptr<BasicBlock>> blockmap = solveBlockmap(method);
         std::map<std::string, std::set<std::string>> dom = solveDom(blockmap, method);
         std::map<std::string, std::string> idom = solveIDom(dom, method);
         std::map<std::string, std::set<std::string>> df = solveDF(idom, blockmap);
         return df;
      }

};

#endif

