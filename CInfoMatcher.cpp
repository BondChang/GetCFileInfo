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
	
	/* �����ȫ�ֱ���,����ȫ�ֱ����ĸ�������ȫ�ֱ�������Ϣ������ṹ���� */
	if (varDeclNode) 
	{
		bool isExistedPath = isExistedNodePath(varDeclNode->getLocation(), Result, cfileInfo);
		if (false == isExistedPath) 
		{
			/* �ж��Ƿ������������ͣ��Ѿ��������������ļ���h�ļ� */
			bool isSkip = isSkipppingSituation(varDeclNode, Result);
			if (false == isSkip)
			{
				varIndex = dealVarDeclNode(varDeclNode, Result, varIndex, cfileInfo);
			}
		}
	}
	
	/* ����ǽṹ��,���½ṹ����������ṹ����Ϣ����ȫ�ֽṹ���б��� */
	if (varRecordDeclNode)
	{
		bool isExistedPath = isExistedNodePath(varRecordDeclNode->getLocation(), Result, cfileInfo);
		if (false == isExistedPath) 
		{
			structIndex = dealVarRecordDeclNode(varRecordDeclNode, Result, structIndex, cfileInfo);
		}
	}
	
	/* �����������,���½ṹ����������ṹ����Ϣ����ȫ�ֽṹ���б��� */
	if (varRecordDeclNodeUnion) 
	{
		bool isExistedPath = isExistedNodePath(varRecordDeclNodeUnion->getLocation(), Result, cfileInfo);
		if (false == isExistedPath)
		{
			structIndex = dealVarRecordDeclNode(varRecordDeclNodeUnion, Result, structIndex, cfileInfo);
		}
	}

	/* ����Ǻ����б�,���º����б�ĸ������洢�ں�����Ϣ�б��� */
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
