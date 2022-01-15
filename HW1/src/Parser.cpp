#include <cctype>
#include "Parser.h"
#include "Utils.h"

void ProgramParser::advanceAndExpectChar(std::istream & input, char c, std::string addlInfo) {
   char nextChar = input.get();
   if (nextChar != c) {
      throw ParserException(std::string("Expected \'") + c + "\', instead got \'" + nextChar + "\'. Context: " + addlInfo);
   }
}

ASTExpression ProgramParser::parseExpr(std::istream & input) {
   skipLeadingWhitespace(input);
   std::stringstream buf;
   // peek first char, determine type of expr
   char firstChar = input.peek();
   if (std::isdigit(firstChar)) {
      // Parse to integer expression
      for (char nextChar = input.peek(); std::isdigit(nextChar); buf << input.get());
      // Cast to 32 bit
      std::string numStr = buf.str();
      try {
         uint32_t val = static_cast<uint32_t>(std::stoul(numStr));
         return UInt32Literal(val);
      } catch (const std::out_of_range & oor) {
         throw ParserException("Integer literal " + numStr + " is out of range.");
      } catch (const std::invalid_argument & ia) {
         throw ParserException("Integer literal " + numStr + " is an invalid value.");
      }
   } else if (std::isalpha(firstChar)) {
      // Parse to variable name
      for (char nextChar = input.peek(); std::isalpha(nextChar); buf << input.get());
      // Check buf, see if it is "this"
      std::string name = buf.str();
      // Build appropriate node
      if (name == "this") {
         return ThisObject();
      } else {
         return VariableIdentifier(name);
      }
   } else if (firstChar == '(') {
      // Skip '('
      input.ignore();
      // Parse to arithmetic expression
      // Recursively get first operand
      ASTExpression e1 = parseExpr(input);
      // Verify space following e1
      advanceAndExpectChar(input, ' ', "ArithmeticExpression space before operator");
      char op = input.get();
      std::string valid_ops = "+-*/";
      if (valid_ops.find(op) == std::string::npos) {
         throw ParserException(std::string("\'") + op + "\' is not a valid arithmetic operator");
      }
      advanceAndExpectChar(input, ' ', "ArithmeticExpression space after operator");
      // Recursively get second operand
      ASTExpression e2 = parseExpr(input);
      // Verify closing paren
      advanceAndExpectChar(input, ')', "ArithmeticExpression closing parenthesis");
      // Build ArithmeticExpression
      return ArithmeticExpression(op, e1, e2);
   } else if (firstChar == '^') {
      // Skip '^'
      input.ignore();
      // Parse to method invocation
      // Parse object expression
      ASTExpression e = parseExpr(input);
      // Verify . following e
      advanceAndExpectChar(input, '.', "CallExpression dot before method name");
      // Parse method name
      for (char nextChar = input.peek(); std::isalnum(nextChar); buf << input.get());
      std::string methodName = buf.str();
      // Verify ( following method name
      advanceAndExpectChar(input, '(', "CallExpression opening parenthesis before parameters");
      // Parse 0-MAXARGS arguments
      int numArgs = 0;
      std::vector<ASTExpression> args;
      while (numArgs < MAXARGS) {
         char nextChar = input.peek();
         if (nextChar == ')') {
            // Done parsing args
            break;
         }
         if (numArgs > 0) {
            if (nextChar == ',') {
               // Skip ','
               input.ignore();
            } else {
               throw ParserException(std::string("Expected \',\', instead got \'") + nextChar + "\'. Context: CallExpression comma before subsequent parameters");
            }
         }
         // Parse arg recursively
         ASTExpression arg = parseExpr(input);
         args.push_back(arg);
         numArgs++;
      }
      advanceAndExpectChar(input, ')', "CallExpression closing parenthesis, or too many parameters");
      return CallExpression(e, methodName, args);
   } else if (firstChar == '&') {
      // Skip '&'
      input.ignore();
      // Parse to field read
      // Get object expression
      ASTExpression e = parseExpr(input);
      // Verify . following e
      advanceAndExpectChar(input, '.', "FieldReadExpression dot before field name");
      // Parse field name
      for (char nextChar = input.peek(); std::isalnum(nextChar); buf << input.get());
      std::string fieldName = buf.str();
      return FieldReadExpression(e, fieldName);
   } else if (firstChar == '@') {
      // Skip '@'
      input.ignore();
      // Parse to new object
      // Parse class name
      for (char nextChar = input.peek(); std::isupper(nextChar); buf << input.get());
      std::string className = buf.str();
      return NewObjectExpression(className);
   } else {
      // If we get an unexpected char or EOF we will hit this
      throw ParserException(std::string("\'") + firstChar + "\' does not start a valid expression");
   }
}

ASTStatement ProgramParser::parseStmt(std::istream & input) {
   return ASTStatement();
}

MethodDeclaration ProgramParser::parseMethod(std::istream & input) {
   return MethodDeclaration();
}

ClassDeclaration ProgramParser::parseClass(std::istream & input) {
   return ClassDeclaration();
}

ProgramDeclaration ProgramParser::parse(std::istream & input) {
   return ProgramDeclaration();
}

