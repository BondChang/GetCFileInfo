
#include "loopconvert.h"
#include "clang/Basic/SourceManager.h"
#include "../../../lib/Frontend/TextDiagnosticPrinter.cpp"
#include "tinyxml/include/tinyxml/tinyxml.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Sema/SemaInternal.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <string>
#include<algorithm>

using namespace clang::tooling;
using namespace llvm;
using namespace clang;
using namespace clang::ast_matchers;
using namespace std::filesystem;

// Apply a custom category to all command-line options so that they are the
// only ones displayed.
static llvm::cl::OptionCategory MyToolCategory("global-detect options");
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);
// A help message for this specific tool can be added afterwards.
static cl::extrahelp MoreHelp("\nMore help text...\n");
/* 记录嵌套结构体的下标 */
int varIndex = -1;
int structIndex = -1;
int funIndex = -1;
int paraIndex = -1;
int fieldCount = -1;
int errorCount = -1;
std::string dimStr;
int anonymousCount = 0;

CFileInfoStruct *cfileInfoAll;
CFileInfoStruct *cfileInfo;
std::string basePath;
std::vector<std::string> allExistPath;
std::vector<std::string> globalNames;
std::vector<AnonymousStruct> allAnonymousStructs;
/* 初始化结构体信息 */
// extern "C" __declspec(dllexport) void initStruct();
/* 处理全局变量信息 */
extern "C" __declspec(dllexport) void dealVarDeclNode(
	VarDecl const *varDeclNode, const MatchFinder::MatchResult &Result);

/* 处理结构体信息 */
extern "C" __declspec(dllexport) void dealVarRecordDeclNode(
	RecordDecl const *varRecordDeclNode,
	const MatchFinder::MatchResult &Result);
extern "C" __declspec(dllexport) bool isSkipppingSituation(VarDecl const *varDeclNode, const MatchFinder::MatchResult &Result);
/* 处理函数列表的信息 */
extern "C" __declspec(dllexport) void dealFunctionDeclNode(
	FunctionDecl const *functionDeclNode, const MatchFinder::MatchResult &Result);
extern "C" __declspec(dllexport) void addAnonymousInfo2Vector(SourceRange sourceRange, SourceLocation sourceLocation, const MatchFinder::MatchResult &Result,std::string fieldStructName);
extern "C" __declspec(dllexport) void getDefaultVaule(
	const ImplicitValueInitExpr *iV, std::string *defaultValueStr);
extern "C" __declspec(dllexport) bool judgeIsInGlobalNames(std::string name);
extern "C" __declspec(dllexport)  std::string getNodePath(SourceLocation sourceLoc, const MatchFinder::MatchResult &Result);
extern "C" __declspec(dllexport) void dealInitValue(const Expr *initExpr,
	std::string *initValueStr);
extern "C" __declspec(dllexport) bool isExistedNodePath(SourceLocation sourceLoc, const MatchFinder::MatchResult &Result);
extern "C" __declspec(dllexport) std::string getAnonymousName(RecordDecl const *varRecordDeclNode, const MatchFinder::MatchResult &Result);
class MyDiagnosticConsumer : public TextDiagnosticPrinter {
public:
	MyDiagnosticConsumer(raw_ostream &os, DiagnosticOptions *diags)
		: TextDiagnosticPrinter(os, diags) {}
	void HandleDiagnostic(DiagnosticsEngine::Level DiagLevel,
		const Diagnostic &Info) override {
		TextDiagnosticPrinter::HandleDiagnostic(DiagLevel, Info);
		const DiagnosticsEngine *diagnosticsEngine = Info.getDiags();
		auto diagnosticOptions = diagnosticsEngine->getDiagnosticOptions();
		SmallString<100> smallErrorStr;
		errorCount++;
		StringRef errorStrRef =
			Info.getDiags()->getDiagnosticIDs()->getDescription(Info.getID());
		Info.FormatDiagnostic(errorStrRef.begin(), errorStrRef.end(),
			smallErrorStr);
		std::string errorInfo = smallErrorStr.c_str();
		std::string errorLoc = Info.getLocation().printToString(Info.getSourceManager());
		FileErrorInfo fileErrorInfo;
		//cfileInfo->fileErrors = new std::vector<FileErrorInfo>;
		fileErrorInfo.errorInfo = strcpy(new char[errorInfo.length() + 1], errorInfo.data());
		fileErrorInfo.errorLoc = strcpy(new char[errorLoc.length() + 1], errorLoc.data());
		cfileInfo->fileErrors.push_back(fileErrorInfo);
	}
};
/* 输出获取的c文件信息 */
extern "C" __declspec(dllexport) void printCFileInfo();
extern "C" __declspec(dllexport)  bool judgeIsInTargetFile(std::string fileName, std::string targetPath);
class LoopPrinter : public MatchFinder::MatchCallback {
public:
	virtual void run(const MatchFinder::MatchResult &Result) {

		VarDecl const *varDeclNode = Result.Nodes.getNodeAs<VarDecl>("globalVars");
		RecordDecl const *varRecordDeclNode =
			Result.Nodes.getNodeAs<RecordDecl>("structs");

		FunctionDecl const *functionDeclNode =
			Result.Nodes.getNodeAs<FunctionDecl>("functionParas");

		RecordDecl const *varRecordDeclNodeUnion =
			Result.Nodes.getNodeAs<RecordDecl>("unions");

		/* 如果是全局变量,更新全局变量的个数并把全局变量的信息存在与结构体中 */
		if (varDeclNode) {
			bool isExistedPath = isExistedNodePath(varDeclNode->getLocation(), Result);
			if (!isExistedPath) {
				/* 判断是否是跳过的类型，已经解析过且现在文件是h文件 */
				bool isSkip= isSkipppingSituation(varDeclNode,Result);
				if (!isSkip) {
					dealVarDeclNode(varDeclNode, Result);
				}
			}
		}
		/* 如果是结构体,更新结构体个数并将结构体信息存于全局结构体列表中 */
		if (varRecordDeclNode) {
			bool isExistedPath = isExistedNodePath(varRecordDeclNode->getLocation(), Result);
			if (!isExistedPath) {
				dealVarRecordDeclNode(varRecordDeclNode, Result);
			}
		}
		/* 如果是联合体,更新结构体个数并将结构体信息存于全局结构体列表中 */
		if (varRecordDeclNodeUnion) {
			bool isExistedPath = isExistedNodePath(varRecordDeclNodeUnion->getLocation(), Result);
			if (!isExistedPath) {
				dealVarRecordDeclNode(varRecordDeclNodeUnion, Result);
			}
		}

		/* 如果是函数列表,更新函数列表的个数并存储于函数信息列表中 */
		if (functionDeclNode) {
			bool isExistedPath = isExistedNodePath(functionDeclNode->getLocation(), Result);
			if (!isExistedPath) {
				dealFunctionDeclNode(functionDeclNode, Result);
			}
		}
	}
};

bool isSkipppingSituation(VarDecl const *varDeclNode,const MatchFinder::MatchResult &Result) {
  bool isInGlobalNames = judgeIsInGlobalNames(varDeclNode->getNameAsString());
  if (isInGlobalNames) {
    std::string fileName = getNodePath(varDeclNode->getLocation(), Result);
    if (!fileName.empty()) {
      std::string suffixStr = fileName.substr(fileName.find_last_of('.') + 1);
	  if (suffixStr.length() == 1) {
		  char arr[2];
		  for (int x = 0; x < sizeof(arr); x++) {
			  arr[x] = suffixStr[x];
		  }
		  if (strcmp(arr, "h") == 0) {
			  return true;
		  }
	  }
    }
  } else {
    globalNames.push_back(varDeclNode->getNameAsString());
	
  }
  return false;
}

std::string getNodePath(SourceLocation sourceLoc,
                        const MatchFinder::MatchResult &Result) {
	SourceManager &sourceManager = Result.Context->getSourceManager();
	std::string fileName = sourceManager.getFilename(sourceLoc).str();
	if (!fileName.empty()) {
		std::replace(fileName.begin(), fileName.end(), '\\', '/');
		bool isInTargetFile =judgeIsInTargetFile(fileName, basePath);
		/* 说明是target的子文件夹 */
		if (isInTargetFile) {
			auto relativePath = std::filesystem::path(fileName).lexically_relative(basePath);
			fileName = relativePath.string();
		}
	}
	return fileName;
}


bool judgeIsInTargetFile(std::string fileName, std::string targetPath) {
	bool isInTargetFile = false;
	std::filesystem::recursive_directory_iterator itEnd;
	for (std::filesystem::recursive_directory_iterator itor("C:\\Users\\bondc\\Desktop\\target"); itor != itEnd; ++itor)
	{
		std::string subFiles = itor->path().string();
		std::replace(subFiles.begin(), subFiles.end(), '\\', '/');
		if (strcmp(fileName.c_str(), subFiles.c_str()) == 0) {
			isInTargetFile= true;
			break;
		}
	}
	return isInTargetFile;
}
/* 获取节点的起始列 */
int getNodeCol(SourceRange sourceRange, SourceManager &sourceManager) {
	PresumedLoc locStart = sourceManager.getPresumedLoc(sourceRange.getBegin());
	//PresumedLoc locEnd = sourceManager.getPresumedLoc(sourceRange.getEnd());
	/*anonymousStruct->row = locStart.getLine();
	anonymousStruct->col = locStart.getColumn();*/
	return locStart.getColumn();
}

/* 获取节点的起始行 */
int getNodeRow(SourceRange sourceRange, SourceManager &sourceManager) {
	PresumedLoc locStart = sourceManager.getPresumedLoc(sourceRange.getBegin());
	//PresumedLoc locEnd = sourceManager.getPresumedLoc(sourceRange.getEnd());
	/*anonymousStruct->row = locStart.getLine();
	anonymousStruct->col = locStart.getColumn();*/
	return locStart.getLine();
}

bool isExistedNodePath(SourceLocation sourceLoc, const MatchFinder::MatchResult &Result) {
	SourceManager &sourceManager = Result.Context->getSourceManager();
	std::string path = sourceManager.getFilename(sourceLoc).str().c_str();
	if (!path.empty()) {
		std::replace(path.begin(), path.end(), '\\', '/');
		bool isInTargetFile = judgeIsInTargetFile(path, basePath);
		/* 说明是target的子文件夹 */
		if (isInTargetFile) {
			auto relativePath = std::filesystem::path(path).lexically_relative(basePath);
			path = relativePath.string();
		}
		cfileInfo->everyFileExistPath.push_back(path);
	}
	std::vector<std::string>::iterator ret;
	ret = std::find(allExistPath.begin(), allExistPath.end(), path);
	if (ret == allExistPath.end()) {

		return false;
	}
	else {
		/* 说明已经解析过此路径，不应该在解析 */
		//std::cout << path << "已经解析" << "\n";
		return true;
	}
}

/* 初始化数组信息 */
// void initStruct() {
// cfileInfo->globalVars = new VariableInfoStruct[100];
// cfileInfo->structs = new StructInfo[100];
// cfileInfo->functions = new FunctionInfoStruct[100];
//}

QualType dealPointerType(const Type *pointerType, std::string &pointer) {

	const Type *childType = pointerType->getPointeeType()
		.getLocalUnqualifiedType()
		.getCanonicalType()
		.getTypePtr();
	/* 如果子表达式仍为指针类型 */
	if (childType->isPointerType()) {
		pointer = pointer + "[]";
		return dealPointerType(childType, pointer);
	}
	/* 如果不为指针类型 */
	else {
		QualType type = pointerType->getPointeeType();
		return type;
	}
}

QualType dealArrayType(const Type *type) {

	/* 转换成数组类型 */
	const ArrayType *arrayType = type->castAsArrayTypeUnsafe();
	/* 获取数组的维度 */

	/* 获取数组的类型 */
	const Type *childType = arrayType->getElementType()
		.getLocalUnqualifiedType()
		.getCanonicalType()
		.getTypePtr();
	/* 数组类型为ConstantArrayType()，如int[3] */
	/* 数组类型为IncompleteArrayType，如int[] */
	if (childType->isConstantArrayType() || childType->isIncompleteArrayType()) {
		return dealArrayType(childType);
	}
	else {
		QualType qualType = arrayType->getElementType()
			.getLocalUnqualifiedType()
			.getCanonicalType();
		return qualType;
	}
}

std::string dealDim(const Type *type, std::string dimStr) {

	/* 转换成数组类型 */
	const ArrayType *arrayType = type->castAsArrayTypeUnsafe();
	/* 处理ConstantArrayType类型，获取数组的维度 */
	if (arrayType->isConstantArrayType()) {
		int dim =
			dyn_cast<ConstantArrayType>(arrayType)->getSize().getLimitedValue();
		dimStr += std::to_string(dim);
		dimStr += "]";
	}
	/* 处理IncompleteArrayType类型 */
	else if (arrayType->isIncompleteArrayType()) {
		dimStr += "]";
	}

	/* 获取数组的类型 */
	const Type *childType = arrayType->getElementType().getTypePtr();

	/* 如果是ConstantArrayType或者IncompleteArrayType类型的数组 */
	if (childType->isConstantArrayType() || childType->isIncompleteArrayType()) {
		dimStr += "[";
		return dealDim(childType, dimStr);
	}
	else {
		return dimStr;
	}
}
/* 处理修饰符 */
void dealQualifiers(QualType nodeType, VariableInfoStruct *variableInfoStruct,
	std::string typeStr) {
	std::string qualifier = nodeType.getQualifiers().getAsString();
	variableInfoStruct->qualifier =
		strcpy(new char[qualifier.length() + 1], qualifier.data());
}
/* 处理类型定义 */
void dealType(const Type *type, std::string typeStr,
	VariableInfoStruct *variableInfoStruct, QualType nodeType) {

	/* 说明有类型修饰符 */
	if (nodeType.hasQualifiers()) {
		dealQualifiers(nodeType, variableInfoStruct, typeStr);
	}
	/* 若无修饰符则置为空 */
	else {
		std::string qualifier = "";
		variableInfoStruct->qualifier =
			strcpy(new char[qualifier.length() + 1], qualifier.data());
	}

	/* 处理类型为ConstantArrayType的数组，如int[3] */
	/* 处理IncompleteArrayType类型的数组,如int[] */
	if (type->isConstantArrayType() || type->isIncompleteArrayType()) {
		/* 处理数组的类型 */
		QualType qualType = dealArrayType(type);
		typeStr = qualType.getAsString();
		/* 处理数组的维度 */
		std::string dimStr = "[";
		dimStr = dealDim(type, dimStr);
		type = qualType.getTypePtr();
		variableInfoStruct->type =
			strcpy(new char[typeStr.length() + 1], typeStr.data());
		if (type->isPointerType()) {
			dealType(type, typeStr, variableInfoStruct, nodeType);
		}
		variableInfoStruct->dim =
			strcpy(new char[dimStr.length() + 1], dimStr.data());
	}
	/* 如果是指针类型 */
	else if (type->isPointerType()) {
		/* 处理指针类型的* */
		std::string pointer = "[]";
		/* 获取除*以外的类型 */
		QualType pointType = dealPointerType(type, pointer);
		typeStr = pointType.getAsString();
		/* 如果像包括const，volatile修饰符 */
		if (pointType.hasQualifiers()) {
			dealQualifiers(pointType, variableInfoStruct, typeStr);
		}
		typeStr =
			pointType.getLocalUnqualifiedType().getCanonicalType().getAsString();
		/* 类型加指针 */
		typeStr = typeStr;
		variableInfoStruct->type =
			strcpy(new char[typeStr.length() + 1], typeStr.data());
		std::string dimStr = pointer;
		variableInfoStruct->dim =
			strcpy(new char[dimStr.length() + 1], dimStr.data());
	}

	/* 如果不是数组 */
	else {
		variableInfoStruct->type =
			strcpy(new char[typeStr.length() + 1], typeStr.data());
		std::string dimStr = "";
		variableInfoStruct->dim =
			strcpy(new char[dimStr.length() + 1], dimStr.data());
	}
}
/* 处理全局变量信息 */
void dealVarDeclNode(VarDecl const *varDeclNode,
	const MatchFinder::MatchResult &Result) {
		varIndex++;
		VariableInfoStruct variableInfoStruct;
		/* 获取变量名 */
		std::string nameStr = varDeclNode->getNameAsString();
		std::string initValueStr = "";
		std::string fileName = getNodePath(varDeclNode->getLocation(), Result);
		auto firstDeclNode=varDeclNode->getFirstDecl();
		std::string firstDeclPath=getNodePath(firstDeclNode->getLocation(), Result);
		variableInfoStruct.hPath= strcpy(new char[firstDeclPath.length() + 1], firstDeclPath.data());
		variableInfoStruct.path = strcpy(new char[fileName.length() + 1], fileName.data());
		variableInfoStruct.name = strcpy(new char[nameStr.length() + 1], nameStr.data());
		/* 获取变量类型,不带类型信息 */
		std::string typeStr = varDeclNode->getType()
			.getLocalUnqualifiedType()
			.getCanonicalType()
			.getAsString();
		QualType nodeType = varDeclNode->getType();
		// std::cout << varDeclNode->getType().getTypePtr()->getTypeClassName();
		const Type *type = varDeclNode->getType().getTypePtr();
		auto name = clang::QualType(type, 0).getAsString();
		// std::cout << nodeType.getCanonicalType().getAsString() << "\n";
		dealType(type, typeStr, &variableInfoStruct, nodeType);
		/* 获取初始化器 */
		const Expr *initExpr = varDeclNode->getAnyInitializer();
		/* 处理全局变量的初值 */
		dealInitValue(initExpr, &initValueStr);
		variableInfoStruct.initValue = strcpy(new char[initValueStr.length() + 1], initValueStr.data());
		cfileInfo->globalVars.push_back(variableInfoStruct);
}

/* 处理全局变量的初值 */
void dealInitValue(const Expr *initExpr, std::string *initValueStr) {
	if (!(initExpr == NULL)) {
		/* double或float类型的初值 */
		if (isa<FloatingLiteral>(initExpr)) {
			const FloatingLiteral *fL = dyn_cast<FloatingLiteral>(initExpr);
			double initValue = fL->getValueAsApproximateDouble();
			/* 将float/double强转为String */
			*initValueStr += std::to_string(initValue);
		}
		/* int类型的value */
		if (isa<IntegerLiteral>(initExpr)) {
			const IntegerLiteral *iL = dyn_cast<IntegerLiteral>(initExpr);
			/* 将int强转为String */
			*initValueStr += std::to_string(iL->getValue().getSExtValue());
		}
		if (isa<ImplicitCastExpr>(initExpr)) {
			const ImplicitCastExpr *iCE = dyn_cast<ImplicitCastExpr>(initExpr);
			auto subExpr = iCE->getSubExpr();
			dealInitValue(subExpr, initValueStr);
		}
		/* 没有值的情况 */
		if (isa<ImplicitValueInitExpr>(initExpr)) {
			std::string defaultValueStr = "";
			const ImplicitValueInitExpr *iV =
				dyn_cast<ImplicitValueInitExpr>(initExpr);
			/* 将int强转为String */
			getDefaultVaule(iV, &defaultValueStr);
			*initValueStr += defaultValueStr;
		}
		/* 数组 */
		if (isa<InitListExpr>(initExpr)) {
			const InitListExpr *iLE = dyn_cast<InitListExpr>(initExpr);
			SourceLocation Loc = iLE->getSourceRange().getBegin();
			signed NumInits = iLE->getNumInits();
			// const Expr *temp = iLE->getInit(0);
			QualType initExprType = initExpr->getType();
			if (initExprType->isStructureType()) {
				*initValueStr += "{";
			}
			else {
				*initValueStr += "[";
			}

			for (int i = 0; i < NumInits; i++) {
				dealInitValue(iLE->getInit(i), initValueStr);
				*initValueStr += ",";
			}
			int num = initValueStr->length();
			if (num > 0) {
				*initValueStr = initValueStr->substr(0, num - 1);
			}
			if (initExprType->isStructureType()) {
				*initValueStr += "}";
			}
			else {
				*initValueStr += "]";
			}
		}
	}
}
/* 判断是否是已经记录的全局变量 */
bool judgeIsInGlobalNames(std::string name) {
	std::vector<std::string>::iterator ret;
	ret = std::find(globalNames.begin(), globalNames.end(), name);
	if (ret == globalNames.end()) {
		return false;
	}
	else {
		return true;
	}
}
/* 设置默认值 */
void getDefaultVaule(const ImplicitValueInitExpr *iV,
	std::string *defaultValueStr) {
	QualType ivType = iV->getType();
	/* double与float */
	if (ivType->isConstantArrayType() || ivType->isIncompleteArrayType()) {
		*defaultValueStr += "[" + std::to_string(0) + "]";
	}
	/* int */
	else if (ivType->isStructureType()) {
		*defaultValueStr += "{" + std::to_string(0) + "}";
	}
	else {
		*defaultValueStr += std::to_string(0);
	}
}

/* 处理结构体信息 */
void dealVarRecordDeclNode(RecordDecl const *varRecordDeclNode,
	const MatchFinder::MatchResult &Result) {
	if (varRecordDeclNode->isCanonicalDecl()) {
		/* 说明结构体没有成员 */
		if (varRecordDeclNode->field_begin() == varRecordDeclNode->field_end()) {

		}
		else {
			structIndex++;
			fieldCount = -1;
			std::string nameStr = "";
			std::string qualifier = "";
			/* 如果是匿名结构体,则获取匿名结构体的名称 */
			std::string anonymousName= getAnonymousName(varRecordDeclNode,Result);
			if (!anonymousName.empty()) {
				nameStr = anonymousName;
			}
			if (nameStr.empty()&&varRecordDeclNode->getTypedefNameForAnonDecl()) {
				/* 获取结构体的typedef名称 */
				nameStr = varRecordDeclNode->getTypedefNameForAnonDecl()->getNameAsString();
			}
			if (nameStr.empty()) {
				/* 直接获取结构体的名字 */
				nameStr = varRecordDeclNode->getNameAsString();
			}
			else {
				qualifier = "typedef";
			}
			StructInfo structInfo;
			std::string structType = "";
			if (varRecordDeclNode->isStruct()) {
				structType = "struct";
			}
			else {
				structType = "union";
			}
			structInfo.type = strcpy(new char[structType.length() + 1], structType.data());
			std::string fileName = getNodePath(varRecordDeclNode->getLocation(), Result);
			structInfo.path = strcpy(new char[fileName.length() + 1], fileName.data());
			structInfo.name = strcpy(new char[nameStr.length() + 1], nameStr.data());
			structInfo.qualifier = strcpy(new char[qualifier.length() + 1], qualifier.data());
			/* 记录结构体的field数量 */
			RecordDecl::field_iterator jt;
			for (jt = varRecordDeclNode->field_begin();
				jt != varRecordDeclNode->field_end(); ++jt) {
				fieldCount++;
				VariableInfoStruct variableInfoStruct;
				std::string nameStr = jt->getNameAsString();
				/* 获取全局变量的类型，为总类型 */
				std::string typeStr = jt->getType().getLocalUnqualifiedType().getCanonicalType().getAsString();
				variableInfoStruct.name = strcpy(new char[nameStr.length() + 1], nameStr.data());
				/* 将全局变量的类型转成Type类型,便于后续判断是否为数组 */
				const Type *type = jt->getType().getTypePtr();
				QualType nodeType = jt->getType();
				dealType(type, typeStr, &variableInfoStruct, nodeType);
				/* 处理匿名结构体 */
				if (jt->getType().getTypePtr()->isStructureType()) {
					std::string type = "anonymous";
					type.append(std::to_string(anonymousCount++));
					variableInfoStruct.type = strcpy(new char[type.length() + 1], type.data());
					
					addAnonymousInfo2Vector(jt->getSourceRange(), jt->getLocation(), Result,type);
				}
				structInfo.fields.push_back(variableInfoStruct);
			}
			structInfo.fieldCount = fieldCount + 1;
			cfileInfo->structs.push_back(structInfo);
		}
	}
}
std::string getAnonymousName(RecordDecl const *varRecordDeclNode, const MatchFinder::MatchResult &Result) {
	std::string nodePath=getNodePath(varRecordDeclNode->getLocation(), Result);
	SourceManager &sourceManager = Result.Context->getSourceManager();
	int nodeRow=getNodeRow(varRecordDeclNode->getSourceRange(), sourceManager);
	int nodeCol=getNodeCol(varRecordDeclNode->getSourceRange(), sourceManager);
	if (allAnonymousStructs.size() > 0) {
		for (auto iter = allAnonymousStructs.begin(); iter != allAnonymousStructs.end(); iter++)
		{
			if ((strcmp(nodePath.c_str(), (*iter).path.c_str()) == 0) && (nodeRow == (*iter).row) && (nodeCol == (*iter).col)) {
				return (*iter).name;
			}
			
		}
	 }
	return "";
}
void addAnonymousInfo2Vector(SourceRange sourceRange,SourceLocation sourceLocation, const MatchFinder::MatchResult &Result,std::string fieldStructName) {
	AnonymousStruct anonymousStruct;
	anonymousStruct.path=getNodePath(sourceLocation, Result);
	SourceManager &sourceManager = Result.Context->getSourceManager();
	anonymousStruct.row = getNodeRow(sourceRange, sourceManager);
	anonymousStruct.col = getNodeCol(sourceRange, sourceManager);
	anonymousStruct.name = fieldStructName;
	allAnonymousStructs.push_back(anonymousStruct);
}

/* 处理函数列表信息 */
void dealFunctionDeclNode(FunctionDecl const *functionDeclNode, const MatchFinder::MatchResult &Result) {
	if (functionDeclNode->doesThisDeclarationHaveABody()) {
		funIndex++;
		const FunctionDecl * firstDeclNode=functionDeclNode->getFirstDecl();
		std::string firstDeclFilePath =getNodePath(firstDeclNode->getLocation(), Result);
		FunctionInfoStruct functionInfoStruct;
		std::string fileName = getNodePath(functionDeclNode->getLocation(), Result);
		functionInfoStruct.path = strcpy(new char[fileName.length() + 1], fileName.data());
		functionInfoStruct.hPath = strcpy(new char[firstDeclFilePath.length() + 1], firstDeclFilePath.data());
		std::string nameStr = functionDeclNode->getNameAsString();
		std::string typeStr = functionDeclNode->getReturnType().getAsString();
		functionInfoStruct.name = strcpy(new char[nameStr.length() + 1], nameStr.data());
		functionInfoStruct.type = strcpy(new char[typeStr.length() + 1], typeStr.data());
		functionInfoStruct.paraCount = functionDeclNode->getNumParams();
		int paraNum = functionDeclNode->getNumParams();
		if (paraNum != 0) {
			for (int i = 0; i < paraNum; i++) {
				VariableInfoStruct variableInfoStruct;
				std::string nameStr = functionDeclNode->getParamDecl(i)->getNameAsString();
				variableInfoStruct.name = strcpy(new char[nameStr.length() + 1], nameStr.data());
				/* 获取函数参数的类型 */
				std::string typeStr = functionDeclNode->getParamDecl(i)
					->getType()
					.getLocalUnqualifiedType()
					.getCanonicalType()
					.getAsString();
				/* 将函数参数的类型转换为Type,方便后续判断是否为数组类型 */
				const Type *type =
					functionDeclNode->getParamDecl(i)->getType().getTypePtr();

				// std::cout << type->getTypeClassName() << type->getTypeClass()<< "\n";
				/* 说明是Decayed类型,即本来是数组类型，被clang转换成指针类型 */
				QualType nodeType = functionDeclNode->getParamDecl(i)->getType();
				if (type->getTypeClass() == 8) {
					const DecayedType *DT = type->getAs<DecayedType>();
					// const ArrayType *ttt =
					// DT->getOriginalType().getTypePtr()->castAsArrayTypeUnsafe();
					// std::cout
					// << ttt->getTypeClassName() << "\n"; std::cout <<
					// DT->getOriginalType().getAsString()<< "\n";
					// std::cout << DT->getTypeClassName() << "\n";
					type = DT->getOriginalType().getTypePtr();
					nodeType = DT->getOriginalType();
				}
				dealType(type, typeStr, &variableInfoStruct,
					nodeType);
				functionInfoStruct.paras.push_back(variableInfoStruct);
			}
		}
		cfileInfo->functions.push_back(functionInfoStruct);
	}

}

void printCFileInfo() {
	/* 输出所有的全局变量信息 */
	for (int i = 0; i <= varIndex; i++) {
		std::cout << "全局变量的名称是 " << cfileInfo->globalVars[i].name << "\n";
		std::cout << "全局变量的修饰符是 " << cfileInfo->globalVars[i].qualifier
			<< "\n";
		std::cout << "全局变量的类型是 " << cfileInfo->globalVars[i].type << "\n";
		std::cout << "全局变量的维度是 " << cfileInfo->globalVars[i].dim << "\n";
		std::cout << "全局变量的初值是 " << cfileInfo->globalVars[i].initValue
			<< "\n";
	}
	/* 输出所有的结构体信息 */
	for (int i = 0; i <= structIndex; i++) {

		std::cout << "结构体的名称是 " << cfileInfo->structs[i].name << "\n";
		std::cout << "结构体成员变量的数量是 " << cfileInfo->structs[i].fieldCount
			<< "\n";
		std::cout << "结构体的修饰符是 " << cfileInfo->structs[i].qualifier << "\n";

		for (int j = 0; j < cfileInfo->structs[i].fieldCount; j++) {
			std::cout << "结构体成员变量的名称是 "
				<< cfileInfo->structs[i].fields[j].name << "\n";
			std::cout << "结构体成员变量的修饰符是 "
				<< cfileInfo->structs[i].fields[j].qualifier << "\n";
			std::cout << "结构体成员变量的类型是 "
				<< cfileInfo->structs[i].fields[j].type << "\n";

			std::cout << "结构体成员变量的维度是 "
				<< cfileInfo->structs[i].fields[j].dim << "\n";
		}
	}
	/* 输出所有的函数列表信息 */
	for (int i = 0; i <= funIndex; i++) {
		std::cout << "函数的名称为 " << cfileInfo->functions[i].name << "\n";
		std::cout << "函数的返回值类型为 " << cfileInfo->functions[i].type << "\n";
		std::cout << "函数的参数个数为 " << cfileInfo->functions[i].paraCount
			<< "\n";
		for (int j = 0; j < cfileInfo->functions[i].paraCount; j++) {
			std::cout << "函数的参数列表名称为 "
				<< cfileInfo->functions[i].paras[j].name << "\n";
			std::cout << "函数的参数列表修饰符为 "
				<< cfileInfo->functions[i].paras[j].qualifier << "\n";
			std::cout << "函数的参数列表类型为 "
				<< cfileInfo->functions[i].paras[j].type << "\n";
			std::cout << "函数的参数列表维度为 "
				<< cfileInfo->functions[i].paras[j].dim << "\n";
		}
	}

	for (int i = 0; i <= errorCount; i++) {
		/*std::cout << "错误的位置信息是 " << cfileInfo->fileErrors[i].errorLoc
				  << "\n";
		std::cout << "错误的原因是 " << cfileInfo->fileErrors[i].errorInfo << "\n";*/
	}
}

void writeCOrHInfo2Xml(const char **filePathList, int fileCount, int argc,
	const char **argv, const char *writePath, const char *targetPath) {
	cfileInfoAll = new CFileInfoStruct[fileCount];
	basePath = targetPath;
	//CFileInfoStruct *cfileInfo = new CFileInfoStruct[sizeof(fileCount)];
	for (int i = 0; i < fileCount; i++) {
		cfileInfo = &cfileInfoAll[i];
		getCFileInfo(filePathList[i], argc, argv);
		//std::cout << "正在解析第" << i << "个" << "\n";
		for (auto iter = cfileInfo->everyFileExistPath.begin(); iter != cfileInfo->everyFileExistPath.end(); iter++)
		{
			allExistPath.push_back((*iter));
		}
		/* 设置文件中错误的个数 */
		cfileInfo->errorCount = cfileInfo->fileErrors.size();
	}

	// write2xml
	using namespace std;
	const char *xmlFile = writePath;
	TiXmlDocument doc;
	TiXmlDeclaration *decl = new TiXmlDeclaration("1.0", "", "");
	TiXmlElement *resources = new TiXmlElement("CInfo");
	TiXmlElement *dictsElement = new TiXmlElement("dicts");
	TiXmlElement *structsElement = new TiXmlElement("structs");
	TiXmlElement *funsElement = new TiXmlElement("functions");
	TiXmlElement *errorsElement = new TiXmlElement("errors");
	for (int i = 0; i < fileCount; i++) {
		cfileInfo = &cfileInfoAll[i];
		/* 写入全局变量 */
		for (auto iter = cfileInfo->globalVars.begin(); iter != cfileInfo->globalVars.end(); iter++)
		{
			TiXmlElement *dictElement = new TiXmlElement("DictItem");
			dictElement->SetAttribute("name", (*iter).name);
			dictElement->SetAttribute("qualifier", (*iter).qualifier);
			dictElement->SetAttribute("type", (*iter).type);
			dictElement->SetAttribute("dim", (*iter).dim);
			dictElement->SetAttribute("path", (*iter).path);
			dictElement->SetAttribute("hPath", (*iter).hPath);
			dictElement->SetAttribute("initValue", (*iter).initValue);
			dictsElement->LinkEndChild(dictElement);
		}
	}

	/* 写入结构体 */
	for (int i = 0; i < fileCount; i++) {
		cfileInfo = &cfileInfoAll[i];
		for (auto iter = cfileInfo->structs.begin(); iter != cfileInfo->structs.end(); iter++) {
			TiXmlElement *structElement = new TiXmlElement("StructItem");
			structElement->SetAttribute("name", (*iter).name);
			structElement->SetAttribute("type", (*iter).type);
			structElement->SetAttribute("qualifier", (*iter).qualifier);
			structElement->SetAttribute("path", (*iter).path);
			TiXmlElement *structItems = new TiXmlElement("children");
			for (auto subIter = (*iter).fields.begin(); subIter != (*iter).fields.end(); subIter++) {
				TiXmlElement *structItem = new TiXmlElement("StructItem");
				structItem->SetAttribute("name", (*subIter).name);
				structItem->SetAttribute("qualifier", (*subIter).qualifier);
				structItem->SetAttribute("type", (*subIter).type);
				structItem->SetAttribute("dim", (*subIter).dim);
				structItems->LinkEndChild(structItem);
			}
			structElement->LinkEndChild(structItems);
			structsElement->LinkEndChild(structElement);
		}
	}
	/* 写入函数信息 */
	for (int i = 0; i < fileCount; i++) {
		cfileInfo = &cfileInfoAll[i];
		for (auto iter = cfileInfo->functions.begin(); iter != cfileInfo->functions.end(); iter++) {
			TiXmlElement *funElement = new TiXmlElement("FunctionsItem");
			funElement->SetAttribute("name", (*iter).name);
			funElement->SetAttribute("returnType", (*iter).type);
			funElement->SetAttribute("path", (*iter).path);
			funElement->SetAttribute("hPath", (*iter).hPath);
			TiXmlElement *parasItem = new TiXmlElement("parameters");
			for (auto subIter = (*iter).paras.begin(); subIter != (*iter).paras.end(); subIter++) {
				TiXmlElement *paraItem = new TiXmlElement("ParametersItem");
				paraItem->SetAttribute("name", (*subIter).name);
				paraItem->SetAttribute("qualifier",
					(*subIter).qualifier);
				paraItem->SetAttribute("type", (*subIter).type);
				paraItem->SetAttribute("dim", (*subIter).dim);
				parasItem->LinkEndChild(paraItem);
			}
			funElement->LinkEndChild(parasItem);
			funsElement->LinkEndChild(funElement);
		}
	}
	/* 写入错误信息 */
	for (int i = 0; i < fileCount; i++) {
		for (auto iter = cfileInfo->fileErrors.begin(); iter != cfileInfo->fileErrors.end(); iter++) {
			TiXmlElement *ErrorItem = new TiXmlElement("ErrorItem");
			ErrorItem->SetAttribute("location", (*iter).errorLoc);
			ErrorItem->SetAttribute("description", (*iter).errorInfo);
			errorsElement->LinkEndChild(ErrorItem);
		}
	}
	resources->LinkEndChild(dictsElement);
	resources->LinkEndChild(structsElement);
	resources->LinkEndChild(funsElement);
	resources->LinkEndChild(errorsElement);
	doc.LinkEndChild(decl);
	doc.LinkEndChild(resources);
	doc.SaveFile(xmlFile);
	// delete cfileInfoAll;
	delete[]cfileInfoAll;
	

}
void getCFileInfo(const char *filePath, int argc, const char **argv) {
	varIndex = -1;
	structIndex = -1;
	funIndex = -1;
	paraIndex = -1;
	fieldCount = -1;
	errorCount = -1;
	// initStruct();
	std::vector<std::string> fileList;

	fileList.push_back(filePath);
	CommonOptionsParser OptionsParser(argc, argv, MyToolCategory);
	ClangTool Tool(OptionsParser.getCompilations(), fileList);
	LoopPrinter GvarPrinter;
	MatchFinder Finder;

	Finder.addMatcher(varDecl(hasGlobalStorage()).bind("globalVars"),
		&GvarPrinter);

	Finder.addMatcher(
		functionDecl(unless(hasAncestor(recordDecl()))).bind("functionParas"),
		&GvarPrinter);

	Finder.addMatcher(
		recordDecl(recordDecl(isStruct()).bind("structs")).bind("fields"),
		&GvarPrinter);
	Finder.addMatcher(
		recordDecl(recordDecl(isUnion()).bind("unions")).bind("unionFields"),
		&GvarPrinter);
	clang::DiagnosticOptions *Options = new clang::DiagnosticOptions();
	MyDiagnosticConsumer *diagConsumer =
		new MyDiagnosticConsumer(llvm::errs(), Options);
	Tool.setDiagnosticConsumer(diagConsumer);
	Tool.run(newFrontendActionFactory(&Finder).get());
	cfileInfo->path = filePath;
	cfileInfo->globalVarsCount = varIndex + 1;
	cfileInfo->structCount = structIndex + 1;
	cfileInfo->funCount = funIndex + 1;
	cfileInfo->errorCount = errorCount + 1;
	//printCFileInfo();
}
// int main(int argc, const char **argv) {
//
//  /* 测试数据
//     for (i; i < argc; i++) {
//     std::cout << argv[i]<<"\n
//     }*/
//  int a = 38;
//  //const char *argvs1212[3] = {"", "", ""};
//  const char *argvs1212[38] = {
//      "",
//      "",
//      "--",
//      "-IC:\\Users\\bondc\\Desktop\\test\\\\lib\\h",
//      "-IC:\\Users\\bondc\\Desktop\\test\\usr\\0.common\\std_lib\\algor",
//      "-IC:\\Users\\bondc\\Desktop\\test\\usr\\0.common\\std_lib",
//      "-IC:\\Users\\bondc\\Desktop\\test\\usr\\0.common",
//      "-IC:\\Users\\bondc\\Desktop\\test\\usr\\1.application\\config",
//      "-IC:\\Users\\bondc\\Desktop\\test\\usr\\1.application\\proj",
//      "-IC:\\Users\\bondc\\Desktop\\test\\usr\\1.application\\share\\app",
//      "-IC:\\Users\\bondc\\Desktop\\test\\usr\\1.application\\share\\arith",
//      "-IC:\\Users\\bondc\\Desktop\\test\\usr\\1.application\\share\\mission",
//      "-IC:\\Users\\bondc\\Desktop\\test\\usr\\1.application\\share\\tctm",
//      "-IC:\\Users\\bondc\\Desktop\\test\\usr\\1.application\\share",
//      "-IC:\\Users\\bondc\\Desktop\\test\\usr\\2.device\\share\\aocc",
//      "-IC:\\Users\\bondc\\Desktop\\test\\usr\\2.device\\share\\OBDH",
//      "-IC:\\Users\\bondc\\Desktop\\test\\usr\\2.device\\share\\part\\ass",
//      "-IC:\\Users\\bondc\\Desktop\\test\\usr\\2.device\\share\\part\\dss",
//      "-IC:\\Users\\bondc\\Desktop\\test\\usr\\2.device\\share\\part\\ground",
//      "-IC:\\Users\\bondc\\Desktop\\test\\usr\\2.device\\share\\part\\gyro",
//      "-IC:\\Users\\bondc\\Desktop\\test\\usr\\2.device\\share\\part\\ires",
//      "-IC:\\Users\\bondc\\Desktop\\test\\usr\\2.device\\share\\part\\magm",
//      "-IC:\\Users\\bondc\\Desktop\\test\\usr\\2.device\\share\\part\\mt",
//      "-IC:\\Users\\bondc\\Desktop\\test\\usr\\2.device\\share\\part\\mw",
//      "-IC:\\Users\\bondc\\Desktop\\test\\usr\\2.device\\share\\part\\pd",
//      "-IC:\\Users\\bondc\\Desktop\\test\\usr\\2.device\\share\\part\\ppcu",
//      "-IC:\\Users\\bondc\\Desktop\\test\\usr\\2.device\\share\\part\\prop",
//      "-IC:\\Users\\bondc\\Desktop\\test\\usr\\2.device\\share\\part\\sada",
//      "-IC:\\Users\\bondc\\Desktop\\test\\usr\\2.device\\share\\part\\sts",
//      "-IC:\\Users\\bondc\\Desktop\\test\\usr\\2.device\\share",
//      "-IC:\\Users\\bondc\\Desktop\\test\\usr\\3.comif\\share",
//      "-IC:\\Users\\bondc\\Desktop\\test\\sys\\arch\\config",
//      "-IC:\\Users\\bondc\\Desktop\\test\\sys\\arch\\sip2115\\h",
//      "-IC:\\Users\\bondc\\Desktop\\test\\sys\\drv\\h",
//      "-IC:\\Users\\bondc\\Desktop\\test\\sys\\lib\\h",
//      "-IC:\\Users\\bondc\\Desktop\\test\\sys\\sys\\h",
//      "-IC:\\Users\\bondc\\Desktop\\test\\sys\\util\\h",
//      "-IC:\\Users\\bondc\\Desktop\\test\\sys\\os\\h"
//     };
//    //CFileInfoStruct *cfileInfo2 = new CFileInfoStruct();
//
//   //getCFileInfo("C:\\Users\\bondc\\Desktop\\test\\usr\\0.common\\common.c",
//   //            cfileInfo1, a, argvs1212);
//
//  const char *argvs1212[2] =
//  {"C:\\Users\\bondc\\Desktop\\test\\usr\\1.application\\share\\app\\a7_ModeExecute.c",
//  "C:\\Users\\bondc\\Desktop\\test\\usr\\0.common\\common.c"};
// //getCFileInfo("C:\\Users\\bondc\\Desktop\\test\\usr\\1.application\\share\\app\\a7_ModeExecute.c",
// //              a, argvs1212);
//  //printSchoolXml();
//  //readSchoolXml();
//  //writeSchoolXml();
//
//  /*getCFileInfo("C:\\Users\\bondc\\Desktop\\2.h", cfileInfo1, a,
//  argvs1212);*/
//  // getCFileInfo("C:\\Users\\bondc\\Desktop\\a.h", cfileInfo1);
//  return 0;
//}

int main(int argc, const char **argv) {

 int a = 3;
 const char *argvs1212[3] = {"", "", ""};
 const char *filePathList[1] = {"C:\\Users\\bondc\\Desktop\\a.c"};

 writeCOrHInfo2Xml(filePathList, 1, a, argvs1212,
                  "C:\\Users\\bondc\\Desktop\\A.xml");
 /*getCFileInfo("C:\\Users\\bondc\\Desktop\\2.h", cfileInfo1, a,
 argvs1212);*/
 return 0;
}