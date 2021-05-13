#include "CInfoMatcher.h"

int varIndex = -1;
int funIndex = -1;
int structIndex = -1;

CFileInfoStruct *cfileInfo;

void CInfoMatcher::run(const MatchFinder::MatchResult &Result) 
{
	VarDecl const *varDeclNode = Result.Nodes.getNodeAs<VarDecl>("globalVars");
	RecordDecl const *varRecordDeclNode = Result.Nodes.getNodeAs<RecordDecl>("structs");
	FunctionDecl const *functionDeclNode = Result.Nodes.getNodeAs<FunctionDecl>("functionParas");
	RecordDecl const *varRecordDeclNodeUnion = Result.Nodes.getNodeAs<RecordDecl>("unions");
	
	/* 如果是全局变量,更新全局变量的个数并把全局变量的信息存在与结构体中 */
	if (varDeclNode) 
	{
		bool isExistedPath = isExistedNodePath(varDeclNode->getLocation(), Result, cfileInfo);
		if (false == isExistedPath) 
		{
			/* 判断是否是跳过的类型，已经解析过且现在文件是h文件 */
			bool isSkip = isSkipppingSituation(varDeclNode, Result);
			if (false == isSkip)
			{
				varIndex = dealVarDeclNode(varDeclNode, Result, varIndex, cfileInfo);
			}
		}
	}
	
	/* 如果是结构体,更新结构体个数并将结构体信息存于全局结构体列表中 */
	if (varRecordDeclNode)
	{
		bool isExistedPath = isExistedNodePath(varRecordDeclNode->getLocation(), Result, cfileInfo);
		if (false == isExistedPath) 
		{
			structIndex = dealVarRecordDeclNode(varRecordDeclNode, Result, structIndex, cfileInfo);
		}
	}
	
	/* 如果是联合体,更新结构体个数并将结构体信息存于全局结构体列表中 */
	if (varRecordDeclNodeUnion) 
	{
		bool isExistedPath = isExistedNodePath(varRecordDeclNodeUnion->getLocation(), Result, cfileInfo);
		if (false == isExistedPath)
		{
			structIndex = dealVarRecordDeclNode(varRecordDeclNodeUnion, Result, structIndex, cfileInfo);
		}
	}

	/* 如果是函数列表,更新函数列表的个数并存储于函数信息列表中 */
	if (functionDeclNode)
	{
		bool isExistedPath = isExistedNodePath(functionDeclNode->getLocation(), Result, cfileInfo);
		if (false == isExistedPath)
		{
			funIndex = dealFunctionDeclNode(functionDeclNode, Result, funIndex, cfileInfo);
		}
	}

	return;
}
