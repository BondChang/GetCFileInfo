#include "CInfoUtil.h"
#include "../SunwiseInfoUtil/SunwiseInfoUtil.h"
std::string basePath;
std::vector<std::string> allExistPath;
std::vector<std::string> globalNames;
std::vector<AnonymousStruct> allAnonymousStructs;

int anonymousCount = 0;



bool judgeIsInTargetFile(std::string fileName, std::string targetPath)
{
	char fullPathFile[FN_BUFSIZE] = { 0 };
	char fullPathTarget[FN_BUFSIZE] = { 0 };

	GetFullPath(fullPathFile, strdup(fileName.c_str()));
	GetFullPath(fullPathTarget, strdup(targetPath.c_str()));

	if (startsWith(fullPathFile, fullPathTarget)) {
		return true;
	}
	else {
		return false;
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

std::string getNodePath(SourceLocation sourceLoc, const MatchFinder::MatchResult &Result)
{
	SourceManager &sourceManager = Result.Context->getSourceManager();
	std::string fileName = sourceManager.getFilename(sourceLoc).str();
	char fullName[FN_BUFSIZE];

	GetFullPath(fullName, strdup(fileName.c_str()));

	return std::string(fullName);
}

/* BUG ? */
bool judgeIsAnonymousStruct(QualType nodeType, const MatchFinder::MatchResult &Result, RecordDecl::field_iterator jt)
{
	const clang::Type *pType = nodeType.getTypePtr();
	const clang::RecordType *pStructType = pType->getAsStructureType();

	RecordDecl *recordDecl = pStructType->getDecl();
	SourceManager &sourceManager = Result.Context->getSourceManager();

	/* 查找结构体定义的行号 */
	PresumedLoc recordDefinePresume = sourceManager.getPresumedLoc(recordDecl->getEndLoc());
	int defineLine = recordDefinePresume.getLine();
	PresumedLoc fieldMemberPresume = sourceManager.getPresumedLoc(jt->getEndLoc());
	int fieldMemberLine = fieldMemberPresume.getLine();

	return (defineLine == fieldMemberLine);
}


/* 获取节点的起始列 */
int getNodeCol(SourceRange sourceRange, SourceManager &sourceManager)
{
	SourceLocation srcLoc = sourceRange.getBegin();
	PresumedLoc locStart = sourceManager.getPresumedLoc(srcLoc);
	unsigned int nodeCol = locStart.getColumn();

	return nodeCol;
}

/* 获取节点的起始行 */
int getNodeRow(SourceRange sourceRange, SourceManager &sourceManager)
{
	SourceLocation srcLoc = sourceRange.getBegin();
	PresumedLoc locStart = sourceManager.getPresumedLoc(srcLoc);
	unsigned int nodeLine = locStart.getLine();

	return nodeLine;
}


std::string getAnonymousName(RecordDecl const *varRecordDeclNode, const MatchFinder::MatchResult &Result) 
{
	std::string nodePath = getNodePath(varRecordDeclNode->getLocation(), Result);
	SourceManager &sourceManager = Result.Context->getSourceManager();
	int nodeRow = getNodeRow(varRecordDeclNode->getSourceRange(), sourceManager);
	int nodeCol = getNodeCol(varRecordDeclNode->getSourceRange(), sourceManager);
	
	if (allAnonymousStructs.size() > 0) 
	{
		for (std::vector<AnonymousStruct>::iterator iter = allAnonymousStructs.begin(); 
			iter != allAnonymousStructs.end(); iter++)
		{
			/* bug ? */
			if ((strcmp(nodePath.c_str(), iter->path.c_str()) == 0) &&
				(nodeRow == iter->row) && (nodeCol == iter->col))
			{
				return iter->name;
			}
		}
	}

	return "";
}

void addAnonymousInfo2Vector(SourceRange sourceRange, SourceLocation sourceLocation, const MatchFinder::MatchResult &Result, std::string fieldStructName) 
{
	AnonymousStruct anonymousStruct;
	anonymousStruct.path = getNodePath(sourceLocation, Result);
	SourceManager &sourceManager = Result.Context->getSourceManager();
	anonymousStruct.row = getNodeRow(sourceRange, sourceManager);
	anonymousStruct.col = getNodeCol(sourceRange, sourceManager);
	anonymousStruct.name = fieldStructName;
	allAnonymousStructs.push_back(anonymousStruct);
}


/* 设置默认值 */
void getDefaultVaule(const ImplicitValueInitExpr *iV, std::string *defaultValueStr)
{
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

/* 处理全局变量的初值 */
void dealInitValue(const Expr *initExpr, std::string *initValueStr,
	const MatchFinder::MatchResult &Result,
	std::vector<InitAstStruct> *initAstStructVector) {

	if (!(initExpr == NULL)) {
		/* double或float类型的初值 */
		if (isa<FloatingLiteral>(initExpr)) {
           const FloatingLiteral *fL =dyn_cast<FloatingLiteral>(initExpr);
           double initValue = fL->getValueAsApproximateDouble();
            /* 将float/double强转为String */
            std::string singleInitStr=dbl2str(initValue);
            /*std::stringstream ss;
            ss.precision(20);
            ss << initValue<<std::noshowpoint;*/
            *initValueStr += singleInitStr;
			InitAstStruct initAstStruct;
            initAstStruct.initValueAst = strcpy(new char[singleInitStr.length() + 1],singleInitStr.data());
			initAstStruct.nodeLocInfo = getLineNumInfo(fL->getSourceRange(), Result.Context);
			initAstStruct.isStructOrArrayBegin = false;
			initAstStruct.isStructOrArrayEnd = false;
			initAstStructVector->push_back(initAstStruct);
        }
		/* int类型的value */
		else if (isa<IntegerLiteral>(initExpr)) {
			const IntegerLiteral *iL = dyn_cast<IntegerLiteral>(initExpr);
                  SourceRange Loc = initExpr->getSourceRange();
                  SourceLocation slStart = Loc.getBegin();
                  SourceLocation slEnd = Loc.getEnd();
                  SourceManager &SM =Result.Context->getSourceManager();
                  const char *Begin = SM.getCharacterData(slStart);
                  const char *End = SM.getCharacterData(Lexer::getLocForEndOfToken(slEnd, 0, SM,Result.Context->getLangOpts()));
                  std::string intValueStr = StringRef(Begin, End - Begin);
                  // const char *pStrEnd = SM.getCharacterData(slEnd);
                  //std::string intValueStr(pStrStart, pStrEnd);
                  // auto a = iL->getValue().getBitsNeeded(
                 // iL->getValue().getSExtValue());
                //std::string intValueStr(Begin);
			   ///* 将int强转为String */
             *initValueStr += intValueStr;
			//*initValueStr += std::to_string(iL->getValue().getSExtValue());
			InitAstStruct initAstStruct;
             initAstStruct.initValueAst =strcpy(new char[intValueStr.length() + 1], intValueStr.data());
			//initAstStruct.initValueAst = strcpy(new char[intValueStr.length() + 1],std::to_string(iL->getValue().getSExtValue()).data());
			initAstStruct.nodeLocInfo = getLineNumInfo(iL->getSourceRange(), Result.Context);
			initAstStruct.isStructOrArrayBegin = false;
			initAstStruct.isStructOrArrayEnd = false;
			initAstStructVector->push_back(initAstStruct);
		}
		else if (isa<ImplicitCastExpr>(initExpr)) {
			const ImplicitCastExpr *iCE = dyn_cast<ImplicitCastExpr>(initExpr);
			auto subExpr = iCE->getSubExpr();
			dealInitValue(subExpr, initValueStr, Result, initAstStructVector);
		}
		/* 没有值的情况 */
        else if(isa<ImplicitValueInitExpr>(initExpr)) {
			std::string defaultValueStr = "";
			const ImplicitValueInitExpr *iV =
				dyn_cast<ImplicitValueInitExpr>(initExpr);
			/* 将int强转为String */
			getDefaultVaule(iV, &defaultValueStr);
			*initValueStr += defaultValueStr;
		}
		/* 数组 */
        else if (isa<InitListExpr>(initExpr)) {

			const InitListExpr *iLE = dyn_cast<InitListExpr>(initExpr);
			SourceLocation Loc = iLE->getSourceRange().getBegin();
			signed NumInits = iLE->getNumInits();
			// const Expr *temp = iLE->getInit(0);
			QualType initExprType = initExpr->getType();
			*initValueStr += "{";
			InitAstStruct initAstStructStart;
			initAstStructStart.isStructOrArrayBegin = true;
			initAstStructStart.isStructOrArrayEnd = false;
			initAstStructStart.nodeLocInfo = getLineNumInfo(iLE->getSourceRange(), Result.Context);

			for (int i = 0; i < NumInits; i++) {
				dealInitValue(iLE->getInit(i), initValueStr, Result,
					&initAstStructStart.initAstStruct);
				*initValueStr += ",";
			}
			InitAstStruct initAstStructEnd;
			initAstStructEnd.isStructOrArrayBegin = false;
			initAstStructEnd.isStructOrArrayEnd = true;

			initAstStructStart.initAstStruct.push_back(initAstStructEnd);
			initAstStructVector->push_back(initAstStructStart);
			// variableInfoStruct->initAstStruct.push_back(initAstStructEnd);
			int num = initValueStr->length();
			if (num > 0) {
				*initValueStr = initValueStr->substr(0, num - 1);
			}
			*initValueStr += "}";
			
		} else
		{
                  std::string initExprStr=parserInitExpr(initExpr);
                  *initValueStr += initExprStr;
                  InitAstStruct initAstStruct;
                  initAstStruct.initValueAst =
                      strcpy(new char[initExprStr.length() + 1],
                             initExprStr.data());
                  initAstStruct.nodeLocInfo =
                      getLineNumInfo(initExpr->getSourceRange(), Result.Context);
                  initAstStruct.isStructOrArrayBegin = false;
                  initAstStruct.isStructOrArrayEnd = false;
                  initAstStructVector->push_back(initAstStruct);
        }
	}
}

void dealRecordField(StructInfo *pStructInfo, RecordDecl::field_iterator &jt, const MatchFinder::MatchResult &Result)
{
	VariableInfoStruct variableInfoStruct;
	variableInfoStruct.nodeLocInfo = getLineNumInfo(jt->getSourceRange(), Result.Context);
	std::string nameStr = jt->getNameAsString();

	/* 获取全局变量的类型，为总类型 */
	QualType jtType = jt->getType();
	std::string typeStr = jtType.getLocalUnqualifiedType().getCanonicalType().getAsString();
	variableInfoStruct.name = strcpy(new char[nameStr.length() + 1], nameStr.data());

	/* 将全局变量的类型转成Type类型,便于后续判断是否为数组 */
	QualType nodeType = jt->getType();
	dealType(typeStr, nodeType, &variableInfoStruct);

	/* 处理匿名结构体 */
	if (nodeType.getTypePtr()->isStructureType())
	{
		bool isAnonymousStruct = judgeIsAnonymousStruct(nodeType, Result, jt);
		if (isAnonymousStruct) {
			std::string type = "anonymous";
			type.append(std::to_string(anonymousCount++));
			variableInfoStruct.type = strcpy(new char[type.length() + 1], type.data());
			addAnonymousInfo2Vector(jt->getSourceRange(), jt->getLocation(), Result, type);
		}
	}

	pStructInfo->fieldCount++;
	pStructInfo->fields.push_back(variableInfoStruct);

	return;
}

/* 解析数组类型 */
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
	if (childType->isConstantArrayType() || childType->isIncompleteArrayType())
	{
		dimStr += "[";
		return dealDim(childType, dimStr);
	}
	else {
		return dimStr;
	}
}

/* 解析指针类型 */
QualType dealPointerType(const Type *pointerType, std::string &pointer)
{
	const Type *childType = pointerType->getPointeeType()
		.getLocalUnqualifiedType()
		.getCanonicalType()
		.getTypePtr();
	/* 如果子表达式仍为指针类型 */
	if (childType->isPointerType()) {
		pointer = pointer + "*";
		return dealPointerType(childType, pointer);
	}
	/* 如果不为指针类型 */
	else {
		QualType type = pointerType->getPointeeType();
		return type;
	}
}

/* 处理类型定义 */
void dealType(std::string typeStr, QualType nodeType, VariableInfoStruct *variableInfoStruct)
{
	variableInfoStruct->pointerType = 0;
	const Type *type = nodeType.getTypePtr();

	/* 说明有类型修饰符 */
	if (nodeType.hasQualifiers()) {
		std::string qualifier = nodeType.getQualifiers().getAsString();
		variableInfoStruct->qualifier = strcpy(new char[qualifier.length() + 1], qualifier.data());
	}
	/* 若无修饰符则置为空 */
	else {
		std::string qualifier = "";
		variableInfoStruct->qualifier = strcpy(new char[qualifier.length() + 1], qualifier.data());
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
			dealType(typeStr, qualType, variableInfoStruct);
		}
		variableInfoStruct->dim =
			strcpy(new char[dimStr.length() + 1], dimStr.data());
	}
	/* 如果是指针类型 */
	else if (type->isPointerType()) {
		variableInfoStruct->pointerType = 1;
		/* 处理指针类型的* */
		std::string pointer = "*";
		/* 获取除*以外的类型 */
		QualType pointType = dealPointerType(type, pointer);
		typeStr = pointType.getAsString();
		/* 如果像包括const，volatile修饰符 */
		if (pointType.hasQualifiers()) {
			std::string qualifier = nodeType.getQualifiers().getAsString();
			variableInfoStruct->qualifier = strcpy(new char[qualifier.length() + 1], qualifier.data());
		}
		typeStr = pointType.getLocalUnqualifiedType().getCanonicalType().getAsString();

		/* 类型加指针 */
		typeStr = typeStr;
		variableInfoStruct->type = strcpy(new char[typeStr.length() + 1], typeStr.data());
		std::string dimStr = pointer;
		variableInfoStruct->dim = strcpy(new char[dimStr.length() + 1], dimStr.data());
	}
	/* 如果不是数组 */
	else {
		variableInfoStruct->type = strcpy(new char[typeStr.length() + 1], typeStr.data());
		std::string dimStr = "";
		variableInfoStruct->dim = strcpy(new char[dimStr.length() + 1], dimStr.data());
	}
}

NodeLoc getLineNumInfo(SourceRange sourceRange, ASTContext *Context)
{
	/* bug 内存泄露 */
	NodeLoc *nodeLocInfo = (struct NodeLoc *)malloc(sizeof(struct NodeLoc));

	SourceLocation slStart = sourceRange.getBegin();
	SourceLocation slEnd = sourceRange.getEnd();
	SourceManager &sourceManager = Context->getSourceManager();
	bool InvalidStartLine = true;
	bool InvalidStartCol = true;
	bool InvalidEndLine = true;
	bool InvalidEndCol = true;
	unsigned CStartLine = sourceManager.getSpellingLineNumber(slStart, &InvalidStartLine);
	unsigned CStartCol = sourceManager.getSpellingColumnNumber(slStart, &InvalidStartCol);
	unsigned CEndLine = sourceManager.getSpellingLineNumber(slEnd, &InvalidEndLine);
	unsigned CEndCol = sourceManager.getSpellingColumnNumber(slEnd, &InvalidEndCol);

	if (!InvalidStartLine && !InvalidStartCol && !InvalidEndLine && !InvalidEndCol) {
		nodeLocInfo->StartLine = CStartLine;
		nodeLocInfo->StartCol = CStartCol;
		nodeLocInfo->EndLine = CEndLine;
		nodeLocInfo->EndCol = CEndCol;
	}
	return *nodeLocInfo;
}
