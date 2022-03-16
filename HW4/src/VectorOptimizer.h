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
      size_t _vector_counter;
      std::shared_ptr<BasicBlock> SLP_extract(std::shared_ptr<BasicBlock> B) {
         PackSet_t P;
         P = find_adj_refs(B, P); 
         P = extend_packlist(B, P);
         P = combine_packs(P);
         return schedule(B, _new_block, P);
      }
      bool isomorphic(std::shared_ptr<PrimitiveStatement> s1, std::shared_ptr<PrimitiveStatement> s2) {
         // Both must be GetElt or Arithmetic
         GetEltPrimitive* ge1 = dynamic_cast<GetEltPrimitive*>(s1.get());
         GetEltPrimitive* ge2 = dynamic_cast<GetEltPrimitive*>(s2.get());
         if (ge1 != nullptr && ge2 != nullptr) {
            return true;
         }
         SetEltPrimitive* se1 = dynamic_cast<SetEltPrimitive*>(s1.get());
         SetEltPrimitive* se2 = dynamic_cast<SetEltPrimitive*>(s2.get());
         if (se1 != nullptr && se2 != nullptr) {
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
         std::vector<std::string> lhs1 = s1->LHS();
         std::vector<std::string> lhs2 = s2->LHS();
         std::vector<std::string> rhs1 = s1->RHS();
         std::vector<std::string> rhs2 = s2->RHS();
         for (const auto & r : rhs1) {
            for (const auto & l : lhs2) {
               if (l == r) {
                  return false;
               }
            }
         }
         for (const auto & r : rhs2) {
            for (const auto & l : lhs1) {
               if (l == r) {
                  return false;
               }
            }
         }
         return true;
      }
      bool adjacent(std::shared_ptr<PrimitiveStatement> s1, std::shared_ptr<PrimitiveStatement> s2) {
         std::string i1, i2, arr1, arr2;
         GetEltPrimitive* a1 = dynamic_cast<GetEltPrimitive*>(s1.get());
         GetEltPrimitive* a2 = dynamic_cast<GetEltPrimitive*>(s2.get());
         SetEltPrimitive* b1 = dynamic_cast<SetEltPrimitive*>(s1.get());
         SetEltPrimitive* b2 = dynamic_cast<SetEltPrimitive*>(s2.get());
         if (a1 != nullptr && a2 != nullptr) {
            i1 = a1->index();
            i2 = a2->index();
            arr1 = a1->arr();
            arr2 = a2->arr();
         } else if (b1 != nullptr && b2 != nullptr) {
            i1 = b1->index();
            i2 = b2->index();
            arr1 = b1->arr();
            arr2 = b2->arr();
         } else {
            return false;
         }
         // Array pointed to by both must be same
         if (arr1 != arr2) {
            return false;
         }
         // Verify indices are off by one
         for (char const &ch : i1) {
            if (std::isdigit(ch) == 0) return false;
         }
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
         std::vector<std::string> x1 = s1->RHS();
         // Get s2 args
         std::vector<std::string> x2 = s2->RHS();
         unsigned long m = x1.size();
         std::vector<std::shared_ptr<PrimitiveStatement>> primitives = B->primitives();
         for (unsigned long j=0; j<m; j++) {
            bool ts_exist = false;
            for (const auto & t1 : primitives) {
               for (const auto & t2 : primitives) {
                  if (t1 != t2) {
                     std::vector<std::string> t1_lhs = t1->LHS();
                     std::vector<std::string> t2_lhs = t2->LHS();
                     if (t1_lhs.size() == 1 && t2_lhs.size() == 1) {
                        if (t1_lhs[0] == x1[j] && t2_lhs[0] == x2[j]) {
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
         std::vector<std::string> x1 = s1->LHS();
         // Get s2 lhs
         std::vector<std::string> x2 = s2->LHS();
         if (x1.size() != 1 || x2.size() != 1) {
            return P;
         }
         int savings = -1;
         std::shared_ptr<PrimitiveStatement> u1;
         std::shared_ptr<PrimitiveStatement> u2;
         std::vector<std::shared_ptr<PrimitiveStatement>> primitives = B->primitives();
         for (const auto & t1 : primitives) {
            std::vector<std::string> t1_rhs = t1->RHS();
            if (std::find(t1_rhs.begin(), t1_rhs.end(), x1[0]) != t1_rhs.end()) {
               for (const auto & t2 : primitives) {
                  std::vector<std::string> t2_rhs = t2->RHS();
                  if (t1 != t2 && std::find(t2_rhs.begin(), t2_rhs.end(), x2[0]) != t2_rhs.end()) {
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
                     return combine_packs(Pnext);
                  }
               }
            }
            P = Pnext;
         } while(P != Pprev);
         return P;
      }
      bool deps_scheduled(std::shared_ptr<PrimitiveStatement> s, std::shared_ptr<BasicBlock> B) {
         if (dynamic_cast<PhiPrimitive*>(s.get()) != nullptr) {
            // Phi primitives may cause cycles, but should always be at top of block - do not reschedule
            return true;
         }
         // Check that all deps of s (RHS) are set in B
         std::vector<std::string> deps = s->RHS();
         for (const auto & dep : deps) {
            if (isRegister(dep)) {
               // Check if already scheduled
               if (_scheduled.find(dep) != _scheduled.end()) {
                  continue;
               }
               // Check that dep is a method parameter, if so it does not have a set line
               std::vector<std::string> params = _new_method->first_block()->params();
               bool is_scheduled = false;
               for (const auto & p : params) {
                  if (p == dep) {
                     _scheduled.insert(dep);
                     is_scheduled = true;
                     break;
                  }
               }
               if (is_scheduled) {
                  continue;
               }
               return false;
            }
         }
         return true;
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
      void schedule_vector(std::vector<std::string> v1_args, std::vector<std::string> v2_args, std::vector<std::string> lhs_args, char op, std::shared_ptr<BasicBlock> B2) {
         std::string VEC1 = std::string("%") + std::string(VECTOR) + std::to_string(_vector_counter++);
         B2->appendPrimitive(std::make_shared<LoadVectorPrimitive>(VEC1, v1_args));
         std::string VEC2 = std::string("%") + std::string(VECTOR) + std::to_string(_vector_counter++);
         B2->appendPrimitive(std::make_shared<LoadVectorPrimitive>(VEC2, v2_args));
         std::string DEST_VEC = std::string("%") + std::string(VECTOR) + std::to_string(_vector_counter++);
         if (op == '+') {
            B2->appendPrimitive(std::make_shared<AddVectorPrimitive>(DEST_VEC, VEC1, VEC2));
         } else if (op == '-') {
            B2->appendPrimitive(std::make_shared<SubtractVectorPrimitive>(DEST_VEC, VEC1, VEC2));
         } else if (op == '*') {
            B2->appendPrimitive(std::make_shared<MultiplyVectorPrimitive>(DEST_VEC, VEC1, VEC2));
         } else if (op == '/') {
            B2->appendPrimitive(std::make_shared<DivideVectorPrimitive>(DEST_VEC, VEC1, VEC2));
         }
         B2->appendPrimitive(std::make_shared<StoreVectorPrimitive>(lhs_args, DEST_VEC));
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
                  ArithmeticPrimitive* atest = dynamic_cast<ArithmeticPrimitive*>(p[0].get());
                  std::vector<char> ops = { '+', '-', '*', '/' };
                  if (atest != nullptr && std::find(ops.begin(), ops.end(), atest->op()) != ops.end()) {
                     // Replace w/ vector equivalent
                     size_t count = 0;
                     std::vector<std::string> v1_args;
                     std::vector<std::string> v2_args;
                     std::vector<std::string> lhs_args;
                     char op = atest->op();
                     for (const auto & s2 : p) {
                        ArithmeticPrimitive* a = dynamic_cast<ArithmeticPrimitive*>(s2.get());
                        v1_args.push_back(a->op1());
                        v2_args.push_back(a->op2());
                        lhs_args.push_back(a->lhs());
                        B1->removePrimitive(s2);
                        std::vector<std::string> s2_lhs = s2->LHS();
                        for (const auto & lhs : s2_lhs) {
                           _scheduled.insert(lhs);
                        }
                        count += 1;
                        count %= UNROLL_SIZE;
                        if (count == 0) {
                           // Flush vectors
                           schedule_vector(v1_args, v2_args, lhs_args, op, B2);
                           v1_args.clear();
                           v2_args.clear();
                           lhs_args.clear();
                        }
                     }
                     // Flush final vectors
                     if (lhs_args.size() > 0) {
                        while (lhs_args.size() < UNROLL_SIZE) {
                           // Pad with zero at end
                           lhs_args.push_back("%0"); // TODO change
                           v1_args.push_back(std::to_string(0));
                           v2_args.push_back(std::to_string(0));
                        }
                        // Final vector
                        schedule_vector(v1_args, v2_args, lhs_args, op, B2);
                     }
                  } else {
                     for (const auto & s2 : p) {
                        B2->appendPrimitive(s2);
                        std::vector<std::string> s2_lhs = s2->LHS();
                        for (const auto & lhs : s2_lhs) {
                           _scheduled.insert(lhs);
                        }
                        B1->removePrimitive(s2);
                     }
                  }
                  return schedule(B1, B2, P);
               }
            } else if (deps_scheduled(s[i], B2)) {
               B2->appendPrimitive(s[i]);
               std::vector<std::string> si_lhs = s[i]->LHS();
               for (const auto & lhs : si_lhs) {
                  _scheduled.insert(lhs);
               }
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
         // Don't copy primitives over just yet
         // Call SLP extract to modify _new_block
         SLP_extract(std::make_shared<BasicBlock>(node));
         // Optimize control
         node.control()->accept(*this);
      }

      void visit(BasicBlock& node) {
         optimizeBlock(node);
         optimizeChildren(node);
      }

      void visit(MethodCFG& node) {
         _vector_counter = 0;
         _scheduled.clear();
         IdentityOptimizer::visit(node);
      }
};

#endif

