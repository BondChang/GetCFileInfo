#pragma once
#include "CInfoUtil.h"
#include "tinyxml/include/tinyxml/tinyxml.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include <cassert>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdio.h>
#include <string>

using namespace clang;
using namespace clang::ast_matchers;
bool isExistedNodePath(SourceLocation sourceLoc, const MatchFinder::MatchResult &Result, CFileInfoStruct *cfile);
bool isSkipppingSituation(VarDecl const *varDeclNode, const MatchFinder::MatchResult &Result);
int dealVarDeclNode(VarDecl const *varDeclNode, const MatchFinder::MatchResult &Result, int varIdx, CFileInfoStruct *cfile);
int dealVarRecordDeclNode(RecordDecl const *varRecordDeclNode, const MatchFinder::MatchResult &Result, int structIdx, CFileInfoStruct *cfile);
int dealFunctionDeclNode(FunctionDecl const *functionDeclNode, const MatchFinder::MatchResult &Result, int funIdx, CFileInfoStruct *cfile);
void dealComplexValue(TiXmlElement *initValueAst, InitAstStruct startValue);
