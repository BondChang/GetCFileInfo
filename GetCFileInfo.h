#ifndef DLLPARSER_H
#define DLLPARSER_H
#include "clang/AST/Decl.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <string>

/* 获取c文件的信息 */
extern "C" __declspec(dllexport) void getCFileInfo(const char *filePath, int argc, const char **argv);
extern "C" __declspec(dllexport) void writeCOrHInfo2Xml(
    const char **filePathList, int fileCount, int argc, const char **argv,
    const char *targetPathDir, const char *writePath);
#endif