#include <cctype>
#include <iostream>
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

int ProgramParser::skipWhitespace(std::istream & input) {
   return skipChars(input, " \t");
}

int ProgramParser::skipWhitespaceAndNewlines(std::istream & input) {
   return skipChars(input, " \t\r\n");
}

void ProgramParser::advanceAndExpectChar(std::istream & input, char c, std::string addlInfo) {
   char nextChar = input.get();
   if (nextChar != c) {
      // Windows hack, replace search for '\n' with search for "\r\n"
      if (nextChar == '\r' && c == '\n' && input.peek() == '\n') {
         input.ignore();
      } else {
         throw ParserException(std::string("Expected \'") + c + "\', instead got \'" + nextChar + "\'. Context: " + addlInfo);
      }
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
      for (; std::isdigit(input.peek()); buf << static_cast<char>(input.get()));
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
      for (; std::isalpha(input.peek()); buf << static_cast<char>(input.get()));
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
      for (; std::isalnum(input.peek()); buf << static_cast<char>(input.get()));
      if (buf.str().length() == 0) {
         throw ParserException("invalid empty method name");
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
               skipWhitespace(input);
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
      for (; std::isalnum(input.peek()); buf << static_cast<char>(input.get()));
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
      for (; std::isupper(input.peek()); buf << static_cast<char>(input.get()));
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
      // Skip '_'
      input.ignore();
      // Don't care assignment
      skipWhitespace(input);
      // Verify = following _
      advanceAndExpectChar(input, '=', "DontCareAssignmentStatement = following _");
      skipWhitespace(input);
      ASTExpression * e = parseExpr(input);
      return new DontCareAssignmentStatement(e);
   } else if (firstChar == '!') {
      // Skip '!'
      input.ignore();
      // Field update
      ASTExpression * obj = parseExpr(input);
      // Verify . following obj
      advanceAndExpectChar(input, '.', "FieldUpdateStatement . following expression");
      // Parse field name
      for (; std::isalnum(input.peek()); buf << static_cast<char>(input.get()));
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
   // Go up to first non-alpha char
   for (; std::isalpha(input.peek()); buf << static_cast<char>(input.get()));
   std::string keyword = buf.str();
   if (input.peek() == '(') {
      // Expecting print(e)
      if (keyword != "print") {
         throw ParserException(std::string("Expected print statement, instead got \"") + buf.str() + "\"");
      }
      // Skip '('
      input.ignore();
      skipWhitespace(input);
      // Parse expr
      ASTExpression * e = parseExpr(input);
      skipWhitespace(input);
      advanceAndExpectChar(input, ')', "PrintStatement missing closing parenthesis");
      return new PrintStatement(e);
   }
   skipWhitespace(input);
   if (input.peek() == '=') {
      // Assignment statement, keyword is varname
      // Skip =
      input.ignore();
      skipWhitespace(input);
      // Parse expr
      ASTExpression * e = parseExpr(input);
      return new AssignmentStatement(keyword, e);
   }
   // Must be a keyword, and input is pointing at expression now
   if (keyword == "if") {
      ASTExpression * cond = parseExpr(input);
      skipWhitespace(input);
      advanceAndExpectChar(input, ':', "IfElseStatement missing colon after cond");
      skipWhitespace(input);
      advanceAndExpectChar(input, '{', "IfElseStatement missing opening brace");
      skipWhitespace(input);
      advanceAndExpectChar(input, '\n', "IfElseStatement missing initial newline");
      std::vector<std::shared_ptr<ASTStatement>> if_statements;
      while (true) {
         skipWhitespaceAndNewlines(input);
         if (input.peek() == '}') {
            input.ignore();
            break;
         }
         // Find next statement
         ASTStatement * stmt = parseStmt(input);
         if_statements.push_back(std::shared_ptr<ASTStatement>(stmt));
         skipWhitespace(input);
         advanceAndExpectChar(input, '\n', "IfElseStatement missing newline between if statements");
      }
      if (if_statements.size() == 0) {
         throw ParserException(std::string("IfElseStatement cannot have 0 if statements"));
      }
      skipWhitespaceAndNewlines(input);
      advanceAndExpectWord(input, "else", "IfElseStatement missing else");
      skipWhitespace(input);
      advanceAndExpectChar(input, '{', "IfElseStatement missing opening brace after else");
      skipWhitespace(input);
      advanceAndExpectChar(input, '\n', "IfElseStatement missing initial newline after else");
      std::vector<std::shared_ptr<ASTStatement>> else_statements;
      while (true) {
         skipWhitespaceAndNewlines(input);
         if (input.peek() == '}') {
            input.ignore();
            break;
         }
         // Find next statement
         ASTStatement * stmt = parseStmt(input);
         else_statements.push_back(std::shared_ptr<ASTStatement>(stmt));
         skipWhitespace(input);
         advanceAndExpectChar(input, '\n', "IfElseStatement missing newline between else statements");
      }
      if (else_statements.size() == 0) {
         throw ParserException(std::string("IfElseStatement cannot have 0 else statements"));
      }
      return new IfElseStatement(cond, if_statements, else_statements);
   } else if (keyword == "ifonly") {
      ASTExpression * cond = parseExpr(input);
      skipWhitespace(input);
      advanceAndExpectChar(input, ':', "IfOnlyStatement missing colon after cond");
      skipWhitespace(input);
      advanceAndExpectChar(input, '{', "IfOnlyStatement missing opening brace");
      skipWhitespace(input);
      advanceAndExpectChar(input, '\n', "IfOnlyStatement missing initial newline");
      std::vector<std::shared_ptr<ASTStatement>> statements;
      while (true) {
         skipWhitespaceAndNewlines(input);
         if (input.peek() == '}') {
            input.ignore();
            break;
         }
         // Find next statement
         ASTStatement * stmt = parseStmt(input);
         statements.push_back(std::shared_ptr<ASTStatement>(stmt));
         skipWhitespace(input);
         advanceAndExpectChar(input, '\n', "IfOnlyStatement missing newline between statements");
      }
      if (statements.size() == 0) {
         throw ParserException(std::string("IfOnlyStatement cannot have 0 statements"));
      }
      return new IfOnlyStatement(cond, statements);
   } else if (keyword == "while") {
      ASTExpression * cond = parseExpr(input);
      skipWhitespace(input);
      advanceAndExpectChar(input, ':', "WhileStatement missing colon after cond");
      skipWhitespace(input);
      advanceAndExpectChar(input, '{', "WhileStatement missing opening brace");
      skipWhitespace(input);
      advanceAndExpectChar(input, '\n', "WhileStatement missing initial newline");
      std::vector<std::shared_ptr<ASTStatement>> statements;
      while (true) {
         skipWhitespaceAndNewlines(input);
         if (input.peek() == '}') {
            input.ignore();
            break;
         }
         // Find next statement
         ASTStatement * stmt = parseStmt(input);
         statements.push_back(std::shared_ptr<ASTStatement>(stmt));
         skipWhitespace(input);
         advanceAndExpectChar(input, '\n', "WhileStatement missing newline between statements");
      }
      if (statements.size() == 0) {
         throw ParserException(std::string("WhileStatement cannot have 0 statements"));
      }
      return new WhileStatement(cond, statements);
   } else if (keyword == "return") {
      ASTExpression * e = parseExpr(input);
      return new ReturnStatement(e);
   }
   throw ParserException(std::string("Found statement starting with \"" + keyword + "\" which is not a valid keyword"));
}

MethodDeclaration * ProgramParser::parseMethod(std::istream & input) {
   // Expect "method"
   advanceAndExpectWord(input, "method", "Method declaration must start with method");
   advanceAndExpectChar(input, ' ', "Space must follow method keyword");
   skipWhitespace(input);
   // Read method name
   std::stringstream buf;
   // Parse method name
   for (; std::isalnum(input.peek()); buf << static_cast<char>(input.get()));
   if (buf.str().length() == 0) {
      throw ParserException("invalid empty method name");
   }
   std::string methodname = buf.str();
   advanceAndExpectChar(input, '(', "Method missing opening parenthesis");
   // Parse 0-MAXARGS arguments
   int numArgs = 0;
   std::vector<std::string> args;
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
            skipWhitespace(input);
         } else {
            throw ParserException(std::string("Expected \',\', instead got \'") + nextChar + "\'. Context: Method parameters");
         }
      }
      std::stringstream buf2;
      // Parse paramname
      for (; std::isalpha(input.peek()); buf2 << static_cast<char>(input.get()));
      std::string arg = buf2.str();
      args.push_back(arg);
      numArgs++;
   }
   skipWhitespace(input);
   advanceAndExpectChar(input, ')', "Method closing parenthesis, or too many parameters");
   advanceAndExpectChar(input, ' ', "Missing space after method params list");
   skipWhitespace(input);
   advanceAndExpectWord(input, "with", "Missing with after method signature");
   advanceAndExpectChar(input, ' ', "Missing space after with in method signature");
   skipWhitespace(input);
   advanceAndExpectWord(input, "locals", "Missing locals after method signature");
   std::vector<std::string> locals;
   numArgs = 0;
   while (true) {
      skipWhitespace(input);
      char nextChar = input.peek();
      if (nextChar == ':') {
         // Done parsing locals
         break;
      }
      if (numArgs > 0) {
         if (nextChar == ',') {
            // Skip ','
            input.ignore();
            skipWhitespace(input);
         } else {
            throw ParserException(std::string("Expected \',\', instead got \'") + nextChar + "\'. Context: Method locals");
         }
      }
      std::stringstream buf2;
      // Parse local name
      for (; std::isalpha(input.peek()); buf2 << static_cast<char>(input.get()));
      std::string local = buf2.str();
      locals.push_back(local);
      numArgs++;
   }
   skipWhitespace(input);
   advanceAndExpectChar(input, ':', "Missing closing colon for method");
   skipWhitespace(input);
   advanceAndExpectChar(input, '\n', "Missing newline at end of method definition");
   // Parse all statements
   std::vector<std::shared_ptr<ASTStatement>> statements;
   while (true) {
      skipWhitespaceAndNewlines(input);
      // Check if end of class
      char nextChar = input.peek();
      if (nextChar == ']' || nextChar == EOF) {
         break;
      }
      size_t offset;
      // Dumb hack to figure out where a method ends
      std::string endOfMethod = "method ";
      std::stringstream buf2;
      bool equalsEndOfMethod = true;
      // Verify that statement does not start with "method "
      for (offset=0; offset<endOfMethod.length(); offset++) {
         if (input.peek() != endOfMethod[offset]) {
            equalsEndOfMethod = false;
            break;
         }
      }
      // Got keyword "method" followed by space
      if (equalsEndOfMethod) {
         // Skip additional whitespace
         offset += skipWhitespace(input);
         // Check if we have =, if so it is a statement, otherwise it is the end of the method
         if (input.peek() == '=') {
            equalsEndOfMethod = false;
         }
         // Assume new method otherwise
      }
      // Move input pointer back
      input.seekg(-offset, std::ios::cur);
      if (equalsEndOfMethod) {
         break;
      }
      // Arrived at first statement, parse it
      ASTStatement * statement = parseStmt(input);
      statements.push_back(std::shared_ptr<ASTStatement>(statement));
      // Statement will not parse ending whitespace, so parse here
      skipWhitespace(input);
      // Assert newline
      advanceAndExpectChar(input, '\n', "Missing newline at end of statement");
   }
   if (statements.size() == 0) {
      throw ParserException(std::string("Method cannot be empty"));
   }
   return new MethodDeclaration(methodname, args, locals, statements); 
}

ClassDeclaration * ProgramParser::parseClass(std::istream & input) {
   advanceAndExpectWord(input, "class", "Class must start with \"class\"");
   skipWhitespace(input);
   std::stringstream buf;
   // Parse class name
   for (; std::isupper(input.peek()); buf << static_cast<char>(input.get()));
   if (buf.str().length() == 0) {
      throw ParserException("invalid class name");
   }
   std::string classname = buf.str();
   skipWhitespace(input);
   advanceAndExpectChar(input, '[', "Class missing opening brace");
   skipWhitespace(input);
   advanceAndExpectChar(input, '\n', "Class missing opening newline");
   skipWhitespaceAndNewlines(input);
   std::vector<std::string> fields;
   if (input.peek() == 'f') {
      // Parse fields
      advanceAndExpectWord(input, "fields", "Class expected \"fields\" but got something else");
      advanceAndExpectChar(input, ' ', "Class missing space after \"fields\"");
      int numArgs = 0;
      while (true) {
         skipWhitespace(input);
         char nextChar = input.peek();
         if (nextChar == ';' || nextChar == '\n') {
            input.ignore();
            // Done parsing locals
            break;
         }
         if (numArgs > 0) {
            if (nextChar == ',') {
               // Skip ','
               input.ignore();
               skipWhitespace(input);
            } else {
               throw ParserException(std::string("Expected \',\', instead got \'") + nextChar + "\'. Context: class fields");
            }
         }
         std::stringstream buf2;
         // Parse field name
         for (; std::isalpha(input.peek()); buf2 << static_cast<char>(input.get()));
         std::string field = buf2.str();
         fields.push_back(field);
         numArgs++;
      }
   }
   std::vector<std::shared_ptr<MethodDeclaration>> methods;
   while (true) {
      skipWhitespaceAndNewlines(input);
      // Parsing methods
      if (input.peek() == ']') {
         input.ignore();
         break;
      }
      MethodDeclaration * method = parseMethod(input);
      methods.push_back(std::unique_ptr<MethodDeclaration>(method));
   }
   return new ClassDeclaration(classname, fields, methods);
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
      for (; std::isalpha(input.peek()); buf << static_cast<char>(input.get()));
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

