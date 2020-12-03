#ifndef DLLPARSER_H
#define DLLPARSER_H 
#include <iostream>
#include<string>
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "clang/AST/Decl.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "llvm/Support/CommandLine.h"
#include <fstream>
/*变量信息结构体*/
struct VariableInfoStruct {
     char* name; /*变量名称*/
     char* qualifier;/*类型修饰符*/
     char* type;  /*变量类型*/
     char *dim;  /*维度*/
     char *initValue;   /*维度*/
	 char* path;
	 char* hPath;
};

/*类型定义结构体*/
struct StructInfo {
    char* name;          /*结构体名称*/
	char* type;
    int fieldCount;             /*域信息列表个数*/
    char* qualifier;/*类型修饰符*/
	char* path;
	std::vector<VariableInfoStruct> fields;/*全局变量信息列表*/
};

/*函数信息结构体*/
struct FunctionInfoStruct {
    char* name;           /*函数名称*/
    char* type;           /*函数返回类型*/
	char* path;
    int paraCount;             /*参数个数*/
	char* hPath;       /* 函数声明的文件路径 */
	std::vector<VariableInfoStruct> paras;/*函数参数列表*/
};

/*函数错误信息列表 */
struct FileErrorInfo {
  const char *errorLoc;  /*错误位置*/
  const char *errorInfo; /*错误信息*/
};

/*C文件信息结构体*/
struct CFileInfoStruct {
    const char *path;
    int funCount;                   /*函数信息列表个数*/
	std::vector<FunctionInfoStruct> functions;/*函数信息列表*/
    int globalVarsCount;            /*全局变量信息列表个数*/
	std::vector<VariableInfoStruct> globalVars; /*全局变量信息列表*/
    int structCount;                /*全局结构体列表个数*/
	std::vector<StructInfo> structs;/*全局结构体列表*/
    int errorCount;  /* 错误数量 */
	std::vector<FileErrorInfo> fileErrors; /*错误信息*/
	std::vector<std::string> everyFileExistPath;
};

/* 匿名结构体 */
struct AnonymousStruct {
	std::string name;
	std::string path;
	int row;
	int col;
};





/* 获取c文件的信息 */
extern "C" __declspec(dllexport) void getCFileInfo(const char *filePath,
                                                   int argc, const char **argv);
extern "C" __declspec(dllexport) void writeCOrHInfo2Xml(const char **filePathList, int fileCount, int argc,const char **argv, const char *writePath, const char *targetPath);
#endif