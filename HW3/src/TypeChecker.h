#ifndef _CS_441_TYPE_CHECKER_H
#define _CS_441_TYPE_CHECKER_H
#include <exception>
#include <map>
#include <set>
#include <string>
#include <utility>
#include "AST.h"

#define INT "int"

class TypeCheckerException : public std::exception
{
   private:
      std::string _info;
      std::string _line;
   public:
      TypeCheckerException(std::string info, std::string line): _info(info), _line(line) {}
      std::string info() { return _info; }
      std::string line() { return _line; }
};

class TypeChecker : public ASTVisitor
{
   private:
      std::string _return_type;
      std::string _curr_class;
      std::map<std::string, std::string> _type_environ;
      std::map<std::string, std::shared_ptr<ClassDeclaration>> _classes;
      
   public:
      void visit(UInt32Literal& node) {
         // Literal has type INT, return INT
         _return_type = INT;
      }

      void visit(VariableIdentifier& node) {
         // Look up type of variable from environ
         _return_type = _type_environ[node.name()];
      }

      void visit(ArithmeticExpression& node) {
         // Check underlying type of expression 1
         node.e1()->accept(*this);
         if (_return_type != INT) {
            throw TypeCheckerException("First subexpression of arithmetic expression does not return an integer", node.toSourceString());
         }
         // Check underyling type of expression 2
         node.e2()->accept(*this);
         if (_return_type != INT) {
            throw TypeCheckerException("Second subexpression of arithmetic expression does not return an integer", node.toSourceString());
         }
         // Arithmetic returns int
         _return_type = INT;
      }

      void visit(CallExpression& node) {
         // Check that the class is valid
         node.obj()->accept(*this);
         if (_return_type == INT || _classes.find(_return_type) == _classes.end()) {
            throw TypeCheckerException("Calling method given invalid class", node.toSourceString());
         }
         // Check that method exists in class
         std::shared_ptr<ClassDeclaration> c = _classes[_return_type];
         std::map<std::string, std::shared_ptr<MethodDeclaration>> methods = c->methods();
         std::string methodname = node.method();
         if (methods.find(methodname) == methods.end()) {
            throw TypeCheckerException("Method given does not exist in supplied object", node.toSourceString());
         }
         // Validate the method parameters
         std::shared_ptr<MethodDeclaration> method = methods[methodname];
         std::vector<std::pair<std::string, std::string>> paramnames = method->params();
         size_t len1 = paramnames.size();
         std::vector<std::shared_ptr<ASTExpression>> params = node.params();
         size_t len2 = params.size();
         if (len1 != len2) {
            throw TypeCheckerException("Calling method with wrong number of parameters", node.toSourceString());
         }
         for (size_t i = 0; i < len1; i++) {
            std::string expected_type = paramnames[i].second;
            params[i]->accept(*this);
            if (_return_type != expected_type) {
               throw TypeCheckerException("Calling method with wrong parameter type", node.toSourceString());
            }
         }
         // Return type is return type of the method
         _return_type = method->return_type();
      }

      void visit(FieldReadExpression& node) {
         // Check that the class is valid
         node.obj()->accept(*this);
         if (_return_type == INT || _classes.find(_return_type) == _classes.end()) {
            throw TypeCheckerException("Field read given invalid class", node.toSourceString());
         }
         // Check that field exists in class
         std::shared_ptr<ClassDeclaration> c = _classes[_return_type];
         std::map<std::string, std::string> fields = c->fields();
         std::string fieldname = node.field();
         if (fields.find(fieldname) == fields.end()) {
            throw TypeCheckerException("Field given does not exist in supplied object", node.toSourceString());
         }
         // Return type is type of the field
         _return_type = fields[fieldname];
      }

      void visit(NewObjectExpression& node) {
         // Check that the class is valid
         std::string classname = node.class_name();
         if (classname == INT || _classes.find(classname) == _classes.end()) {
            throw TypeCheckerException("Null object returns invalid class", node.toSourceString());
         }
         // Return type is whatever was given
         _return_type = classname;
      }

      void visit(ThisObjectExpression& node) {
         // No validation necessary, unless not in a class
         if (_curr_class == "") {
            throw TypeCheckerException("Using \"this\" outside context of a class method", node.toSourceString());
         }
         // Return type is current class
         _return_type = _curr_class;
      }

      void visit(NullObjectExpression& node) {
         // Check that the class is valid
         std::string classname = node.class_name();
         if (classname == INT || _classes.find(classname) == _classes.end()) {
            throw TypeCheckerException("Null object returns invalid class", node.toSourceString());
         }
         // Return type is whatever was given
         _return_type = classname;
      }

      void visit(AssignmentStatement& node) {
         // Consult the environment for LHS type
         std::string expected_type = _type_environ[node.variable()];
         // Evaluate value
         node.val()->accept(*this);
         // Validate legitimate type
         if (_return_type != INT && _classes.find(_return_type) == _classes.end()) {
            throw TypeCheckerException("Return type of expression does not exist", node.toSourceString());
         }
         // Validate correct type
         if (_return_type != expected_type) {
            throw TypeCheckerException("Return type of expression is not the same as the type of the variable", node.toSourceString());
         }
      }

      void visit(DontCareAssignmentStatement& node) {
         // Result doesn't get used
         // Just validate that the type is legitimate
         node.val()->accept(*this);
         if (_return_type != INT && _classes.find(_return_type) == _classes.end()) {
            throw TypeCheckerException("Return type of expression does not exist", node.toSourceString());
         }
      }
      void visit(FieldUpdateStatement& node) {
         // Check that the class is valid
         node.obj()->accept(*this);
         if (_return_type == INT || _classes.find(_return_type) == _classes.end()) {
            throw TypeCheckerException("Field update given invalid class", node.toSourceString());
         }
         // Check that field exists in class
         std::shared_ptr<ClassDeclaration> c = _classes[_return_type];
         std::map<std::string, std::string> fields = c->fields();
         std::string fieldname = node.field();
         if (fields.find(fieldname) == fields.end()) {
            throw TypeCheckerException("Field given does not exist in supplied object", node.toSourceString());
         }
         // Check if val has expected type
         std::string expected_type = fields[fieldname];
         node.val()->accept(*this);
         if (_return_type != INT && _classes.find(_return_type) == _classes.end()) {
            throw TypeCheckerException("Return type of expression does not exist", node.toSourceString());
         }
         if (_return_type != expected_type) {
            throw TypeCheckerException("Return type of expression is not the same as the type of the field", node.toSourceString());
         }
      }
      void visit(IfElseStatement& node);
      void visit(IfOnlyStatement& node);
      void visit(WhileStatement& node);
      void visit(ReturnStatement& node);
      void visit(PrintStatement& node);
      void visit(MethodDeclaration& node) {
         _type_environ.clear();
         // TODO set up type environ
      }

      void visit(ClassDeclaration& node) {
         // Store class name for this statements
         _curr_class = node.name();
      }
      void visit(ProgramDeclaration& node) {
         // Save all class declarations
         _classes = node.classes();
      }
      void check(std::shared_ptr<ProgramDeclaration> p) {
         p->accept(*this);
      }
};

#endif
