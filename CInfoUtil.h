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

/*������Ϣ�ṹ��*/
struct VariableInfoStruct {
  char *name;      /*��������*/
  char *qualifier; /*�������η�*/
  char *type;      /*��������*/
  char *dim;       /*ά��*/
  char *initValue; /*��ֵ*/
  char *path;
  char *hPath;
  int pointerType;
  std::vector<InitAstStruct> initAstStruct;
  NodeLoc nodeLocInfo;
};

/*���Ͷ���ṹ��*/
struct StructInfo {
  char *name; /*�ṹ������*/
  char *type;
  int fieldCount;  /*����Ϣ�б����*/
  char *qualifier; /*�������η�*/
  char *path;
  std::vector<VariableInfoStruct> fields; /*ȫ�ֱ�����Ϣ�б�*/
  NodeLoc nodeLocInfo;
};

/*������Ϣ�ṹ��*/
struct FunctionInfoStruct {
  char *name; /*��������*/
  char *type; /*������������*/
  char *path;
  int paraCount;                         /*��������*/
  char *hPath;                           /* �����������ļ�·�� */
  std::vector<VariableInfoStruct> paras; /*���������б�*/
  NodeLoc nodeLocInfo;
};

/*C�ļ���Ϣ�ṹ��*/
struct CFileInfoStruct {
  const char *path;
  int funCount;                               /*������Ϣ�б����*/
  std::vector<FunctionInfoStruct> functions;  /*������Ϣ�б�*/
  int globalVarsCount;                        /*ȫ�ֱ�����Ϣ�б����*/
  std::vector<VariableInfoStruct> globalVars; /*ȫ�ֱ�����Ϣ�б�*/
  int structCount;                            /*ȫ�ֽṹ���б����*/
  std::vector<StructInfo> structs;            /*ȫ�ֽṹ���б�*/
  std::vector<std::string> everyFileExistPath;
};

/* �����ṹ�� */
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
