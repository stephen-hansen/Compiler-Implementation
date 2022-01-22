#include "AST.h"
#include "CFG.h"
#include "CFGBuilder.h"

void CFGBuilder::visit(UInt32Literal& node) {
   // Don't care about input value
   _input_values.pop();
   // Send (tagged) int back as value
   _return_values.push(std::make_pair(std::to_string(tag(node.val())), INTEGER));
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
   _curr_block->appendPrimitive(std::make_shared<Comment>(node.toSourceString()));
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
      _curr_block->appendPrimitive(std::make_shared<ArithmeticPrimitive>(temp, r2, '&', "18446744073709551614"));
      ret = setReturnName(INTEGER);
      // Do the addition
      _curr_block->appendPrimitive(std::make_shared<ArithmeticPrimitive>(ret, r1, '+', temp));
   } else if (op == '-') {
      // Subtract first
      std::string temp = createTemp();
      // The value will be correct but missing tag
      _curr_block->appendPrimitive(std::make_shared<ArithmeticPrimitive>(temp, r1, '-', r2));
      // Add back tag bit
      ret = setReturnName(INTEGER);
      _curr_block->appendPrimitive(std::make_shared<ArithmeticPrimitive>(ret, temp, '+', "1"));
   } else {
      // Assume mult or div
      // Shift left operand right one bit (divide by 2)
      std::string temp1 = createTemp();
      _curr_block->appendPrimitive(std::make_shared<ArithmeticPrimitive>(temp1, r1, '/', "2"));
      // Shift right operand right one bit (divide by 2)
      std::string temp2 = createTemp();
      _curr_block->appendPrimitive(std::make_shared<ArithmeticPrimitive>(temp2, r2, '/', "2"));
      // Do the operation
      std::string temp3 = createTemp();
      _curr_block->appendPrimitive(std::make_shared<ArithmeticPrimitive>(temp3, temp1, op, temp2));
      // Shift back (multiply by 2)
      std::string temp4 = createTemp();
      _curr_block->appendPrimitive(std::make_shared<ArithmeticPrimitive>(temp4, temp3, '*', "2"));
      // Re-tag
      ret = setReturnName(INTEGER);
      _curr_block->appendPrimitive(std::make_shared<ArithmeticPrimitive>(ret, temp4, '+', "1"));
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
   _curr_block->appendPrimitive(std::make_shared<Comment>(node.toSourceString()));
   // Tag check obj is pointer
   if (receivertype != POINTER) {
      tagCheck(receiver, false, BADPOINTER, NOT_A_POINTER);
   }
   // Load vtable
   std::string vtable = createTemp();
   _curr_block->appendPrimitive(std::make_shared<LoadPrimitive>(vtable, receiver));
   // Lookup method ID corresponding to method string
   std::string methodAddr = createTemp();
   std::string method = node.method();
   std::string index = std::to_string(_method_to_vtable_offset[method]);
   _curr_block->appendPrimitive(std::make_shared<GetEltPrimitive>(methodAddr, vtable, index));
   // Verify that method exists
   nonzeroCheck(methodAddr, BADMETHOD, NO_SUCH_METHOD);
   // Call and return
   std::string ret = setReturnName();
   _curr_block->appendPrimitive(std::make_shared<CallPrimitive>(ret, methodAddr, receiver, regParams));
}

void CFGBuilder::visit(FieldReadExpression& node) {
   // Visit obj
   _input_values.push(TEMP);
   node.obj()->accept(*this);
   std::string baseaddr = _return_values.top().first;
   ReturnType basetype = _return_values.top().second;
   _return_values.pop();
   _curr_block->appendPrimitive(std::make_shared<Comment>(node.toSourceString()));
   // Tag check obj is pointer
   if (basetype != POINTER) {
      tagCheck(baseaddr, false, BADPOINTER, NOT_A_POINTER);
   }
   // Get field map addr
   std::string fieldmapaddr = createTemp();
   _curr_block->appendPrimitive(std::make_shared<ArithmeticPrimitive>(fieldmapaddr, baseaddr, '+', "8"));
   // Load field map, map is guaranteed pointer
   std::string fieldmap = createTemp();
   _curr_block->appendPrimitive(std::make_shared<LoadPrimitive>(fieldmap, fieldmapaddr));
   // Lookup field offset corresponding to field string
   std::string fieldOffset = createTemp();
   std::string field = node.field();
   std::string index = std::to_string(_field_to_map_offset[field]);
   _curr_block->appendPrimitive(std::make_shared<GetEltPrimitive>(fieldOffset, fieldmap, index));
   // Verify that field exists
   nonzeroCheck(fieldOffset, BADFIELD, NO_SUCH_FIELD);
   // Get and return
   std::string ret = setReturnName();
   _curr_block->appendPrimitive(std::make_shared<GetEltPrimitive>(ret, baseaddr, fieldOffset));
}

void CFGBuilder::visit(NewObjectExpression& node) {
   _curr_block->appendPrimitive(std::make_shared<Comment>(node.toSourceString()));
   // Get the return value early so we can alloc at it
   std::string ret = setReturnName(POINTER);
   // Get size to allocate
   std::string classname = node.class_name();
   std::string allocSize = std::to_string(_class_name_to_alloc_size[classname]);
   // Allocate the class
   _curr_block->appendPrimitive(std::make_shared<AllocPrimitive>(ret, allocSize));
   // No tag checks necessary ahead, we know ret is a POINTER
   // Store the vtbl
   _curr_block->appendPrimitive(std::make_shared<StorePrimitive>(ret, toGlobal(toVtable(classname))));
   // Increment pointer
   std::string fieldMapAddr = createTemp();
   _curr_block->appendPrimitive(std::make_shared<ArithmeticPrimitive>(fieldMapAddr, ret, '+', "8"));
   // Store the field map
   _curr_block->appendPrimitive(std::make_shared<StorePrimitive>(fieldMapAddr, toGlobal(toFieldMap(classname))));
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
   _curr_block->appendPrimitive(std::make_shared<Comment>(node.toSourceString()));
   node.val()->accept(*this);
   // Get return value
   // This should either be the destRegister pushed in, if it was satisfied by the expression
   // If it is not the destRegister, then we have some statement in the form "x = y"
   // where "y" is either an integer, a different variable, or "this"
   std::string retValue = _return_values.top().first;
   _return_values.pop();
   if (retValue != destRegister) {
      _curr_block->appendPrimitive(std::make_shared<AssignmentPrimitive>(destRegister, retValue));
   }
}

void CFGBuilder::visit(DontCareAssignmentStatement& node) {
   // This is like an assignment statement but we're not pushing a dest register
   // And don't need to make an assignment primitive either
   // Still need a dest though for most statements, so push TEMP up
   _input_values.push(TEMP);
   _curr_block->appendPrimitive(std::make_shared<Comment>(node.toSourceString()));
   node.val()->accept(*this);
   // Don't care about return value
   _return_values.pop();
}

void CFGBuilder::visit(FieldUpdateStatement& node) {
   // Visit obj
   _input_values.push(TEMP);
   node.obj()->accept(*this);
   std::string baseaddr = _return_values.top().first;
   ReturnType basetype = _return_values.top().second;
   _return_values.pop();
   _curr_block->appendPrimitive(std::make_shared<Comment>(node.toSourceString()));
   // Tag check obj is pointer
   if (basetype != POINTER) {
      tagCheck(baseaddr, false, BADPOINTER, NOT_A_POINTER);
   }
   // Get field map addr
   std::string fieldmapaddr = createTemp();
   _curr_block->appendPrimitive(std::make_shared<ArithmeticPrimitive>(fieldmapaddr, baseaddr, '+', "8"));
   // Load field map, map is guaranteed pointer
   std::string fieldmap = createTemp();
   _curr_block->appendPrimitive(std::make_shared<LoadPrimitive>(fieldmap, fieldmapaddr));
   // Lookup field offset corresponding to field string
   std::string fieldOffset = createTemp();
   std::string field = node.field();
   std::string index = std::to_string(_field_to_map_offset[field]);
   _curr_block->appendPrimitive(std::make_shared<GetEltPrimitive>(fieldOffset, fieldmap, index));
   // Verify that field exists
   nonzeroCheck(fieldOffset, BADFIELD, NO_SUCH_FIELD);
   // Visit val
   _input_values.push(TEMP);
   node.val()->accept(*this);
   std::string val = _return_values.top().first;
   _return_values.pop();
   // Call setelt, send result to temporary
   // Field update result is never used in original syntax
   std::string temp = createTemp();
   _curr_block->appendPrimitive(std::make_shared<SetEltPrimitive>(temp, baseaddr, fieldOffset, val));
}

void CFGBuilder::visit(IfElseStatement& node) {
   // Visit cond
   _input_values.push(TEMP);
   node.cond()->accept(*this);
   std::string cond = _return_values.top().first;
   ReturnType rt = _return_values.top().second;
   _return_values.pop();
   if (rt != INTEGER) {
      tagCheck(cond, true, BADNUMBER, NOT_A_NUMBER);
   }
   // Convert cond to untagged
   std::string trueCond = createTemp();
   _curr_block->appendPrimitive(std::make_shared<ArithmeticPrimitive>(trueCond, cond, '/', "2"));
   // Create if true block
   std::string trueLabel = createLabel();
   std::shared_ptr<BasicBlock> true_block = std::make_shared<BasicBlock>(trueLabel);
   // If block owns true block
   _curr_block->addNewChild(true_block);
   // Create if false block
   std::string falseLabel = createLabel();
   std::shared_ptr<BasicBlock> false_block = std::make_shared<BasicBlock>(falseLabel);
   // If block owns false block
   _curr_block->addNewChild(false_block);
   // End current block with if/else
   _curr_block->appendPrimitive(std::make_shared<Comment>(node.toSourceString()));
   _curr_block->setControl(std::make_shared<IfElseControl>(trueCond, trueLabel, falseLabel));
   // Recursively build true block, push to current scope
   _curr_block = true_block;
   std::vector<std::shared_ptr<ASTStatement>> if_statements = node.if_statements();
   for (auto & s : if_statements) {
      if (_curr_block->isUnreachable()) {
         break;
      }
      s->accept(*this);
   }
   std::shared_ptr<BasicBlock> last_true_block = _curr_block;
   // Recursively build false block, push to current scope
   _curr_block = false_block;
   std::vector<std::shared_ptr<ASTStatement>> else_statements = node.else_statements();
   for (auto & s : else_statements) {
      if (_curr_block->isUnreachable()) {
         break;
      }
      s->accept(*this);
   }
   std::shared_ptr<BasicBlock> last_false_block = _curr_block; 
   // Create final block
   std::string finalLabel = createLabel();
   std::shared_ptr<BasicBlock> final_block = std::make_shared<BasicBlock>(finalLabel);
   bool final_block_is_owned = false;
   if (!last_false_block->isUnreachable()) {
      last_false_block->addNewChild(final_block);
      final_block_is_owned = true;
      last_false_block->setControl(std::make_shared<JumpControl>(finalLabel));
   }
   if (!last_true_block->isUnreachable()) {
      if (!final_block_is_owned) {
         last_true_block->addNewChild(final_block);
         final_block_is_owned = true;
      } else {
         last_true_block->addExistingChild(final_block);
      }
      // End current block with jump to final block
      last_true_block->setControl(std::make_shared<JumpControl>(finalLabel));
   }
   // Update current block to final block
   _curr_block = final_block;
   if (!final_block_is_owned) {
      // Both if and else return so anything after is unreachable
      _curr_block->setUnreachable(true);
   }
}

void CFGBuilder::visit(IfOnlyStatement& node) {
   // Visit cond
   _input_values.push(TEMP);
   node.cond()->accept(*this);
   std::string cond = _return_values.top().first;
   ReturnType rt = _return_values.top().second;
   _return_values.pop();
   if (rt != INTEGER) {
      tagCheck(cond, true, BADNUMBER, NOT_A_NUMBER);
   }
   // Convert cond to untagged
   std::string trueCond = createTemp();
   _curr_block->appendPrimitive(std::make_shared<ArithmeticPrimitive>(trueCond, cond, '/', "2"));
   // Create if true block
   std::string trueLabel = createLabel();
   std::shared_ptr<BasicBlock> true_block = std::make_shared<BasicBlock>(trueLabel);
   // If block owns true block
   _curr_block->addNewChild(true_block);
   // Create if false block
   std::string falseLabel = createLabel();
   std::shared_ptr<BasicBlock> false_block = std::make_shared<BasicBlock>(falseLabel);
   // If block owns false block
   _curr_block->addNewChild(false_block);
   // End current block with if/else
   _curr_block->appendPrimitive(std::make_shared<Comment>(node.toSourceString()));
   _curr_block->setControl(std::make_shared<IfElseControl>(trueCond, trueLabel, falseLabel));
   // Recursively build true block, push to current scope
   _curr_block = true_block;
   std::vector<std::shared_ptr<ASTStatement>> statements = node.statements();
   for (auto & s : statements) {
      if (_curr_block->isUnreachable()) {
         break;
      }
      s->accept(*this);
   }
   if (!_curr_block->isUnreachable()) {
      // Update current block to jump to false block. Might be different from true_block.
      _curr_block->setControl(std::make_shared<JumpControl>(falseLabel));
      // Curr block points to false block but does NOT own it
      _curr_block->addExistingChild(false_block);
   }
   // Update current block to false block
   _curr_block = false_block;
}

void CFGBuilder::visit(WhileStatement& node) {
   // End current block by jumping to conditional block
   std::string condLabel = createLabel();
   std::shared_ptr<BasicBlock> cond_block = std::make_shared<BasicBlock>(condLabel);
   // Original block owns cond block
   _curr_block->addNewChild(cond_block);
   _curr_block->setControl(std::make_shared<JumpControl>(condLabel));
   // Visit cond
   _curr_block = cond_block;
   _input_values.push(TEMP);
   node.cond()->accept(*this);
   std::string cond = _return_values.top().first;
   ReturnType rt = _return_values.top().second;
   _return_values.pop();
   if (rt != INTEGER) {
      tagCheck(cond, true, BADNUMBER, NOT_A_NUMBER);
   }
   // Convert cond to untagged
   std::string trueCond = createTemp();
   _curr_block->appendPrimitive(std::make_shared<ArithmeticPrimitive>(trueCond, cond, '/', "2"));
   // Create if true block
   std::string trueLabel = createLabel();
   std::shared_ptr<BasicBlock> true_block = std::make_shared<BasicBlock>(trueLabel);
   // curr block owns true block
   _curr_block->addNewChild(true_block);
   // Create if false block
   std::string falseLabel = createLabel();
   std::shared_ptr<BasicBlock> false_block = std::make_shared<BasicBlock>(falseLabel);
   // curr block owns false block
   _curr_block->addNewChild(false_block);
   // End current block with if/else
   _curr_block->appendPrimitive(std::make_shared<Comment>(node.toSourceString()));
   _curr_block->setControl(std::make_shared<IfElseControl>(trueCond, trueLabel, falseLabel));
   // Recursively build true block, push to current scope
   _curr_block = true_block;
   std::vector<std::shared_ptr<ASTStatement>> statements = node.statements();
   for (auto & s : statements) {
      if (_curr_block->isUnreachable()) {
         break;
      }
      s->accept(*this);
   }
   if (!_curr_block->isUnreachable()) {
      // Update current block to jump to cond block. Might be different from true_block.
      _curr_block->setControl(std::make_shared<JumpControl>(condLabel));
      // Curr block points to cond block but does NOT own it
      _curr_block->addExistingChild(cond_block);
   }
   // Update current block to false block
   _curr_block = false_block;
}

void CFGBuilder::visit(ReturnStatement& node) {
   // Visit val, get register to return
   _input_values.push(TEMP);
   node.val()->accept(*this);
   std::string retValue = _return_values.top().first;
   _return_values.pop();
   // Update current block with return control
   _curr_block->appendPrimitive(std::make_shared<Comment>(node.toSourceString()));
   _curr_block->setControl(std::make_shared<RetControl>(retValue));
   // Create new block so that we don't override control
   std::shared_ptr<BasicBlock> next_block = std::make_shared<BasicBlock>("unreachable");
   // Mark as unreachable
   next_block->setUnreachable(true);
   // No one owns it
   _curr_block = next_block;
}

void CFGBuilder::visit(PrintStatement& node) {
   // Visit val, get register to print
   _input_values.push(TEMP);
   node.val()->accept(*this);
   std::string retValue = _return_values.top().first;
   ReturnType retType = _return_values.top().second;
   // Print should probably do a tag check and de-convert the value by dividing by 2
   _return_values.pop();
   _curr_block->appendPrimitive(std::make_shared<Comment>(node.toSourceString()));
   if (retType != INTEGER) {
      tagCheck(retValue, true, BADNUMBER, NOT_A_NUMBER);  
   }
   std::string temp = createTemp();
   _curr_block->appendPrimitive(std::make_shared<ArithmeticPrimitive>(temp, retValue, '/', "2"));
   // Add print primitive
   _curr_block->appendPrimitive(std::make_shared<PrintPrimitive>(temp));
}

void CFGBuilder::visit(MethodDeclaration& node) {
   // Reset temporary counter
   resetCounter();
   std::string methodName = toMethodName(_curr_class->name(), node.name());
   // Build list of parameters
   std::vector<std::string> params;
   // First param is %this
   params.push_back(toRegister("this"));
   // Convert method params to remaining params
   for (auto & p : node.params()) {
      params.push_back(toRegister(p));
   }
   std::shared_ptr<BasicBlock> entry_block = std::make_shared<BasicBlock>(methodName, params);
   _curr_block = entry_block;
   // Initialize every local to 0 (0 << 1 + 1 => 1 as tagged)
   for (auto & l : node.locals()) {
      _curr_block->appendPrimitive(std::make_shared<AssignmentPrimitive>(toRegister(l), std::to_string(tag(0))));
   }
   // Recursively visit statements and build
   std::vector<std::shared_ptr<ASTStatement>> statements = node.statements();
   for (auto & s : statements) {
      if (_curr_block->isUnreachable()) {
         break;
      }
      s->accept(*this);
   }
   // Append method with ENTRY block (_curr_block will be LAST block here)
   _curr_class->appendMethod(std::make_shared<MethodCFG>(entry_block));
}

void CFGBuilder::visit(ClassDeclaration& node) {
   // Construct the vtable
   std::vector<std::shared_ptr<MethodDeclaration>> methods = node.methods();
   std::vector<std::string> vtable;
   vtable.resize(_method_to_vtable_offset.size());
   std::fill(vtable.begin(), vtable.end(), std::string("0"));
   for (auto & m : methods) {
      int loc = _method_to_vtable_offset[m->name()];
      std::string methodname = toMethodName(node.name(), m->name());
      vtable[loc] = methodname;
   }
   // Construct the fields map
   std::vector<std::string> fields = node.fields();
   std::vector<unsigned long> fieldsMap;
   fieldsMap.resize(_field_to_map_offset.size());
   std::fill(fieldsMap.begin(), fieldsMap.end(), 0);
   int index = 2;
   for (auto & f : fields) {
      int loc = _field_to_map_offset[f];
      unsigned long offset = index++;
      fieldsMap[loc] = offset;
   }
   _curr_class = std::make_shared<ClassCFG>(node.name(), vtable, fieldsMap);
   // Build all methods
   for (auto & m : methods) {
      m->accept(*this);
   }
   // Add class to program
   _curr_program->appendClass(_curr_class);
}

void CFGBuilder::visit(ProgramDeclaration& node) {
   std::vector<std::shared_ptr<ClassDeclaration>> classes = node.classes();
   // Location in field map
   int field_offset = 0;
   // Location in vtable
   int method_offset = 0;
   // Class maps must be built first to understand offsets for fields, methods
   for (auto & c : classes) {
      // Build map of class names to allocation sizes
      // Size is always 2 + number of fields
      _class_name_to_alloc_size[c->name()] = 2 + c->fields().size();
      // Build field map
      for (auto & f : c->fields()) {
         if (!_field_to_map_offset.count(f)) {
            _field_to_map_offset[f] = field_offset++;
         }
      }
      // Build vtable
      for (auto & m : c->methods()) {
         std::string key = m->name();
         if (!_method_to_vtable_offset.count(key)) {
            _method_to_vtable_offset[key] = method_offset++;
         }
      }
   } 
   // Build main method
   std::shared_ptr<BasicBlock> main_block = std::make_shared<BasicBlock>("main");
   // Create method entrypoint with MAIN block (_curr_block will be LAST block here)
   _curr_program = std::make_shared<ProgramCFG>(std::make_shared<MethodCFG>(main_block));
   // Build every class
   for (auto & c : classes) {
      c->accept(*this);
   }
   _curr_block = main_block;
   // Initialize every local to 0 (0 << 1 + 1 => 1 as tagged)
   for (auto & l : node.main_locals()) {
      _curr_block->appendPrimitive(std::make_shared<AssignmentPrimitive>(toRegister(l), std::to_string(tag(0))));
   }
   // Recursively visit statements and build
   std::vector<std::shared_ptr<ASTStatement>> statements = node.main_statements();
   for (auto & s : statements) {
      if (_curr_block->isUnreachable()) {
         break;
      }
      s->accept(*this);
   }
   // Main will end with ret 0 due to default control value, or ret whatever specified
}

std::shared_ptr<ProgramCFG> CFGBuilder::build(std::shared_ptr<ProgramDeclaration> p) {
   p->accept(*this);
   return _curr_program;
}

