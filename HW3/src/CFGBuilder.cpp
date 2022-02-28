#include <algorithm>
#include "AST.h"
#include "CFG.h"
#include "CFGBuilder.h"

void CFGBuilder::visit(UInt32Literal& node) {
   // Don't care about input value
   _input_values.pop();
   // Send int back as value
   _return_values.push(std::make_pair(std::to_string(node.val()), INT));
}

void CFGBuilder::visit(VariableIdentifier& node) {
   // Don't care about input value
   _input_values.pop();
   // Send variable back as named register
   std::string reg = toRegister(node.name());
   _return_values.push(std::make_pair(reg, _curr_method->getType(reg)));
}

void CFGBuilder::visit(ArithmeticExpression& node) {
   // Visit e1, e2
   _input_values.push(TEMP);
   node.e1()->accept(*this);
   std::pair<std::string, std::string> p1 = _return_values.top();
   std::string r1 = p1.first;
   std::string rt1 = p1.second;
   _return_values.pop();
   _input_values.push(TEMP);
   node.e2()->accept(*this);
   std::pair<std::string, std::string> p2 = _return_values.top();
   std::string r2 = p2.first;
   std::string rt2 = p2.second;
   _return_values.pop();
   // _curr_block->appendPrimitive(std::make_shared<Comment>(node.toSourceString()));
   // Derive operation
   char op = node.op();
   std::string ret = setReturnName(INT);
   _curr_block->appendPrimitive(std::make_shared<ArithmeticPrimitive>(ret, r1, op, r2));
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
   std::string receivertype = _return_values.top().second;
   _return_values.pop();
   // Check that receiver is NOT NULL
   nonzeroCheck(receiver, BADPOINTER, NOT_A_POINTER);
   // _curr_block->appendPrimitive(std::make_shared<Comment>(node.toSourceString()));
   // Load vtable
   std::string vtable = createTemp(VTBL);
   _curr_block->appendPrimitive(std::make_shared<LoadPrimitive>(vtable, receiver));
   // Lookup method ID corresponding to method string
   std::string methodAddr = createTemp(METHOD);
   std::string method = node.method();
   std::string index = std::to_string(_method_to_vtable_offset[method]);
   _curr_block->appendPrimitive(std::make_shared<GetEltPrimitive>(methodAddr, vtable, index));
   // Type of callee is receivertype
   // Check type of return from method
   std::string methodreturn = _program_ast->classes()[receivertype]->methods()[method]->return_type();
   // Call and return
   std::string ret = setReturnName(methodreturn);
   _curr_block->appendPrimitive(std::make_shared<CallPrimitive>(ret, methodAddr, receiver, regParams));
}

void CFGBuilder::visit(FieldReadExpression& node) {
   // Visit obj
   _input_values.push(TEMP);
   node.obj()->accept(*this);
   std::string baseaddr = _return_values.top().first;
   std::string basetype = _return_values.top().second;
   _return_values.pop();
   // Check that baseaddr is NOT NULL
   nonzeroCheck(baseaddr, BADPOINTER, NOT_A_POINTER);
   // _curr_block->appendPrimitive(std::make_shared<Comment>(node.toSourceString()));
   // Lookup field offset corresponding to field string
   // Just do this directly off type
   std::string field = node.field();
   std::string fieldOffset = std::to_string(_curr_program->classes()[basetype]->field_table()[field]);
   std::string fieldType = _curr_program->classes()[basetype]->getType(field);
   // Get and return
   std::string ret = setReturnName(fieldType);
   _curr_block->appendPrimitive(std::make_shared<GetEltPrimitive>(ret, baseaddr, fieldOffset));
}

void CFGBuilder::visit(NewObjectExpression& node) {
   // _curr_block->appendPrimitive(std::make_shared<Comment>(node.toSourceString()));
   // Get the return value early so we can alloc at it
   std::string classname = node.class_name();
   std::string ret = setReturnName(classname);
   // Get size to allocate
   std::string allocSize = std::to_string(_class_name_to_alloc_size[classname]);
   // Allocate the class
   _curr_block->appendPrimitive(std::make_shared<AllocPrimitive>(ret, allocSize));
   // Store the vtbl
   _curr_block->appendPrimitive(std::make_shared<StorePrimitive>(ret, toGlobal(toVtable(classname))));
   // Set all fields to 0
   for (unsigned int i=0; i<_class_name_to_num_fields[classname]; i++) {
      _curr_block->appendPrimitive(std::make_shared<SetEltPrimitive>(ret, std::to_string(i+1), std::to_string(0)));
   }
}

void CFGBuilder::visit(ThisObjectExpression& node) {
   // Don't care about input value
   _input_values.pop();
   // Send variable back as "this" register
   // Technically should always hold pointer, but no guarantees
   std::string reg = toRegister("this");
   std::string classname = _curr_class->name();
   _return_values.push(std::make_pair(toRegister("this"), classname));
}

void CFGBuilder::visit(NullObjectExpression& node) {
   // Don't care about input value
   _input_values.pop();
   // Send back int literal 0 (POINTER)
   std::string classname = node.class_name();
   _return_values.push(std::make_pair("0", classname));
}

void CFGBuilder::visit(AssignmentStatement& node) {
   std::string destRegister = toRegister(node.variable());
   // Set destRegister as input value
   _input_values.push(destRegister);
   // Visit expression
   // _curr_block->appendPrimitive(std::make_shared<Comment>(node.toSourceString()));
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
   // _curr_block->appendPrimitive(std::make_shared<Comment>(node.toSourceString()));
   node.val()->accept(*this);
   // Don't care about return value
   _return_values.pop();
}

void CFGBuilder::visit(FieldUpdateStatement& node) {
   // Visit obj
   _input_values.push(TEMP);
   node.obj()->accept(*this);
   std::string baseaddr = _return_values.top().first;
   std::string basetype = _return_values.top().second;
   _return_values.pop();
   // Check that baseaddr is NOT NULL
   nonzeroCheck(baseaddr, BADPOINTER, NOT_A_POINTER);
   // _curr_block->appendPrimitive(std::make_shared<Comment>(node.toSourceString()));
   // Lookup field offset corresponding to field string
   // Just do this directly off type
   std::string field = node.field();
   std::string fieldOffset = std::to_string(_curr_program->classes()[basetype]->field_table()[field]);
   std::string fieldType = _curr_program->classes()[basetype]->getType(field);
   // Visit val
   _input_values.push(TEMP);
   node.val()->accept(*this);
   std::string val = _return_values.top().first;
   _return_values.pop();
   _curr_block->appendPrimitive(std::make_shared<SetEltPrimitive>(baseaddr, fieldOffset, val));
}

void CFGBuilder::visit(IfElseStatement& node) {
   // Visit cond
   _input_values.push(TEMP);
   node.cond()->accept(*this);
   std::string cond = _return_values.top().first;
   std::string rt = _return_values.top().second;
   _return_values.pop();
   // Create if true block
   std::string trueLabel = createLabel();
   std::shared_ptr<BasicBlock> true_block = std::make_shared<BasicBlock>(trueLabel);
   // If block owns true block
   addNewChild(_curr_block, true_block);
   // Create if false block
   std::string falseLabel = createLabel();
   std::shared_ptr<BasicBlock> false_block = std::make_shared<BasicBlock>(falseLabel);
   // If block owns false block
   addNewChild(_curr_block, false_block);
   // End current block with if/else
   // _curr_block->appendPrimitive(std::make_shared<Comment>(node.toSourceString()));
   _curr_block->setControl(std::make_shared<IfElseControl>(cond, trueLabel, falseLabel));
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
      addNewChild(last_false_block, final_block);
      final_block_is_owned = true;
      last_false_block->setControl(std::make_shared<JumpControl>(finalLabel));
   }
   if (!last_true_block->isUnreachable()) {
      if (!final_block_is_owned) {
         addNewChild(last_true_block, final_block);
         final_block_is_owned = true;
      } else {
         addExistingChild(last_true_block, final_block);
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
   std::string rt = _return_values.top().second;
   _return_values.pop();
   // Create if true block
   std::string trueLabel = createLabel();
   std::shared_ptr<BasicBlock> true_block = std::make_shared<BasicBlock>(trueLabel);
   // If block owns true block
   addNewChild(_curr_block, true_block);
   // Create if false block
   std::string falseLabel = createLabel();
   std::shared_ptr<BasicBlock> false_block = std::make_shared<BasicBlock>(falseLabel);
   // If block owns false block
   addNewChild(_curr_block, false_block);
   // End current block with if/else
   // _curr_block->appendPrimitive(std::make_shared<Comment>(node.toSourceString()));
   _curr_block->setControl(std::make_shared<IfElseControl>(cond, trueLabel, falseLabel));
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
      addExistingChild(_curr_block, false_block);
   }
   // Update current block to false block
   _curr_block = false_block;
}

void CFGBuilder::visit(WhileStatement& node) {
   // End current block by jumping to conditional block
   std::string condLabel = createLabel();
   std::shared_ptr<BasicBlock> cond_block = std::make_shared<BasicBlock>(condLabel);
   // Original block owns cond block
   addNewChild(_curr_block, cond_block);
   _curr_block->setControl(std::make_shared<JumpControl>(condLabel));
   // Visit cond
   _curr_block = cond_block;
   _input_values.push(TEMP);
   node.cond()->accept(*this);
   std::string cond = _return_values.top().first;
   std::string rt = _return_values.top().second;
   _return_values.pop();
   // Create if true block
   std::string trueLabel = createLabel();
   std::shared_ptr<BasicBlock> true_block = std::make_shared<BasicBlock>(trueLabel);
   // curr block owns true block
   addNewChild(_curr_block, true_block);
   // Create if false block
   std::string falseLabel = createLabel();
   std::shared_ptr<BasicBlock> false_block = std::make_shared<BasicBlock>(falseLabel);
   // curr block owns false block
   addNewChild(_curr_block, false_block);
   // End current block with if/else
   // _curr_block->appendPrimitive(std::make_shared<Comment>(node.toSourceString()));
   _curr_block->setControl(std::make_shared<IfElseControl>(cond, trueLabel, falseLabel));
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
      addExistingChild(_curr_block, cond_block);
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
   // _curr_block->appendPrimitive(std::make_shared<Comment>(node.toSourceString()));
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
   std::string retType = _return_values.top().second;
   // Print should probably do a tag check and de-convert the value by dividing by 2
   _return_values.pop();
   // _curr_block->appendPrimitive(std::make_shared<Comment>(node.toSourceString()));
   // Add print primitive
   _curr_block->appendPrimitive(std::make_shared<PrintPrimitive>(retValue));
}

void CFGBuilder::visit(MethodDeclaration& node) {
   // Reset temporary counter
   resetCounter();
   std::string methodName = toMethodName(_curr_class->name(), node.name());
   // Build list of parameters and types
   std::vector<std::string> params;
   std::map<std::string, std::string> var_to_type;
   // First param is %this
   std::string thisReg = toRegister("this");
   params.push_back(thisReg);
   // %this is an object of the current class
   var_to_type[thisReg] = _curr_class->name();
   // Convert method params to remaining params
   for (auto & p : node.params()) {
      // p.first is name, p.second is type
      std::string reg = toRegister(p.first);
      params.push_back(reg);
      var_to_type[reg] = p.second;
   }
   std::shared_ptr<BasicBlock> entry_block = std::make_shared<BasicBlock>(methodName, params);
   _curr_block = entry_block;
   // Initialize every local to 0
   for (auto & l : node.locals()) {
      std::string reg = toRegister(l.first);
      _curr_block->appendPrimitive(std::make_shared<AssignmentPrimitive>(reg, std::to_string(0)));
      var_to_type[reg] = l.second;
   }
   // Append method with ENTRY block (_curr_block will be LAST block here)
   std::vector<std::pair<std::string, std::string>> m_params = node.params();
   // Combine params
   std::vector<std::string> m_paramnames;
   for (auto & p : m_params) {
      m_paramnames.push_back(p.first);
   }
   std::vector<std::pair<std::string, std::string>> m_locals = node.locals();
   std::vector<std::string> m_localnames;
   for (auto & l : m_locals) {
      m_localnames.push_back(l.first);
   }
   std::vector<std::string> variables;
   variables.reserve(m_paramnames.size() + m_localnames.size());
   variables.insert(variables.end(), m_paramnames.begin(), m_paramnames.end());
   variables.insert(variables.end(), m_localnames.begin(), m_localnames.end());
   _curr_method = std::make_shared<MethodCFG>(entry_block, variables, var_to_type);
   // Recursively visit statements and build
   std::vector<std::shared_ptr<ASTStatement>> statements = node.statements();
   for (auto & s : statements) {
      if (_curr_block->isUnreachable()) {
         break;
      }
      s->accept(*this);
   }
   _curr_class->appendMethod(_curr_method);
}

void CFGBuilder::visit(ClassDeclaration& node) {
   std::map<std::string, std::shared_ptr<MethodDeclaration>> methods = node.methods();
   _curr_class = _curr_program->classes()[node.name()];
   // Build all methods
   for (auto & m : methods) {
      m.second->accept(*this);
   }
}

void CFGBuilder::visit(ProgramDeclaration& node) {
   _program_ast = std::make_shared<ProgramDeclaration>(node);
   std::map<std::string, std::shared_ptr<ClassDeclaration>> classes = node.classes();
   // Location in field map
   int field_offset = 0;
   // Location in vtable
   int method_offset = 0;
   // Class maps must be built first to understand offsets for fields, methods
   for (auto & cl : classes) {
      std::shared_ptr<ClassDeclaration> c = cl.second;
      // Build map of class names to allocation sizes
      // Size is always 1 + number of fields
      _class_name_to_alloc_size[c->name()] = 1 + c->fields().size();
      _class_name_to_num_fields[c->name()] = c->fields().size();
      // Build field map
      for (auto & fi : c->fields()) {
         std::string f = fi.first;
         if (!_field_to_map_offset.count(f)) {
            _field_to_map_offset[f] = field_offset++;
         }
      }
      // Build vtable
      for (auto & m : c->methods()) {
         std::string key = m.second->name();
         if (!_method_to_vtable_offset.count(key)) {
            _method_to_vtable_offset[key] = method_offset++;
         }
      }
   }
   for (auto & cl : classes) {
      // Build auxiliary info
      std::shared_ptr<ClassDeclaration> node = cl.second;
      std::map<std::string, std::shared_ptr<MethodDeclaration>> methods = node->methods();
      std::vector<std::string> vtable;
      vtable.resize(_method_to_vtable_offset.size());
      std::fill(vtable.begin(), vtable.end(), std::string("0"));
      for (auto & m : methods) {
         int loc = _method_to_vtable_offset[node->name()];
         std::string methodname = toMethodName(node->name(), m.second->name());
         vtable[loc] = methodname;
      }
      // Construct the fields map
      std::map<std::string, std::string> fields = node->fields();
      std::vector<std::string> fieldnames;
      std::map<std::string, std::string> field_to_type;
      for (auto & fi : fields) {
         fieldnames.push_back(fi.first);
         field_to_type[fi.first] = fi.second;
      }
      std::map<std::string, unsigned long> fieldsMap;
      int index = 1;
      for (auto & f : fieldnames) {
         unsigned long offset = index++;
         fieldsMap[f] = offset;
      }
      std::shared_ptr<ClassCFG> newclass = std::make_shared<ClassCFG>(node->name(), vtable, fieldsMap, field_to_type);
      // Add class to program
      _curr_program->appendClass(newclass);
   }
   // Build main method
   std::shared_ptr<BasicBlock> main_block = std::make_shared<BasicBlock>("main");
   std::vector<std::pair<std::string, std::string>> variables = node.main_locals();
   std::map<std::string, std::string> var_to_type;
   // Convert variables to single vector
   std::vector<std::string> varnames;
   for (auto & loc : variables) {
      varnames.push_back(loc.first);
      var_to_type[toRegister(loc.first)] = loc.second;
   }
   // Create method entrypoint with MAIN block (_curr_block will be LAST block here)
   std::shared_ptr<MethodCFG> main_method = std::make_shared<MethodCFG>(main_block, varnames, var_to_type);
   _curr_program = std::make_shared<ProgramCFG>(main_method);
   // Build every class
   for (auto & cl : classes) {
      cl.second->accept(*this);
   }
   resetCounter();
   _curr_block = main_block;
   _curr_method = main_method;
   // Initialize every local to 0
   for (auto & loc : node.main_locals()) {
      _curr_block->appendPrimitive(std::make_shared<AssignmentPrimitive>(toRegister(loc.first), std::to_string(0)));
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

