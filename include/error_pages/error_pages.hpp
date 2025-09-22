#ifndef ERROR_PAGES_HPP
#define ERROR_PAGES_HPP

#include <iostream>
#include <string>
#include <map>

using namespace std;

// global map holding default HTML pages for common HTTP errors
extern map<int, string> defaultErrorPages;

// return the HTML content for a given HTTP error code
const string &getErrorPage(int code);

// populate defaultErrorPages with standard error pages
void initErrorPages();

#endif
