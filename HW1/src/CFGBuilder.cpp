#include "AST.h"
#include "CFG.h"
#include "CFGBuilder.h"

void CFGBuilder::visit(UInt32Literal& node) {
   // Don't care about input value
   _input_values.pop();
   // Send (tagged) int back as value
   _return_values.push(std::make_pair(std::to_string((node.val() << 1) + 1), INTEGER));
}

void CFGBuilder::visit(VariableIdentifier& node) {
   // Don't care about input value
   _input_values.pop();
   // Send variable back as named register
   _return_values.push(std::make_pair(toRegister(node.name()), UNKNOWN));
}

void CFGBuilder::visit(ArithmeticExpression& node) {
   // Visit e1, e2
   _input_values.push(TEMP);
   node.e1()->accept(*this);
   std::pair<std::string, ReturnType> p1 = _return_values.top();
   std::string r1 = p1.first;
   ReturnType rt1 = p1.second;
   _return_values.pop();
   _input_values.push(TEMP);
   node.e2()->accept(*this);
   std::pair<std::string, ReturnType> p2 = _return_values.top();
   std::string r2 = p2.first;
   ReturnType rt2 = p2.second;
   _return_values.pop();
   // Tagged integer check on each operand
   // Only need to check if it's not an int
   if (rt1 != INTEGER) {
      tagCheck(r1, true, BADNUMBER, NOT_A_NUMBER);
   }
   if (rt2 != INTEGER) {
      tagCheck(r2, true, BADNUMBER, NOT_A_NUMBER);
   }
   // Derive operation
   char op = node.op();
   std::string ret;
   if (op == '+') {
      // Mask off right operand's tag bit
      std::string temp = createTemp();
      // The value chosen is 64-bit max minus 1
      _curr_block.top()->appendPrimitive(std::make_shared<ArithmeticPrimitive>(temp, r2, '&', "18446744073709551614"));
      ret = setReturnName(INTEGER);
      // Do the addition
      _curr_block.top()->appendPrimitive(std::make_shared<ArithmeticPrimitive>(ret, r1, '+', temp));
   } else if (op == '-') {
      // Subtract first
      std::string temp = createTemp();
      // The value will be correct but missing tag
      _curr_block.top()->appendPrimitive(std::make_shared<ArithmeticPrimitive>(temp, r1, '-', r2));
      // Add back tag bit
      ret = setReturnName(INTEGER);
      _curr_block.top()->appendPrimitive(std::make_shared<ArithmeticPrimitive>(ret, temp, '+', "1"));
   } else {
      // Assume mult or div
      // Shift left operand right one bit (divide by 2)
      std::string temp1 = createTemp();
      _curr_block.top()->appendPrimitive(std::make_shared<ArithmeticPrimitive>(temp1, r1, '/', "2"));
      // Shift right operand right one bit (divide by 2)
      std::string temp2 = createTemp();
      _curr_block.top()->appendPrimitive(std::make_shared<ArithmeticPrimitive>(temp2, r2, '/', "2"));
      // Do the operation
      std::string temp3 = createTemp();
      _curr_block.top()->appendPrimitive(std::make_shared<ArithmeticPrimitive>(temp3, temp1, op, temp2));
      // Shift back (multiply by 2)
      std::string temp4 = createTemp();
      _curr_block.top()->appendPrimitive(std::make_shared<ArithmeticPrimitive>(temp4, temp3, '*', "2"));
      // Re-tag
      ret = setReturnName(INTEGER);
      _curr_block.top()->appendPrimitive(std::make_shared<ArithmeticPrimitive>(ret, temp4, '+', "1"));
   }
}

void CFGBuilder::visit(CallExpression& node) {
   // Evalauate arguments first
   std::vector<std::shared_ptr<ASTExpression>> params = node.params();
   std::vector<std::string> regParams;
   for (auto & p : params) {
      _input_values.push(TEMP);
      p->accept(*this);
      regParams.push_back(_return_values.top().first);
      _return_values.pop();
   }
   // Visit obj
   _input_values.push(TEMP);
   node.obj()->accept(*this);
   std::string receiver = _return_values.top().first;
   ReturnType receivertype = _return_values.top().second;
   _return_values.pop();
   // Tag check obj is pointer
   if (receivertype != POINTER) {
      tagCheck(receiver, false, BADPOINTER, NOT_A_POINTER);
   }
   // Load vtable
   std::string vtable = createTemp();
   _curr_block.top()->appendPrimitive(std::make_shared<LoadPrimitive>(vtable, receiver));
   // Lookup method ID corresponding to method string
   std::string methodAddr = createTemp();
   std::string method = node.method();
   std::string index = std::to_string(_method_to_vtable_offset[method]);
   _curr_block.top()->appendPrimitive(std::make_shared<GetEltPrimitive>(methodAddr, vtable, index));
   // Verify that method exists
   nonzeroCheck(methodAddr, BADMETHOD, NO_SUCH_METHOD);
   // Call and return
   std::string ret = setReturnName();
   _curr_block.top()->appendPrimitive(std::make_shared<CallPrimitive>(ret, methodAddr, receiver, regParams));
}

void CFGBuilder::visit(FieldReadExpression& node) {
   // Visit obj
   _input_values.push(TEMP);
   node.obj()->accept(*this);
   std::string baseaddr = _return_values.top().first;
   ReturnType basetype = _return_values.top().second;
   _return_values.pop();
   // Tag check obj is pointer
   if (basetype != POINTER) {
      tagCheck(baseaddr, false, BADPOINTER, NOT_A_POINTER);
   }
   // Get field map addr
   std::string fieldmapaddr = createTemp();
   _curr_block.top()->appendPrimitive(std::make_shared<ArithmeticPrimitive>(fieldmapaddr, baseaddr, '+', "8"));
   // Load field map, map is guaranteed pointer
   std::string fieldmap = createTemp();
   _curr_block.top()->appendPrimitive(std::make_shared<LoadPrimitive>(fieldmap, fieldmapaddr));
   // Lookup field offset corresponding to field string
   std::string fieldOffset = createTemp();
   std::string field = node.field();
   std::string index = std::to_string(_field_to_map_offset[field]);
   _curr_block.top()->appendPrimitive(std::make_shared<GetEltPrimitive>(fieldOffset, fieldmap, index));
   // Verify that field exists
   nonzeroCheck(fieldOffset, BADFIELD, NO_SUCH_FIELD);
   // Get and return
   std::string ret = setReturnName();
   _curr_block.top()->appendPrimitive(std::make_shared<GetEltPrimitive>(ret, baseaddr, fieldOffset));
}

void CFGBuilder::visit(NewObjectExpression& node) {
   // Get the return value early so we can alloc at it
   std::string ret = setReturnName(POINTER);
   // Get size to allocate
   std::string classname = node.class_name();
   std::string allocSize = std::to_string(_class_name_to_alloc_size[classname]);
   // Allocate the class
   _curr_block.top()->appendPrimitive(std::make_shared<AllocPrimitive>(ret, allocSize));
   // No tag checks necessary ahead, we know ret is a POINTER
   // Store the vtbl
   _curr_block.top()->appendPrimitive(std::make_shared<StorePrimitive>(ret, toGlobal(toVtable(classname))));
   // Increment pointer
   std::string fieldMapAddr = createTemp();
   _curr_block.top()->appendPrimitive(std::make_shared<ArithmeticPrimitive>(fieldMapAddr, ret, '+', "8"));
   // Store the field map
   _curr_block.top()->appendPrimitive(std::make_shared<StorePrimitive>(fieldMapAddr, toGlobal(toFieldMap(classname))));
}

void CFGBuilder::visit(ThisObjectExpression& node) {
   // Don't care about input value
   _input_values.pop();
   // Send variable back as "this" register
   // Technically should always hold pointer, but no guarantees
   _return_values.push(std::make_pair(toRegister("this"), UNKNOWN));
}

void CFGBuilder::visit(AssignmentStatement& node) {
   std::string destRegister = toRegister(node.variable());
   // Set destRegister as input value
   _input_values.push(destRegister);
   // Visit expression
   node.val()->accept(*this);
   // Get return value
   // This should either be the destRegister pushed in, if it was satisfied by the expression
   // If it is not the destRegister, then we have some statement in the form "x = y"
   // where "y" is either an integer, a different variable, or "this"
   std::string retValue = _return_values.top().first;
   _return_values.pop();
   if (retValue != destRegister) {
      _curr_block.top()->appendPrimitive(std::make_shared<AssignmentPrimitive>(destRegister, retValue));
   }
}

void CFGBuilder::visit(DontCareAssignmentStatement& node) {
   // This is like an assignment statement but we're not pushing a dest register
   // And don't need to make an assignment primitive either
   // Still need a dest though for most statements, so push TEMP up
   _input_values.push(TEMP);
   node.val()->accept(*this);
   // Don't care about return value
   _return_values.pop();
}

void CFGBuilder::visit(FieldUpdateStatement& node) {
}

void CFGBuilder::visit(IfElseStatement& node) {
}

void CFGBuilder::visit(IfOnlyStatement& node) {
}

void CFGBuilder::visit(WhileStatement& node) {
}

void CFGBuilder::visit(ReturnStatement& node) {
   // Visit val, get register to return
   _input_values.push(TEMP);
   node.val()->accept(*this);
   std::string retValue = _return_values.top().first;
   _return_values.pop();
   // Update current block with return control
   _curr_block.top()->setControl(std::make_shared<RetControl>(retValue));
   // Pop off curr_block (nothing left to do on current CFG)
   _curr_block.pop();
}

void CFGBuilder::visit(PrintStatement& node) {
   // Visit val, get register to print
   _input_values.push(TEMP);
   node.val()->accept(*this);
   std::string retValue = _return_values.top().first;
   _return_values.pop();
   // Add print primitive
   _curr_block.top()->appendPrimitive(std::make_shared<PrintPrimitive>(retValue));
}

void CFGBuilder::visit(MethodDeclaration& node) {
}

void CFGBuilder::visit(ClassDeclaration& node) {
}

void CFGBuilder::visit(ProgramDeclaration& node) {
}

