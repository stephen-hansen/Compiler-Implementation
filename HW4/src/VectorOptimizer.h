#ifndef _CS_441_VECTOR_OPTIMIZER_H
#define _CS_441_VECTOR_OPTIMIZER_H
#include <map>
#include <set>
#include <stack>
#include <vector>
#include "CFG.h"
#include "IdentityOptimizer.h"

using Pack_t = std::vector<std::shared_ptr<PrimitiveStatement>>;
using PackSet_t = std::set<Pack_t>;

class VectorOptimizer : public IdentityOptimizer
{
   private:
      std::set<std::string> _scheduled;
      std::shared_ptr<BasicBlock> SLP_extract(std::shared_ptr<BasicBlock> B) {
         PackSet_t P;
         P = find_adj_refs(B, P);
         P = extend_packlist(B, P);
         P = combine_packs(P);
         return schedule(B, _new_block, P);
      }
      bool isomorphic(std::shared_ptr<PrimitiveStatement> s1, std::shared_ptr<PrimitiveStatement> s2) {
         // TODO return true if s1 and s2 are same statement type
      }
      bool independent(std::shared_ptr<PrimitiveStatement> s1, std::shared_ptr<PrimitiveStatement> s2) {
         // TODO return true if s1 and s2 do not depend on each other
      }
      bool stmts_can_pack(std::shared_ptr<BasicBlock> B, PackSet_t P, std::shared_ptr<PrimitiveStatement> s1, std::shared_ptr<PrimitiveStatement> s2) {
      }
      PackSet_t find_adj_refs(std::shared_ptr<BasicBlock> B, PackSet_t P) {
         for (const auto & s1 : B->primitives()) {
            for (const auto & s2 : B->primitives()) {
               if (s1 != s2) {
                  if (stmts_can_pack(B, P, s1, s2)) {
                     P.insert(Pack_t({ s1, s2 }));
                  }
               }
            }
         }
         return P;
      }
      PackSet_t follow_use_defs(std::shared_ptr<BasicBlock> B, PackSet_t P, Pack_t p) {
      }
      PackSet_t follow_def_uses(std::shared_ptr<BasicBlock> B, PackSet_t P, Pack_t p) {
      }
      PackSet_t extend_packlist(std::shared_ptr<BasicBlock> B, PackSet_t P) {
         PackSet_t Pprev, Pnext;
         do {
            Pprev = P;
            Pnext = P;
            for (const auto & p : P) {
               Pnext = follow_use_defs(B, Pnext, p);
               Pnext = follow_def_uses(B, Pnext, p);
            }
            P = Pnext;
         } while (P != Pprev);
         return P;
      }
      PackSet_t combine_packs(PackSet_t P) {
         PackSet_t Pprev, Pnext;
         do {
            Pprev = P;
            Pnext = P;
            for (const auto & p1 : P) {
               for (const auto & p2 : P) {
                  std::shared_ptr<IRStatement> sn = p1[p1.size()-1];
                  std::shared_ptr<IRStatement> s1 = p2[0];
                  if (sn == s1) {
                     Pack_t newPack;
                     newPack.reserve(p1.size()+p2.size()-1);
                     newPack.insert(newPack.end(), p1.begin(), p1.end());
                     newPack.insert(newPack.end(), p2.begin()+1, p2.end());
                     Pnext.erase(p1);
                     Pnext.erase(p2);
                     Pnext.insert(newPack);
                  }
               }
            }
            P = Pnext;
         } while(P != Pprev);
         return P;
      }
      bool deps_scheduled(std::shared_ptr<PrimitiveStatement> s, std::shared_ptr<BasicBlock> B) {
         // TODO type check find deps
      }
      Pack_t first(std::shared_ptr<BasicBlock> B, PackSet_t P) {
         std::map<std::shared_ptr<PrimitiveStatement>, unsigned long> stmt_to_offset;
         unsigned long i = 0;
         for (const auto & s : B->primitives()) {
            stmt_to_offset[s] = i++;
         }
         unsigned long earliest = stmt_to_offset.size();
         Pack_t earliest_pack;
         for (const auto & p : P) {
            for (const auto & s : p) {
               if (stmt_to_offset.find(s) != stmt_to_offset.end()) {
                  if (stmt_to_offset[s] < earliest) {
                     earliest = stmt_to_offset[s];
                     earliest_pack = p;
                  }
               }
            }
         }
         return earliest_pack;
      }
      std::shared_ptr<BasicBlock> schedule(std::shared_ptr<BasicBlock> B1, std::shared_ptr<BasicBlock> B2, PackSet_t P) {
         std::vector<std::shared_ptr<PrimitiveStatement>> s = B1->primitives();
         for (unsigned long i=0; i < s.size(); i++) {
            bool p_exists = false;
            Pack_t p;
            for (const auto & p_cand : P) {
               if (std::find(p_cand.begin(), p_cand.end(), s[i]) != p_cand.end()) {
                  p = p_cand;
                  p_exists = true;
                  break;
               }
            }
            if (p_exists) {
               bool all_deps_scheduled = true;
               for (const auto & s2 : p) {
                  if (!deps_scheduled(s2, B2)) {
                     all_deps_scheduled = false;
                     break;
                  }
               }
               if (all_deps_scheduled) {
                  for (const auto & s2 : p) {
                     B2->appendPrimitive(s2);
                     B1->removePrimitive(s2);
                     return schedule(B1, B2, P);
                  }
               }
            } else if (deps_scheduled(s[i], B2)) {
              B2->appendPrimitive(s[i]);
              B1->removePrimitive(s[i]);
              return schedule(B1, B2, P);
            }
         }
         if (s.size() != 0) {
            Pack_t p = first(B1, P);
            P.erase(p);
            return schedule(B1, B2, P);
         }
         return B2;
      }
   public:
      void visit(AssignmentPrimitive& node) {
         // TODO
      }
      void visit(ArithmeticPrimitive& node) {
         // TODO
      }
};

#endif

