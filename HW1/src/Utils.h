#ifndef _CS441_UTILS_H
#define _CS441_UTILS_H
#include <algorithm>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

void skipLeadingWhitespace(std::istream & input) {
   const std::string ignoreChars = " \t";
   while (ignoreChars.find(input.peek()) != std::string::npos) {
      input.ignore();
   }
}

#endif

