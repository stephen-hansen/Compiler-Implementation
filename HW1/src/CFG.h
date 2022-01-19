#ifndef _CS_441_CFG_H
#define _CS_441_CFG_H
#include <memory>
#include <string>
#include <vector>

class IRStatement
{
   public:
      virtual std::string toString() = 0;
};

class PrimitiveStatement : public IRStatement
{};

class ControlStatement : public IRStatement
{};

class BasicBlock
{
   private:
      std::vector<std::shared_ptr<PrimitiveStatement>> _primitives;
      std::shared_ptr<ControlStatement> _control;
      std::shared_ptr<BasicBlock> _parent;
      std::vector<std::shared_ptr<BasicBlock>> _children;
   public:
      BasicBlock(): _parent(nullptr) {}
      void appendPrimitive(std::shared_ptr<PrimitiveStatement> ps) {
         _primitives.push_back(ps);
      }
      void setControl(std::shared_ptr<ControlStatement> c) {
         _control = c;
      }
      void addChild(std::shared_ptr<BasicBlock> b) {
         _children.push_back(b);
         b->_parent = std::shared_ptr<BasicBlock>(this);
      }
};

#endif
