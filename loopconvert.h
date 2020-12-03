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
/*������Ϣ�ṹ��*/
struct VariableInfoStruct {
     char* name; /*��������*/
     char* qualifier;/*�������η�*/
     char* type;  /*��������*/
     char *dim;  /*ά��*/
     char *initValue;   /*ά��*/
	 char* path;
	 char* hPath;
};

/*���Ͷ���ṹ��*/
struct StructInfo {
    char* name;          /*�ṹ������*/
	char* type;
    int fieldCount;             /*����Ϣ�б����*/
    char* qualifier;/*�������η�*/
	char* path;
	std::vector<VariableInfoStruct> fields;/*ȫ�ֱ�����Ϣ�б�*/
};

/*������Ϣ�ṹ��*/
struct FunctionInfoStruct {
    char* name;           /*��������*/
    char* type;           /*������������*/
	char* path;
    int paraCount;             /*��������*/
	char* hPath;       /* �����������ļ�·�� */
	std::vector<VariableInfoStruct> paras;/*���������б�*/
};

/*����������Ϣ�б� */
struct FileErrorInfo {
  const char *errorLoc;  /*����λ��*/
  const char *errorInfo; /*������Ϣ*/
};

/*C�ļ���Ϣ�ṹ��*/
struct CFileInfoStruct {
    const char *path;
    int funCount;                   /*������Ϣ�б����*/
	std::vector<FunctionInfoStruct> functions;/*������Ϣ�б�*/
    int globalVarsCount;            /*ȫ�ֱ�����Ϣ�б����*/
	std::vector<VariableInfoStruct> globalVars; /*ȫ�ֱ�����Ϣ�б�*/
    int structCount;                /*ȫ�ֽṹ���б����*/
	std::vector<StructInfo> structs;/*ȫ�ֽṹ���б�*/
    int errorCount;  /* �������� */
	std::vector<FileErrorInfo> fileErrors; /*������Ϣ*/
	std::vector<std::string> everyFileExistPath;
};

/* �����ṹ�� */
struct AnonymousStruct {
	std::string name;
	std::string path;
	int row;
	int col;
};





/* ��ȡc�ļ�����Ϣ */
extern "C" __declspec(dllexport) void getCFileInfo(const char *filePath,
                                                   int argc, const char **argv);
extern "C" __declspec(dllexport) void writeCOrHInfo2Xml(const char **filePathList, int fileCount, int argc,const char **argv, const char *writePath, const char *targetPath);
#endif