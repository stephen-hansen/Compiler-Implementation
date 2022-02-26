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

std::shared_ptr<ASTExpression> ProgramParser::parseExpr(std::istream & input) {
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
         return std::make_shared<UInt32Literal>(val);
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
      if (name.length() == 0) {
         throw ParserException("Invalid empty variable name");
      }
      // Build appropriate node
      if (name == "this") {
         return std::make_shared<ThisObjectExpression>();
      } else if (name == "null") {
         // Expect ":"
         if (input.peek() == ':') {
            advanceAndExpectChar(input, ':', "NullObjectExpression colon following null");
            // Parse class name
            std::stringstream classBuf;
            for (; std::isalpha(input.peek()); classBuf << static_cast<char>(input.get()));
            std::string className = classBuf.str();
            if (className.length() == 0) {
               throw ParserException("invalid named class for null expression");
            }
            // Build NullObject
            return std::make_shared<NullObjectExpression>(className);
         } else {
            // null as variable not as expression
            return std::make_shared<VariableIdentifier>(name);
         }
      } else {
         return std::make_shared<VariableIdentifier>(name);
      }
   } else if (firstChar == '(') {
      // Skip '('
      input.ignore();
      // Skip whitespace
      skipWhitespace(input);
      // Parse to arithmetic expression
      // Recursively get first operand
      std::shared_ptr<ASTExpression> e1 = parseExpr(input);
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
      std::shared_ptr<ASTExpression> e2 = parseExpr(input);
      // Skip whitespace
      skipWhitespace(input);
      // Verify closing paren
      advanceAndExpectChar(input, ')', "ArithmeticExpression closing parenthesis");
      // Build ArithmeticExpression
      return std::make_shared<ArithmeticExpression>(op, e1, e2);
   } else if (firstChar == '^') {
      // Skip '^'
      input.ignore();
      // Parse to method invocation
      // Parse object expression
      std::shared_ptr<ASTExpression> e = parseExpr(input);
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
         std::shared_ptr<ASTExpression> arg = parseExpr(input);
         args.push_back(arg);
         numArgs++;
      }
      skipWhitespace(input);
      advanceAndExpectChar(input, ')', "CallExpression closing parenthesis, or too many parameters");
      return std::make_shared<CallExpression>(e, methodName, args);
   } else if (firstChar == '&') {
      // Skip '&'
      input.ignore();
      // Parse to field read
      // Get object expression
      std::shared_ptr<ASTExpression> e = parseExpr(input);
      // Verify . following e
      advanceAndExpectChar(input, '.', "FieldReadExpression dot before field name");
      // Parse field name
      for (; std::isalnum(input.peek()); buf << static_cast<char>(input.get()));
      if (buf.str().length() == 0) {
         throw ParserException("invalid field name");
      }
      std::string fieldName = buf.str();
      return std::make_shared<FieldReadExpression>(e, fieldName);
   } else if (firstChar == '@') {
      // Skip '@'
      input.ignore();
      // Parse to new object
      // Parse class name
      for (; std::isupper(input.peek()); buf << static_cast<char>(input.get()));
      std::string className = buf.str();
      if (className.length() == 0) {
         throw ParserException("invalid class name");
      }
      return std::make_shared<NewObjectExpression>(className);
   } else {
      // If we get an unexpected char or EOF we will hit this
      throw ParserException(std::string("\'") + firstChar + "\' does not start a valid expression");
   }
}

std::shared_ptr<ASTStatement> ProgramParser::parseStmt(std::istream & input) {
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
      std::shared_ptr<ASTExpression> e = parseExpr(input);
      return std::make_shared<DontCareAssignmentStatement>(e);
   } else if (firstChar == '!') {
      // Skip '!'
      input.ignore();
      // Field update
      std::shared_ptr<ASTExpression> obj = parseExpr(input);
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
      std::shared_ptr<ASTExpression> val = parseExpr(input);
      return std::make_shared<FieldUpdateStatement>(obj, fieldName, val);
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
      std::shared_ptr<ASTExpression> e = parseExpr(input);
      skipWhitespace(input);
      advanceAndExpectChar(input, ')', "PrintStatement missing closing parenthesis");
      return std::make_shared<PrintStatement>(e);
   }
   skipWhitespace(input);
   if (input.peek() == '=') {
      // Assignment statement, keyword is varname
      // Validate keyword != this if in method body
      if (_inside_method_body && keyword == "this") {
         throw ParserException(std::string("Cannot write to this inside a method body"));
      }
      // Skip =
      input.ignore();
      skipWhitespace(input);
      // Parse expr
      std::shared_ptr<ASTExpression> e = parseExpr(input);
      return std::make_shared<AssignmentStatement>(keyword, e);
   }
   // Must be a keyword, and input is pointing at expression now
   if (keyword == "if") {
      std::shared_ptr<ASTExpression> cond = parseExpr(input);
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
         std::shared_ptr<ASTStatement> stmt = parseStmt(input);
         if_statements.push_back(stmt);
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
         std::shared_ptr<ASTStatement> stmt = parseStmt(input);
         else_statements.push_back(stmt);
         skipWhitespace(input);
         advanceAndExpectChar(input, '\n', "IfElseStatement missing newline between else statements");
      }
      if (else_statements.size() == 0) {
         throw ParserException(std::string("IfElseStatement cannot have 0 else statements"));
      }
      return std::make_shared<IfElseStatement>(cond, if_statements, else_statements);
   } else if (keyword == "ifonly") {
      std::shared_ptr<ASTExpression> cond = parseExpr(input);
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
         std::shared_ptr<ASTStatement> stmt = parseStmt(input);
         statements.push_back(stmt);
         skipWhitespace(input);
         advanceAndExpectChar(input, '\n', "IfOnlyStatement missing newline between statements");
      }
      if (statements.size() == 0) {
         throw ParserException(std::string("IfOnlyStatement cannot have 0 statements"));
      }
      return std::make_shared<IfOnlyStatement>(cond, statements);
   } else if (keyword == "while") {
      std::shared_ptr<ASTExpression> cond = parseExpr(input);
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
         std::shared_ptr<ASTStatement> stmt = parseStmt(input);
         statements.push_back(stmt);
         skipWhitespace(input);
         advanceAndExpectChar(input, '\n', "WhileStatement missing newline between statements");
      }
      if (statements.size() == 0) {
         throw ParserException(std::string("WhileStatement cannot have 0 statements"));
      }
      return std::make_shared<WhileStatement>(cond, statements);
   } else if (keyword == "return") {
      std::shared_ptr<ASTExpression> e = parseExpr(input);
      return std::make_shared<ReturnStatement>(e);
   } else if (keyword == "print") {
      std::shared_ptr<ASTExpression> e = parseExpr(input);
      return std::make_shared<PrintStatement>(e);
   }
   throw ParserException(std::string("Found statement starting with \"" + keyword + "\" which is not a valid keyword"));
}

std::shared_ptr<MethodDeclaration> ProgramParser::parseMethod(std::istream & input) {
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
   std::vector<std::pair<std::string, std::string>> args;
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
      if (arg.length() == 0) {
         throw ParserException("Method argument has zero length");
      }
      buf2.clear();
      buf2.str("");
      advanceAndExpectChar(input, ':', "Method parameter missing colon between variable and type");
      for (; std::isalpha(input.peek()); buf2 << static_cast<char>(input.get()));
      std::string className = buf2.str();
      if (className.length() == 0) {
         throw ParserException("invalid named type");
      }
      args.push_back(std::make_pair(arg, className));
      numArgs++;
   }
   skipWhitespace(input);
   advanceAndExpectChar(input, ')', "Method closing parenthesis, or too many parameters");
   advanceAndExpectChar(input, ' ', "Missing space after method params list");
   skipWhitespace(input);
   advanceAndExpectWord(input, "returning", "Missing returning after method signature");
   advanceAndExpectChar(input, ' ', "Missing space after returning in method signature");
   std::stringstream returnbuf;
   // Parse return type
   for (; std::isalpha(input.peek()); returnbuf << static_cast<char>(input.get()));
   std::string return_type = returnbuf.str();
   advanceAndExpectChar(input, ' ', "Missing space after return type");
   advanceAndExpectWord(input, "with", "Missing with after method signature");
   std::vector<std::pair<std::string, std::string>> locals;
   if (input.peek() != ':') {
      advanceAndExpectChar(input, ' ', "Missing space after with in method signature");
      skipWhitespace(input);
      advanceAndExpectWord(input, "locals", "Missing locals after method signature");
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
         if (local.length() == 0) {
            throw ParserException("Local variable has zero length");
         }
         buf2.clear();
         buf2.str("");
         advanceAndExpectChar(input, ':', "Local variable missing colon between variable and type");
         for (; std::isalpha(input.peek()); buf2 << static_cast<char>(input.get()));
         std::string className = buf2.str();
         if (className.length() == 0) {
            throw ParserException("invalid named type");
         }
         locals.push_back(std::make_pair(local, className));
         numArgs++;
      }
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
         buf2 << static_cast<char>(input.get());
      }
      // Got keyword "method" followed by space
      if (equalsEndOfMethod) {
         // Skip additional whitespace
         int skipped = skipWhitespace(input);
         for (int i=0; i<skipped; i++) {
            buf2 << " ";
            offset++;
         }
         // Check if we have =, if so it is a statement, otherwise it is the end of the method
         if (input.peek() == '=') {
            equalsEndOfMethod = false;
         }
         // Assume new method otherwise
      }
      // Move input data back
      std::string placeback = buf2.str();
      for (size_t i=0; i<offset; i++) {
         input.putback(placeback[offset-i-1]);
      }
      if (equalsEndOfMethod) {
         break;
      }
      // Arrived at first statement, parse it
      std::shared_ptr<ASTStatement> statement = parseStmt(input);
      statements.push_back(statement);
      // Statement will not parse ending whitespace, so parse here
      skipWhitespace(input);
      // Assert newline
      advanceAndExpectChar(input, '\n', "Missing newline at end of statement");
   }
   if (statements.size() == 0) {
      throw ParserException(std::string("Method cannot be empty"));
   }
   return std::make_shared<MethodDeclaration>(methodname, return_type, args, locals, statements); 
}

std::shared_ptr<ClassDeclaration> ProgramParser::parseClass(std::istream & input) {
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
   std::vector<std::pair<std::string, std::string>> fields;
   if (input.peek() == 'f') {
      // Parse fields
      advanceAndExpectWord(input, "fields", "Class expected \"fields\" but got something else");
      if (input.peek() != '\n') {
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
            if (field.length() == 0) {
               throw ParserException("field has zero length");
            }
            buf2.clear();
            buf2.str("");
            advanceAndExpectChar(input, ':', "Field name missing colon between variable and type");
            for (; std::isalpha(input.peek()); buf2 << static_cast<char>(input.get()));
            std::string className = buf2.str();
            if (className.length() == 0) {
               throw ParserException("invalid named type");
            }
            fields.push_back(std::make_pair(field, className));
            numArgs++;
         }
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
      _inside_method_body = true;
      std::shared_ptr<MethodDeclaration> method = parseMethod(input);
      methods.push_back(method);
   }
   return std::make_shared<ClassDeclaration>(classname, fields, methods);
}

std::shared_ptr<ProgramDeclaration> ProgramParser::parse(std::istream & input) {
   // Parse all classes
   char firstChar;
   std::map<std::string, std::shared_ptr<ClassDeclaration>> classes;
   while (true) {
      skipWhitespaceAndNewlines(input);
      firstChar = input.peek();
      if (firstChar == 'c') {
         // Assume it's a class, try to parse
         std::shared_ptr<ClassDeclaration> newClass = parseClass(input);
         std::string key = newClass->name();
         if (classes.find(key) != classes.end()) {
            throw ParserException("two classes with same class name, cannot statically type");
         }
         classes[key] = newClass;
         // Class will not parse ending whitespace, so parse here, assert newline
         skipWhitespace(input);
         advanceAndExpectChar(input, '\n', "Missing newline at end of class definition");
      } else {
         break;
      }
   }
   // Okay, we must be at main
   _inside_method_body = false;
   advanceAndExpectWord(input, "main", "Missing main program block");
   advanceAndExpectChar(input, ' ', "Program missing space after main");
   skipWhitespace(input);
   advanceAndExpectWord(input, "with", "Missing with after main");
   std::vector<std::pair<std::string, std::string>> main_locals;
   if (input.peek() != ':') {
      advanceAndExpectChar(input, ' ', "Program missing space after with");
      while (true) {
         skipWhitespace(input);
         std::stringstream buf;
         if (input.peek() == ':') {
            // Break early, no locals
            break;
         }
         for (; std::isalpha(input.peek()); buf << static_cast<char>(input.get()));
         std::string varname = buf.str();
         if (varname.length() == 0) {
            throw ParserException("main has invalid named local");
         }
         buf.clear();
         buf.str("");
         advanceAndExpectChar(input, ':', "Main local missing colon between variable and type");
         for (; std::isalpha(input.peek()); buf << static_cast<char>(input.get()));
         std::string className = buf.str();
         if (className.length() == 0) {
            throw ParserException("main has invalid named class");
         }
         main_locals.push_back(std::make_pair(varname, className));
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
      std::shared_ptr<ASTStatement> statement = parseStmt(input);
      statements.push_back(statement);
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

   return std::make_shared<ProgramDeclaration>(classes, main_locals, statements);
}

