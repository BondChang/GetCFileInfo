#pragma once
#include "clang/AST/Expr.h"
#include "clang/AST/StmtVisitor.h"
#include "clang/Parse/Parser.h"
#include <iostream>
#include <stdio.h>
#include <sstream>
#include <limits>
#include <string>
#include <cassert>
using namespace std;
using namespace clang;
std::string parserInitExpr(const Stmt *stmt);
std::string deleteUnnecessarySign(std::string exprStr);
std::string dbl2str(double d);