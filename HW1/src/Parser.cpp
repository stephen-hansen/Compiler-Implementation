#include <cctype>
#include <sstream>
#include "Parser.h"

int ProgramParser::skipChars(std::istream & input, std::string chars) {
   int count = 0;
   while (chars.find(input.peek()) != std::string::npos) {
      input.ignore();
      count++;
   }
   return count;
}

void ProgramParser::skipWhitespace(std::istream & input) {
   skipChars(input, " \t");
}

void ProgramParser::skipWhitespaceAndNewlines(std::istream & input) {
   skipChars(input, " \t\r\n");
}

void ProgramParser::advanceAndExpectChar(std::istream & input, char c, std::string addlInfo) {
   char nextChar = input.get();
   if (nextChar != c) {
      throw ParserException(std::string("Expected \'") + c + "\', instead got \'" + nextChar + "\'. Context: " + addlInfo);
   }
}

void ProgramParser::advanceAndExpectWord(std::istream & input, std::string word, std::string addlInfo) {
   for (char const & c : word) {
      try {
         advanceAndExpectChar(input, c, addlInfo);
      } catch (const ParserException & p) {
         throw ParserException(std::string("Expected \"") + word + "\", mismatch at char \'" + c + "\'. Context: " + addlInfo);
      }
   }
}

ASTExpression * ProgramParser::parseExpr(std::istream & input) {
   std::stringstream buf;
   // peek first char, determine type of expr
   char firstChar = input.peek();
   if (std::isdigit(firstChar)) {
      // Parse to integer expression
      // Will have length > 0 since firstChar exists
      for (char nextChar = input.peek(); std::isdigit(nextChar); buf << input.get());
      // Cast to 32 bit
      std::string numStr = buf.str();
      try {
         uint32_t val = static_cast<uint32_t>(std::stoul(numStr));
         return new UInt32Literal(val);
      } catch (const std::out_of_range & oor) {
         throw ParserException("Integer literal " + numStr + " is out of range.");
      } catch (const std::invalid_argument & ia) {
         throw ParserException("Integer literal " + numStr + " is an invalid value.");
      }
   } else if (std::isalpha(firstChar)) {
      // Parse to variable name
      // Will have length > 0 since firstChar exists
      for (char nextChar = input.peek(); std::isalpha(nextChar); buf << input.get());
      // Check buf, see if it is "this"
      std::string name = buf.str();
      // Build appropriate node
      if (name == "this") {
         return new ThisObjectExpression();
      } else {
         return new VariableIdentifier(name);
      }
   } else if (firstChar == '(') {
      // Skip '('
      input.ignore();
      // Skip whitespace
      skipWhitespace(input);
      // Parse to arithmetic expression
      // Recursively get first operand
      ASTExpression * e1 = parseExpr(input);
      // Skip whitespace
      skipWhitespace(input);
      // Get arithmetic operator
      char op = input.get();
      std::string valid_ops = "+-*/";
      if (valid_ops.find(op) == std::string::npos) {
         throw ParserException(std::string("\'") + op + "\' is not a valid arithmetic operator");
      }
      // Skip whitespace
      skipWhitespace(input);
      // Recursively get second operand
      ASTExpression * e2 = parseExpr(input);
      // Skip whitespace
      skipWhitespace(input);
      // Verify closing paren
      advanceAndExpectChar(input, ')', "ArithmeticExpression closing parenthesis");
      // Build ArithmeticExpression
      return new ArithmeticExpression(op, e1, e2);
   } else if (firstChar == '^') {
      // Skip '^'
      input.ignore();
      // Parse to method invocation
      // Parse object expression
      ASTExpression * e = parseExpr(input);
      // Verify . following e
      advanceAndExpectChar(input, '.', "CallExpression dot before method name");
      // Parse method name
      for (char nextChar = input.peek(); std::isalnum(nextChar); buf << input.get());
      if (buf.str().length() == 0) {
         throw ParserException("invalid method name");
      }
      std::string methodName = buf.str();
      // Verify ( following method name
      advanceAndExpectChar(input, '(', "CallExpression opening parenthesis before parameters");
      // Parse 0-MAXARGS arguments
      int numArgs = 0;
      std::vector<std::shared_ptr<ASTExpression>> args;
      while (numArgs < MAXARGS) {
         skipWhitespace(input);
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
         ASTExpression * arg = parseExpr(input);
         args.push_back(std::shared_ptr<ASTExpression>(arg));
         numArgs++;
      }
      skipWhitespace(input);
      advanceAndExpectChar(input, ')', "CallExpression closing parenthesis, or too many parameters");
      return new CallExpression(e, methodName, args);
   } else if (firstChar == '&') {
      // Skip '&'
      input.ignore();
      // Parse to field read
      // Get object expression
      ASTExpression * e = parseExpr(input);
      // Verify . following e
      advanceAndExpectChar(input, '.', "FieldReadExpression dot before field name");
      // Parse field name
      for (char nextChar = input.peek(); std::isalnum(nextChar); buf << input.get());
      if (buf.str().length() == 0) {
         throw ParserException("invalid field name");
      }
      std::string fieldName = buf.str();
      return new FieldReadExpression(e, fieldName);
   } else if (firstChar == '@') {
      // Skip '@'
      input.ignore();
      // Parse to new object
      // Parse class name
      for (char nextChar = input.peek(); std::isupper(nextChar); buf << input.get());
      if (buf.str().length() == 0) {
         throw ParserException("invalid class name");
      }
      std::string className = buf.str();
      return new NewObjectExpression(className);
   } else {
      // If we get an unexpected char or EOF we will hit this
      throw ParserException(std::string("\'") + firstChar + "\' does not start a valid expression");
   }
}

ASTStatement * ProgramParser::parseStmt(std::istream & input) {
   // Assume we start at the start of the statement (no starting whitespace)
   std::stringstream buf;
   // peek first char, determine type of expr
   char firstChar = input.peek();
   if (firstChar == '_') {
      // Don't care assignment
      skipWhitespace(input);
      // Verify = following _
      advanceAndExpectChar(input, '=', "DontCareAssignmentStatement = following _");
      skipWhitespace(input);
      ASTExpression * e = parseExpr(input);
      return new DontCareAssignmentStatement(e);
   } else if (firstChar == '!') {
      // Field update
      ASTExpression * obj = parseExpr(input);
      // Verify . following obj
      advanceAndExpectChar(input, '.', "FieldUpdateStatement . following expression");
      // Parse field name
      for (char nextChar = input.peek(); std::isalnum(nextChar); buf << input.get());
      if (buf.str().length() == 0) {
         throw ParserException("invalid field name");
      }
      std::string fieldName = buf.str();
      skipWhitespace(input);
      // Verify = following fieldName
      advanceAndExpectChar(input, '=', "FieldUpdateStatement = following field name");
      skipWhitespace(input);
      ASTExpression * val = parseExpr(input);
      return new FieldUpdateStatement(obj, fieldName, val);
   }
   // Okay, need more info
   // TODO
}

MethodDeclaration * ProgramParser::parseMethod(std::istream & input) {
}

ClassDeclaration * ProgramParser::parseClass(std::istream & input) {
}

ProgramDeclaration * ProgramParser::parse(std::istream & input) {
   // Parse all classes
   char firstChar;
   std::vector<std::shared_ptr<ClassDeclaration>> classes;
   while (true) {
      skipWhitespaceAndNewlines(input);
      firstChar = input.peek();
      if (firstChar == 'c') {
         // Assume it's a class, try to parse
         ClassDeclaration * newClass = parseClass(input);
         classes.push_back(std::shared_ptr<ClassDeclaration>(newClass));
         // Class will not parse ending whitespace, so parse here, assert newline
         skipWhitespace(input);
         advanceAndExpectChar(input, '\n', "Missing newline at end of class definition");
      } else {
         break;
      }
   }
   // Okay, we must be at main
   advanceAndExpectWord(input, "main", "Missing main program block");
   advanceAndExpectChar(input, ' ', "Program missing space after main");
   skipWhitespace(input);
   advanceAndExpectWord(input, "with", "Missing with after main");
   advanceAndExpectChar(input, ' ', "Program missing space after with");
   std::vector<std::string> main_locals;
   while (true) {
      skipWhitespace(input);
      std::stringstream buf;
      if (input.peek() == ':') {
         // Break early, no locals
         break;
      }
      for (char nextChar = input.peek(); std::isalpha(nextChar); buf << input.get());
      if (buf.str().length() == 0) {
         throw ParserException("main has invalid named local");
      }
      main_locals.push_back(buf.str());
      skipWhitespace(input);
      if (input.peek() != ',') {
         break;
      }
      // Skip ','
      input.ignore();
   }
   if (main_locals.size() == 0) {
      throw ParserException("Cannot have a program with zero local variables");
   }
   advanceAndExpectChar(input, ':', "Missing colon at end of main declaration");
   skipWhitespace(input);
   advanceAndExpectChar(input, '\n', "Missing newline at end of main declaration");
   // Parse all statements
   std::vector<std::shared_ptr<ASTStatement>> statements;
   while (true) {
      skipWhitespaceAndNewlines(input);
      if (input.peek() == EOF) {
         break;
      }
      // Arrived at first statement, parse it
      ASTStatement * statement = parseStmt(input);
      statements.push_back(std::shared_ptr<ASTStatement>(statement));
      // Statement will not parse ending whitespace, so parse here
      skipWhitespace(input);
      // Assert newline OR eof
      firstChar = input.peek();
      if (firstChar != '\n') {
         if (firstChar == EOF) {
            break;
         } else {
            throw ParserException("Statement does not end with newline or EOF");
         }
      }
   }

   return new ProgramDeclaration(classes, main_locals, statements);
}

