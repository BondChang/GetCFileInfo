#pragma once
#include "parserInitExpr.h"
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

struct NodeLoc {
  unsigned StartLine, StartCol, EndLine, EndCol;
};

struct InitAstStruct {
  std::string initValueAst;
  NodeLoc nodeLocInfo;
  bool isStructOrArrayBegin;
  bool isStructOrArrayEnd;
  std::vector<InitAstStruct> initAstStruct;
};

/*变量信息结构体*/
struct VariableInfoStruct {
  char *name;      /*变量名称*/
  char *qualifier; /*类型修饰符*/
  char *type;      /*变量类型*/
  char *dim;       /*维度*/
  char *initValue; /*初值*/
  char *path;
  char *hPath;
  int pointerType;
  std::vector<InitAstStruct> initAstStruct;
  NodeLoc nodeLocInfo;
};

/*类型定义结构体*/
struct StructInfo {
  char *name; /*结构体名称*/
  char *type;
  int fieldCount;  /*域信息列表个数*/
  char *qualifier; /*类型修饰符*/
  char *path;
  std::vector<VariableInfoStruct> fields; /*全局变量信息列表*/
  NodeLoc nodeLocInfo;
};

/*函数信息结构体*/
struct FunctionInfoStruct {
  char *name; /*函数名称*/
  char *type; /*函数返回类型*/
  char *path;
  int paraCount;                         /*参数个数*/
  char *hPath;                           /* 函数声明的文件路径 */
  std::vector<VariableInfoStruct> paras; /*函数参数列表*/
  NodeLoc nodeLocInfo;
};

/*C文件信息结构体*/
struct CFileInfoStruct {
  const char *path;
  int funCount;                               /*函数信息列表个数*/
  std::vector<FunctionInfoStruct> functions;  /*函数信息列表*/
  int globalVarsCount;                        /*全局变量信息列表个数*/
  std::vector<VariableInfoStruct> globalVars; /*全局变量信息列表*/
  int structCount;                            /*全局结构体列表个数*/
  std::vector<StructInfo> structs;            /*全局结构体列表*/
  std::vector<std::string> everyFileExistPath;
};

/* 匿名结构体 */
struct AnonymousStruct {
  std::string name;
  std::string path;
  int row;
  int col;
};

bool judgeIsInTargetFile(std::string fileName, std::string targetPath);
bool judgeIsInGlobalNames(std::string name);

std::string getNodePath(SourceLocation sourceLoc,
                        const MatchFinder::MatchResult &Result);
void dealRecordField(StructInfo *pStructInfo, RecordDecl::field_iterator &jt,
                     const MatchFinder::MatchResult &Result);

std::string getAnonymousName(RecordDecl const *varRecordDeclNode,
                             const MatchFinder::MatchResult &Result);
void dealType(std::string typeStr, QualType nodeType,
              VariableInfoStruct *variableInfoStruct);
void dealInitValue(const Expr *initExpr, std::string *initValueStr,
                   const MatchFinder::MatchResult &Result,
                   std::vector<InitAstStruct> *initAstStruct);
NodeLoc getLineNumInfo(SourceRange sourceRange, ASTContext *Context);

extern std::string basePath;
extern std::vector<std::string> allExistPath;
extern std::vector<std::string> globalNames;
extern std::vector<AnonymousStruct> allAnonymousStructs;
