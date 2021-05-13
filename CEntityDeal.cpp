#include "CEntityDeal.h"
#include "../SunwiseInfoUtil/SunwiseInfoUtil.h"

bool isExistedNodePath(SourceLocation sourceLoc, const MatchFinder::MatchResult &Result, CFileInfoStruct *cfile)
{
	SourceManager &sourceManager = Result.Context->getSourceManager();
	std::string path = sourceManager.getFilename(sourceLoc).str().c_str();

	if (path.empty() || path.length() == 0)
	{
		return true;
	}

	std::replace(path.begin(), path.end(), '\\', '/');
	bool isInTargetFile = judgeIsInTargetFile(path, basePath);
	/* ˵����target�����ļ��� */
	if (!isInTargetFile) {
		return true;
	}
	cfile->everyFileExistPath.push_back(path);

	if (allExistPath.size() > 0) {
		/* �ж�·���Ƿ��Ѿ������� */
		for (auto iter = allExistPath.begin(); iter != allExistPath.end(); iter++) {
			/* ��const char* תΪchar* */
			char *fileNameInVector = strdup((*iter).c_str());
			char *presentFileName = strdup(path.c_str());
			if (ComparePathSame(fileNameInVector, presentFileName) == 0) {
				return true;
			}
		}
	}

	return false;
}

bool isSkipppingSituation(VarDecl const *varDeclNode, const MatchFinder::MatchResult &Result)
{
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
	}
	else
	{
		globalNames.push_back(varDeclNode->getNameAsString());
	}

	return false;
}

/* ����ȫ�ֱ�����Ϣ */
int dealVarDeclNode(VarDecl const *varDeclNode, const MatchFinder::MatchResult &Result, int varIdx, CFileInfoStruct *cfile)
{
	VariableInfoStruct variableInfoStruct;
	/* ��ȡ������ */
	std::string nameStr = varDeclNode->getNameAsString();
	std::string initValueStr = "";
	std::string fileName = getNodePath(varDeclNode->getLocation(), Result);
	auto firstDeclNode = varDeclNode->getFirstDecl();
	std::string firstDeclPath = getNodePath(firstDeclNode->getLocation(), Result);
	variableInfoStruct.hPath =
		strcpy(new char[firstDeclPath.length() + 1], firstDeclPath.data());
	variableInfoStruct.path =
		strcpy(new char[fileName.length() + 1], fileName.data());
	variableInfoStruct.name =
		strcpy(new char[nameStr.length() + 1], nameStr.data());
	variableInfoStruct.nodeLocInfo = getLineNumInfo(varDeclNode->getSourceRange(), Result.Context);
	/* ��ȡ��������,����������Ϣ */
	std::string typeStr = varDeclNode->getType()
		.getLocalUnqualifiedType()
		.getCanonicalType()
		.getAsString();
	QualType nodeType = varDeclNode->getType();
	// std::cout << varDeclNode->getType().getTypePtr()->getTypeClassName();
	const Type *type = varDeclNode->getType().getTypePtr();
	std::string name = clang::QualType(type, 0).getAsString();
	// std::cout << nodeType.getCanonicalType().getAsString() << "\n";
	dealType(typeStr, nodeType, &variableInfoStruct);
	/* ��ȡ��ʼ���� */
	const Expr *initExpr = varDeclNode->getAnyInitializer();
	/* ����ȫ�ֱ����ĳ�ֵ */
	std::vector<InitAstStruct> initAstStructVector;
	dealInitValue(initExpr, &initValueStr, Result, &initAstStructVector);
	variableInfoStruct.initValue =
		strcpy(new char[initValueStr.length() + 1], initValueStr.data());
	variableInfoStruct.initAstStruct = initAstStructVector;
	cfile->globalVars.push_back(variableInfoStruct);

	return (varIdx + 1);
}

/* ����ṹ����Ϣ */
int dealVarRecordDeclNode(RecordDecl const *varRecordDeclNode, const MatchFinder::MatchResult &Result, int structIdx, CFileInfoStruct *cfile)
{
	if (varRecordDeclNode->field_begin() != varRecordDeclNode->field_end())
	{
		/* ˵���ṹ���г�Ա */
		StructInfo structInfo;
		structInfo.nodeLocInfo = getLineNumInfo(varRecordDeclNode->getSourceRange(), Result.Context);
		std::string nameStr = "";
		std::string qualifier = "";

		/* ����������ṹ��,���ȡ�����ṹ������� */
		std::string anonymousName = getAnonymousName(varRecordDeclNode, Result);
		if (!anonymousName.empty()) {
			nameStr = anonymousName;
		}
		if (nameStr.empty() && varRecordDeclNode->getTypedefNameForAnonDecl()) {
			/* ��ȡ�ṹ���typedef���� */
			nameStr = varRecordDeclNode->getTypedefNameForAnonDecl()->getNameAsString();
		}

		/* BUG ? */
		if (nameStr.empty()) {
			/* ֱ�ӻ�ȡ�ṹ������� */
			nameStr = varRecordDeclNode->getNameAsString();
		}
		else {
			qualifier = "typedef";
		}

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

		/* ��¼�ṹ���field���� */
		for (RecordDecl::field_iterator jt = varRecordDeclNode->field_begin(); jt != varRecordDeclNode->field_end(); jt++)
		{
			dealRecordField(&structInfo, jt, Result);
		}

		cfile->structs.push_back(structInfo);

		return (structIdx + 1);
	}
	else
	{
		return structIdx;
	}
}

/* �������б���Ϣ */
int dealFunctionDeclNode(FunctionDecl const *functionDeclNode, const MatchFinder::MatchResult &Result, int funIdx, CFileInfoStruct *cfile)
{
	const FunctionDecl *firstDeclNode = functionDeclNode->getFirstDecl();
	std::string firstDeclFilePath = getNodePath(firstDeclNode->getLocation(), Result);
	FunctionInfoStruct functionInfoStruct;
	std::string fileName = getNodePath(functionDeclNode->getLocation(), Result);
	functionInfoStruct.path = strcpy(new char[fileName.length() + 1], fileName.data());
	functionInfoStruct.hPath = strcpy(new char[firstDeclFilePath.length() + 1], firstDeclFilePath.data());
	std::string nameStr = functionDeclNode->getNameAsString();
	std::string typeStr = functionDeclNode->getReturnType().getAsString();
	functionInfoStruct.name = strcpy(new char[nameStr.length() + 1], nameStr.data());
	functionInfoStruct.type = strcpy(new char[typeStr.length() + 1], typeStr.data());
	functionInfoStruct.nodeLocInfo = getLineNumInfo(functionDeclNode->getSourceRange(), Result.Context);
	functionInfoStruct.paraCount = functionDeclNode->getNumParams();

	unsigned int paraNum = functionDeclNode->getNumParams();
	if (paraNum > 0) 
	{
		for (int i = 0; i < paraNum; i++)
		{
			VariableInfoStruct variableInfoStruct;
			std::string nameStr = functionDeclNode->getParamDecl(i)->getNameAsString();
			variableInfoStruct.name = strcpy(new char[nameStr.length() + 1], nameStr.data());

			/* ��ȡ�������������� */
			const clang::ParmVarDecl *paramDecl = functionDeclNode->getParamDecl(i);
			QualType parType = paramDecl->getType();
			QualType parLocalUnqualifiedType = parType.getLocalUnqualifiedType();
			QualType parCanonicalType = parLocalUnqualifiedType.getCanonicalType();
			std::string typeStr = parCanonicalType.getAsString();

			/* ����������������ת��ΪType,��������ж��Ƿ�Ϊ�������� */
			const Type *type = functionDeclNode->getParamDecl(i)->getType().getTypePtr();
			clang::SourceRange srcRange = functionDeclNode->getParamDecl(i)->getSourceRange();
			variableInfoStruct.nodeLocInfo = getLineNumInfo(srcRange, Result.Context);

			/* ˵����Decayed����,���������������ͣ���clangת����ָ������ */
			QualType nodeType = functionDeclNode->getParamDecl(i)->getType();
			if (type->getTypeClass() == 8) {
				const DecayedType *DT = type->getAs<DecayedType>();
				type = DT->getOriginalType().getTypePtr();
				nodeType = DT->getOriginalType();
			}
			dealType(typeStr, nodeType, &variableInfoStruct);
			functionInfoStruct.paras.push_back(variableInfoStruct);
		}
	}
	cfile->functions.push_back(functionInfoStruct);

	return (funIdx + 1);
}

void dealComplexValue(TiXmlElement *initValueAst, InitAstStruct startValue)
{
	TiXmlElement *complexValue = new TiXmlElement("complexValue");
	initValueAst->LinkEndChild(complexValue);
	complexValue->SetAttribute("startLine", startValue.nodeLocInfo.StartLine);
	complexValue->SetAttribute("endLine", startValue.nodeLocInfo.EndLine);
	complexValue->SetAttribute("startCol", startValue.nodeLocInfo.StartCol);
	complexValue->SetAttribute("endCol", startValue.nodeLocInfo.EndCol);
	for (auto subIter = (startValue).initAstStruct.begin();
		subIter != (startValue).initAstStruct.end(); subIter++) {
		if ((*subIter).isStructOrArrayBegin) {
			dealComplexValue(complexValue, (*subIter));
		}
		else if ((*subIter).isStructOrArrayEnd) {
			continue;
		}
		else {
			TiXmlElement *expression = new TiXmlElement("expression");
			expression->SetAttribute("initValue", ((*subIter)).initValueAst.c_str());
			expression->SetAttribute("startLine", ((*subIter)).nodeLocInfo.StartLine);
			expression->SetAttribute("endLine", ((*subIter)).nodeLocInfo.EndLine);
			expression->SetAttribute("startCol", ((*subIter)).nodeLocInfo.StartCol);
			expression->SetAttribute("endCol", ((*subIter)).nodeLocInfo.EndCol);
			complexValue->LinkEndChild(expression);
		}
	}
}
