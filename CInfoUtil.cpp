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

/* �ж��Ƿ����Ѿ���¼��ȫ�ֱ��� */
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

	/* ���ҽṹ�嶨����к� */
	PresumedLoc recordDefinePresume = sourceManager.getPresumedLoc(recordDecl->getEndLoc());
	int defineLine = recordDefinePresume.getLine();
	PresumedLoc fieldMemberPresume = sourceManager.getPresumedLoc(jt->getEndLoc());
	int fieldMemberLine = fieldMemberPresume.getLine();

	return (defineLine == fieldMemberLine);
}


/* ��ȡ�ڵ����ʼ�� */
int getNodeCol(SourceRange sourceRange, SourceManager &sourceManager)
{
	SourceLocation srcLoc = sourceRange.getBegin();
	PresumedLoc locStart = sourceManager.getPresumedLoc(srcLoc);
	unsigned int nodeCol = locStart.getColumn();

	return nodeCol;
}

/* ��ȡ�ڵ����ʼ�� */
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


/* ����Ĭ��ֵ */
void getDefaultVaule(const ImplicitValueInitExpr *iV, std::string *defaultValueStr)
{
	QualType ivType = iV->getType();
	/* double��float */
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

/* ����ȫ�ֱ����ĳ�ֵ */
void dealInitValue(const Expr *initExpr, std::string *initValueStr,
	const MatchFinder::MatchResult &Result,
	std::vector<InitAstStruct> *initAstStructVector) {

	if (!(initExpr == NULL)) {
		/* double��float���͵ĳ�ֵ */
		if (isa<FloatingLiteral>(initExpr)) {
           const FloatingLiteral *fL =dyn_cast<FloatingLiteral>(initExpr);
           double initValue = fL->getValueAsApproximateDouble();
            /* ��float/doubleǿתΪString */
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
		/* int���͵�value */
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
			   ///* ��intǿתΪString */
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
		/* û��ֵ����� */
        else if(isa<ImplicitValueInitExpr>(initExpr)) {
			std::string defaultValueStr = "";
			const ImplicitValueInitExpr *iV =
				dyn_cast<ImplicitValueInitExpr>(initExpr);
			/* ��intǿתΪString */
			getDefaultVaule(iV, &defaultValueStr);
			*initValueStr += defaultValueStr;
		}
		/* ���� */
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

	/* ��ȡȫ�ֱ��������ͣ�Ϊ������ */
	QualType jtType = jt->getType();
	std::string typeStr = jtType.getLocalUnqualifiedType().getCanonicalType().getAsString();
	variableInfoStruct.name = strcpy(new char[nameStr.length() + 1], nameStr.data());

	/* ��ȫ�ֱ���������ת��Type����,���ں����ж��Ƿ�Ϊ���� */
	QualType nodeType = jt->getType();
	dealType(typeStr, nodeType, &variableInfoStruct);

	/* ���������ṹ�� */
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

/* ������������ */
QualType dealArrayType(const Type *type) {

	/* ת������������ */
	const ArrayType *arrayType = type->castAsArrayTypeUnsafe();
	/* ��ȡ�����ά�� */

	/* ��ȡ��������� */
	const Type *childType = arrayType->getElementType()
		.getLocalUnqualifiedType()
		.getCanonicalType()
		.getTypePtr();
	/* ��������ΪConstantArrayType()����int[3] */
	/* ��������ΪIncompleteArrayType����int[] */
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

	/* ת������������ */
	const ArrayType *arrayType = type->castAsArrayTypeUnsafe();
	/* ����ConstantArrayType���ͣ���ȡ�����ά�� */
	if (arrayType->isConstantArrayType()) {
		int dim =
			dyn_cast<ConstantArrayType>(arrayType)->getSize().getLimitedValue();
		dimStr += std::to_string(dim);
		dimStr += "]";
	}
	/* ����IncompleteArrayType���� */
	else if (arrayType->isIncompleteArrayType()) {
		dimStr += "]";
	}

	/* ��ȡ��������� */
	const Type *childType = arrayType->getElementType().getTypePtr();

	/* �����ConstantArrayType����IncompleteArrayType���͵����� */
	if (childType->isConstantArrayType() || childType->isIncompleteArrayType())
	{
		dimStr += "[";
		return dealDim(childType, dimStr);
	}
	else {
		return dimStr;
	}
}

/* ����ָ������ */
QualType dealPointerType(const Type *pointerType, std::string &pointer)
{
	const Type *childType = pointerType->getPointeeType()
		.getLocalUnqualifiedType()
		.getCanonicalType()
		.getTypePtr();
	/* ����ӱ��ʽ��Ϊָ������ */
	if (childType->isPointerType()) {
		pointer = pointer + "*";
		return dealPointerType(childType, pointer);
	}
	/* �����Ϊָ������ */
	else {
		QualType type = pointerType->getPointeeType();
		return type;
	}
}

/* �������Ͷ��� */
void dealType(std::string typeStr, QualType nodeType, VariableInfoStruct *variableInfoStruct)
{
	variableInfoStruct->pointerType = 0;
	const Type *type = nodeType.getTypePtr();

	/* ˵�����������η� */
	if (nodeType.hasQualifiers()) {
		std::string qualifier = nodeType.getQualifiers().getAsString();
		variableInfoStruct->qualifier = strcpy(new char[qualifier.length() + 1], qualifier.data());
	}
	/* �������η�����Ϊ�� */
	else {
		std::string qualifier = "";
		variableInfoStruct->qualifier = strcpy(new char[qualifier.length() + 1], qualifier.data());
	}

	/* ��������ΪConstantArrayType�����飬��int[3] */
	/* ����IncompleteArrayType���͵�����,��int[] */
	if (type->isConstantArrayType() || type->isIncompleteArrayType()) {
		/* ������������� */
		QualType qualType = dealArrayType(type);
		typeStr = qualType.getAsString();
		/* ���������ά�� */
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
	/* �����ָ������ */
	else if (type->isPointerType()) {
		variableInfoStruct->pointerType = 1;
		/* ����ָ�����͵�* */
		std::string pointer = "*";
		/* ��ȡ��*��������� */
		QualType pointType = dealPointerType(type, pointer);
		typeStr = pointType.getAsString();
		/* ��������const��volatile���η� */
		if (pointType.hasQualifiers()) {
			std::string qualifier = nodeType.getQualifiers().getAsString();
			variableInfoStruct->qualifier = strcpy(new char[qualifier.length() + 1], qualifier.data());
		}
		typeStr = pointType.getLocalUnqualifiedType().getCanonicalType().getAsString();

		/* ���ͼ�ָ�� */
		typeStr = typeStr;
		variableInfoStruct->type = strcpy(new char[typeStr.length() + 1], typeStr.data());
		std::string dimStr = pointer;
		variableInfoStruct->dim = strcpy(new char[dimStr.length() + 1], dimStr.data());
	}
	/* ����������� */
	else {
		variableInfoStruct->type = strcpy(new char[typeStr.length() + 1], typeStr.data());
		std::string dimStr = "";
		variableInfoStruct->dim = strcpy(new char[dimStr.length() + 1], dimStr.data());
	}
}

NodeLoc getLineNumInfo(SourceRange sourceRange, ASTContext *Context)
{
	/* bug �ڴ�й¶ */
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
