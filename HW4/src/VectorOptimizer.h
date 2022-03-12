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
      std::string getLHS(std::shared_ptr<PrimitiveStatement> s) {
         GetEltPrimitive* ge = dynamic_cast<GetEltPrimitive*>(s.get());
         if (ge != nullptr) {
            return ge->lhs();
         }
         ArithmeticPrimitive *a = dynamic_cast<ArithmeticPrimitive*>(s.get());
         if (a != nullptr) {
            return a->lhs();
         }
         return "";
      }
      std::vector<std::string> getRHS(std::shared_ptr<PrimitiveStatement> s) {
         GetEltPrimitive* ge = dynamic_cast<GetEltPrimitive*>(s.get());
         if (ge != nullptr) {
            return { ge->arr(), ge->index() };
         }
         ArithmeticPrimitive *a = dynamic_cast<ArithmeticPrimitive*>(s.get());
         if (a != nullptr) {
            return { a->op1(), a->op2() };
         }
         return {};
      }
      std::shared_ptr<BasicBlock> SLP_extract(std::shared_ptr<BasicBlock> B) {
         PackSet_t P;
         P = find_adj_refs(B, P); 
         P = extend_packlist(B, P);
         P = combine_packs(P);
         for (const auto & p : P) {
            for (const auto & s : p) {
               std::cout << s->toString() << std::endl;
            }
            std::cout << std::endl;
         }
         //return schedule(B, _new_block, P);
         return _new_block;
      }
      bool isomorphic(std::shared_ptr<PrimitiveStatement> s1, std::shared_ptr<PrimitiveStatement> s2) {
         // Both must be GetElt or Arithmetic
         GetEltPrimitive* ge1 = dynamic_cast<GetEltPrimitive*>(s1.get());
         GetEltPrimitive* ge2 = dynamic_cast<GetEltPrimitive*>(s2.get());
         if (ge1 != nullptr && ge2 != nullptr) {
            return true;
         }
         ArithmeticPrimitive *a1 = dynamic_cast<ArithmeticPrimitive*>(s1.get());
         ArithmeticPrimitive *a2 = dynamic_cast<ArithmeticPrimitive*>(s2.get());
         if (a1 != nullptr && a2 != nullptr) {
            return a1->op() == a2->op();
         }
         return false;
      }
      bool independent(std::shared_ptr<PrimitiveStatement> s1, std::shared_ptr<PrimitiveStatement> s2) {
         // s1 must not refer to s2
         // s2 must not refer to s1
         std::string lhs1 = getLHS(s1);
         std::string lhs2 = getLHS(s2);
         std::vector<std::string> rhs1 = getRHS(s1);
         std::vector<std::string> rhs2 = getRHS(s2);
         for (const auto & r : rhs1) {
            if (lhs2 == r) {
               return false;
            }
         }
         for (const auto & r : rhs2) {
            if (lhs1 == r) {
               return false;
            }
         }
         return true;
      }
      bool adjacent(std::shared_ptr<PrimitiveStatement> s1, std::shared_ptr<PrimitiveStatement> s2) {
         GetEltPrimitive* a1 = dynamic_cast<GetEltPrimitive*>(s1.get());
         GetEltPrimitive* a2 = dynamic_cast<GetEltPrimitive*>(s2.get());
         if (a1 == nullptr || a2 == nullptr) {
            return false;
         }
         // Array pointed to by both must be same
         if (a1->arr() != a2->arr()) {
            return false;
         }
         // Verify indices are off by one
         std::string i1 = a1->index();
         for (char const &ch : i1) {
            if (std::isdigit(ch) == 0) return false;
         }
         std::string i2 = a2->index();
         for (char const &ch : i2) {
            if (std::isdigit(ch) == 0) return false;
         }
         return (std::stoi(i2) - std::stoi(i1) == 1);
      }
      bool stmts_can_pack(std::shared_ptr<BasicBlock> B, PackSet_t P, std::shared_ptr<PrimitiveStatement> s1, std::shared_ptr<PrimitiveStatement> s2) {
         if (isomorphic(s1, s2)) {
            if (independent(s1, s2)) {
               bool allt1 = true;
               for (const auto & p : P) {
                  std::shared_ptr<PrimitiveStatement> t = p[0];
                  if (t == s1) {
                     allt1 = false;
                     break;
                  }
               }
               if (allt1) {
                  bool allt2 = true;
                  for (const auto & p : P) {
                     std::shared_ptr<PrimitiveStatement> t = p[1];
                     if (t == s2) {
                        allt2 = false;
                        break;
                     }
                  }
                  if (allt2) {
                     // Assume alignment works
                     return true;
                  }
               }
            }
         }
         return false;
      }
      PackSet_t find_adj_refs(std::shared_ptr<BasicBlock> B, PackSet_t P) {
         for (const auto & s1 : B->primitives()) {
            for (const auto & s2 : B->primitives()) {
               if (s1 != s2) {
                  if (adjacent(s1, s2)) {
                     if (stmts_can_pack(B, P, s1, s2)) {
                        // Avoid inserting same pack twice
                        if (P.find(Pack_t({ s2, s1 })) == P.end()) {
                           P.insert(Pack_t({ s1, s2 }));
                        }
                     }
                  }
               }
            }
         }
         return P;
      }
      PackSet_t follow_use_defs(std::shared_ptr<BasicBlock> B, PackSet_t P, Pack_t p) {
         std::shared_ptr<PrimitiveStatement> s1 = p[0];
         std::shared_ptr<PrimitiveStatement> s2 = p[1];
         // Get s1 args
         std::vector<std::string> x1 = getRHS(s1);
         // Get s2 args
         std::vector<std::string> x2 = getRHS(s2);
         unsigned long m = x1.size();
         std::vector<std::shared_ptr<PrimitiveStatement>> primitives = B->primitives();
         for (unsigned long j=0; j<m; j++) {
            bool ts_exist = false;
            for (const auto & t1 : primitives) {
               for (const auto & t2 : primitives) {
                  if (t1 != t2) {
                     std::string t1_lhs = getLHS(t1);
                     std::string t2_lhs = getLHS(t2);
                     if (t1_lhs == x1[j] && t2_lhs == x2[j]) {
                        if (stmts_can_pack(B, P, t1, t2)) {
                           // Avoid inserting same pack twice
                           if (P.find(Pack_t({ t2, t1 })) == P.end()) {
                              P.insert(Pack_t({ t1, t2 }));
                           }
                           ts_exist = true;
                           break;
                        }
                     }
                  }
               }
               if (ts_exist) {
                  break;
               }
            }
         }
         return P;
      }
      PackSet_t follow_def_uses(std::shared_ptr<BasicBlock> B, PackSet_t P, Pack_t p) {
         std::shared_ptr<PrimitiveStatement> s1 = p[0];
         std::shared_ptr<PrimitiveStatement> s2 = p[1];
         // Get s1 lhs
         std::string x1 = getLHS(s1);
         // Get s2 lhs
         std::string x2 = getLHS(s2);
         int savings = -1;
         std::shared_ptr<PrimitiveStatement> u1;
         std::shared_ptr<PrimitiveStatement> u2;
         std::vector<std::shared_ptr<PrimitiveStatement>> primitives = B->primitives();
         for (const auto & t1 : primitives) {
            std::vector<std::string> t1_rhs = getRHS(t1);
            if (std::find(t1_rhs.begin(), t1_rhs.end(), x1) != t1_rhs.end()) {
               for (const auto & t2 : primitives) {
                  std::vector<std::string> t2_rhs = getRHS(t2);
                  if (t1 != t2 && std::find(t2_rhs.begin(), t2_rhs.end(), x2) != t2_rhs.end()) {
                     if (stmts_can_pack(B, P, t1, t2)) {
                        // Assume we save
                        savings = 1;
                        u1 = t1;
                        u2 = t2;
                     }
                  }
               }
            }
         }
         if (savings >= 0) {
            // Avoid inserting same pack twice
            if (P.find(Pack_t({ u2, u1 })) == P.end()) {
               P.insert(Pack_t({ u1, u2 }));
            }
         }
         return P;
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
      void optimizeBlock(BasicBlock& node) {
         std::string label = node.label();
         if (!_label_to_block.count(label)) {
            _label_to_block[label] = std::make_shared<BasicBlock>(label, node.params());
         }
         _new_block = _label_to_block[label];
         // Optimize each primitive
         // Call SLP extract
         SLP_extract(std::make_shared<BasicBlock>(node));
         // Optimize control
         node.control()->accept(*this);
      }

      void visit(BasicBlock& node) {
         optimizeBlock(node);
         optimizeChildren(node);
      }
};

#endif

