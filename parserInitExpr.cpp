#include "parserInitExpr.h"


/* double转string */
std::string dbl2str(double d) {
  size_t len = std::snprintf(0, 0, "%.10f", d);
  std::string s(len + 1, 0);
  // 将double值赋值给string
  std::snprintf(&s[0], len + 1, "%.10f", d);
  // 移除string 终止符号
  s.pop_back();
  // 如果存在很多个0，擦除
  s.erase(s.find_last_not_of('0') + 1, std::string::npos);
  // 如果结尾是小数点，擦除
  if (s.back() == '.') {
    s.pop_back();
  }
  return s;
}

/* 删除不必要的符号 */
std::string deleteUnnecessarySign(std::string exprStr) {
	if (exprStr.length() >= 1) {
		if (exprStr.length() >= 2) {
			std::string suffix =
				exprStr.substr(exprStr.length() - 2, exprStr.length());
			if (suffix == "->") {
				exprStr = exprStr.substr(0, exprStr.length() - 2);
			}
		}
		std::string commaSuffix =
			exprStr.substr(exprStr.length() - 1, exprStr.length());
		if (commaSuffix == ".") {
			exprStr = exprStr.substr(0, exprStr.length() - 1);
		}
	}
	return exprStr;
}
/* 解析每一个节点 */
std::string parserInitExpr(const Stmt *stmt) {
	switch (stmt->getStmtClass()) {
	case Expr::ImplicitCastExprClass:
	case Expr::CXXFunctionalCastExprClass:
	case Expr::CXXStaticCastExprClass:
	case Expr::CXXReinterpretCastExprClass:
	case Expr::CXXConstCastExprClass: {
		return parserInitExpr(cast<CastExpr>(stmt)->getSubExpr());
	}
	case Expr::CStyleCastExprClass: {
		const CStyleCastExpr *cStyleCastExpr = dyn_cast<CStyleCastExpr>(stmt);
		std::string cStyleCastExprStr=cStyleCastExpr->getExprStmt()->getType().getAsString();
		return "("+ cStyleCastExprStr+")"+ parserInitExpr(cast<CastExpr>(stmt)->getSubExpr());

	}
	case Expr::CXXBoolLiteralExprClass: {
		const CXXBoolLiteralExpr *cxxBoolLiteralExpr = dyn_cast<CXXBoolLiteralExpr>(stmt);
		bool boolExpr = cxxBoolLiteralExpr->getValue();
		if (boolExpr) {
			return "1";
		}
		else {
			return "0";
		}
	}
	case Expr::CXXDeleteExprClass: {
		const CXXDeleteExpr *cxxDeleteExpr = dyn_cast<CXXDeleteExpr>(stmt);
		//auto a = cxxDeleteExpr->getExprStmt();

		auto deleteArgument = cxxDeleteExpr->getArgument();
		auto deleteArgumentStr = parserInitExpr(deleteArgument);
		return "/*delete [] " + deleteArgumentStr + ";*/\n\t";
	}
	
	case Expr::ParenExprClass: {
		const ParenExpr *parenExpr = dyn_cast<ParenExpr>(stmt);
		const Expr *subParentExpr = parenExpr->getSubExpr();
		return "(" + parserInitExpr(subParentExpr) + ")";
	}
	
	case Expr::ConstantExprClass: {
		const ConstantExpr *constantExpr = dyn_cast<ConstantExpr>(stmt);
		return parserInitExpr(constantExpr->getSubExpr());
	}
	case Expr::BinaryOperatorClass: {
		const BinaryOperator *binaryOperator = dyn_cast<BinaryOperator>(stmt);
		Expr *leftBinaryOperator = binaryOperator->getLHS();
		Expr *rightBinaryOperator = binaryOperator->getRHS();
		//auto c = binaryOperator->getOpcode();
		switch (binaryOperator->getOpcode()) {
		case BO_Add: {
			return parserInitExpr(leftBinaryOperator) + "+" +
				parserInitExpr(rightBinaryOperator);
		}
		case BO_Sub: {
			return parserInitExpr(leftBinaryOperator) + "-" +
				parserInitExpr(rightBinaryOperator);
		}
		case BO_Mul: {
			return parserInitExpr(leftBinaryOperator) + "*" +
				parserInitExpr(rightBinaryOperator);
		}
		case BO_Div: {
			return parserInitExpr(leftBinaryOperator) + "/" +
				parserInitExpr(rightBinaryOperator);
		}
		case BO_LT: {
			return parserInitExpr(leftBinaryOperator) + "<" +
				parserInitExpr(rightBinaryOperator);
		}
		case BO_GT: {
			return parserInitExpr(leftBinaryOperator) + ">" +
				parserInitExpr(rightBinaryOperator);
		}
		case BO_LE: {
			return parserInitExpr(leftBinaryOperator) +
				"<=" + parserInitExpr(rightBinaryOperator);
		}
		case BO_Rem: {
			return parserInitExpr(leftBinaryOperator) + "%" +
				parserInitExpr(rightBinaryOperator);
		}
		case BO_GE: {
			return parserInitExpr(leftBinaryOperator) +
				">=" + parserInitExpr(rightBinaryOperator);
		}
		case BO_Assign: {
			std::string leftExprStr = parserInitExpr(leftBinaryOperator);
			std::string rightExprStr = parserInitExpr(rightBinaryOperator);
			leftExprStr = deleteUnnecessarySign(leftExprStr);
			rightExprStr = deleteUnnecessarySign(rightExprStr);
			return leftExprStr + "=" + rightExprStr;
		}
		case BO_Shl: {
			return parserInitExpr(leftBinaryOperator) + "<<" +
				parserInitExpr(rightBinaryOperator);
		}

		case BO_EQ: {
			return parserInitExpr(leftBinaryOperator) +
				"==" + parserInitExpr(rightBinaryOperator);
		}
		case BO_LAnd: {
			return parserInitExpr(leftBinaryOperator) + "&&" +
				parserInitExpr(rightBinaryOperator);
		}
		case BO_LOr: {
			return parserInitExpr(leftBinaryOperator) + "||" +
				parserInitExpr(rightBinaryOperator);
		}
		case BO_NE: {
			return parserInitExpr(leftBinaryOperator) +
				"!=" + parserInitExpr(rightBinaryOperator);
		}
		case BO_AddAssign: {
			return parserInitExpr(leftBinaryOperator) +
				"+=" + parserInitExpr(rightBinaryOperator);
		}
		default: {
			return "BinaryOperatorClass";
			break;
		}
		}
	}
	case Expr::ArraySubscriptExprClass: {
		const ArraySubscriptExpr *arraySubscriptExpr = dyn_cast<ArraySubscriptExpr>(stmt);
		const Expr *arrayBase = arraySubscriptExpr->getBase();
		std::string arrayName = parserInitExpr(arrayBase);
		//Expr *subExpr = arraySubscriptExpr->getRHS();
		auto dimStr = parserInitExpr(arraySubscriptExpr->getRHS());
		return arrayName + "[" + dimStr + "]";
	}
	case Expr::MemberExprClass: {
		const MemberExpr *memberExpr = dyn_cast<MemberExpr>(stmt);
		std::string className = "";
		auto cxxThisExprNode = memberExpr->getBase();
		className = parserInitExpr(cxxThisExprNode);
		auto fildName = memberExpr->getMemberDecl()->getDeclName().getAsString();
		auto stmtClassType = cxxThisExprNode->getStmtClass();
		if (stmtClassType == Expr::CXXThisExprClass) {
			//bool isCallExpr = judgeConstStIsCallExpr(stmt);
			return className + "->" + fildName;
		}
		auto type = cxxThisExprNode->getType();
		//auto classType = type->getTypeClass();
		if (type->getTypeClass() == 34) {
			QualType pointType = type->getPointeeType();
			if (pointType.getTypePtr()->isStructureOrClassType()) {
				return className + "->" + fildName;
			}

		}
		else if (type->getTypeClass() == 36) {
			std::string startStr = className.substr(0, 1);
			if (startStr == "*") {
				return className.substr(1, className.length()) + "->" + fildName;
			}
			else {
				return className + "." + fildName;
			}

		}
		else {
			return className + "." + fildName;
		}
	}
	case Expr::CXXThisExprClass: {
		const CXXThisExpr *cxxThisExpr = dyn_cast<CXXThisExpr>(stmt);
		//auto cxxThisExprType = cxxThisExpr->getType().getTypePtr();
		auto className = cxxThisExpr->getType().getBaseTypeIdentifier()->getName();
		std::string classNameStr = className;
		//auto a = cxxThisExprType->getTypeClass();
		return "p" + classNameStr;
	}
	case Expr::IntegerLiteralClass: {
		const IntegerLiteral *integerLiteral = dyn_cast<IntegerLiteral>(stmt);
		int intValue = integerLiteral->getValue().getZExtValue();
		return to_string(intValue);
	}
	case Expr::FloatingLiteralClass: {
		const FloatingLiteral *floatingLiteral = dyn_cast<FloatingLiteral>(stmt);
		double doubleValue = floatingLiteral->getValue().convertToDouble();
		/*llvm::SmallVector<char, 32> floatValue;
		floatingLiteral->getValue().toString(floatValue, 32, 0);*/
        std::string singleInitStr = dbl2str(doubleValue);
		/*std::stringstream ss;
		ss.precision(20);
		ss << doubleValue;*/
        return singleInitStr;
	}
	
	case Expr::CompoundAssignOperatorClass: {
		const CompoundAssignOperator *compoundAssignOperator =
			dyn_cast<CompoundAssignOperator>(stmt);
		Expr *leftCompondAssign = compoundAssignOperator->getLHS();
		Expr *rightCompondAssign = compoundAssignOperator->getRHS();
		return parserInitExpr(leftCompondAssign) +
			"+=" + parserInitExpr(rightCompondAssign);
	}
	case Expr::UnaryOperatorClass: {
		const UnaryOperator *unaryOperatorExpr = dyn_cast<UnaryOperator>(stmt);
		switch (unaryOperatorExpr->getOpcode()) {
		case UO_PostInc: {
			return parserInitExpr(unaryOperatorExpr->getSubExpr()) + "++";
		}
		case UO_PostDec: {
			return parserInitExpr(unaryOperatorExpr->getSubExpr()) + "--";
		}
		case UO_Minus: {
			return "-" + parserInitExpr(unaryOperatorExpr->getSubExpr());
		}
		case UO_Deref: {
			return "*" + parserInitExpr(unaryOperatorExpr->getSubExpr());
		}
		case UO_LNot: {
			return "!" + parserInitExpr(unaryOperatorExpr->getSubExpr());
		}
		case UO_AddrOf: {
			return "&" + parserInitExpr(unaryOperatorExpr->getSubExpr());
		}
		default: { return parserInitExpr(unaryOperatorExpr->getSubExpr()); }
		}
		break;
	}
	case Expr::DeclRefExprClass: {

		const DeclRefExpr *declRefExpr = dyn_cast<DeclRefExpr>(stmt);
		const ValueDecl *valueDecl = declRefExpr->getDecl();
		auto type = valueDecl->getType();
		if (type->getTypeClass() == 36) {
			/*bool isMemberCallExpr = judgeConstStIsMethodStmt(declRefExpr);
			if (isMemberCallExpr) {
				return type->getAsRecordDecl()->getNameAsString() + "_";
			}*/
			// return type->getAsRecordDecl()->getNameAsString() + ".";
			return valueDecl->getNameAsString() + ".";
		}
		/* 说明是指针类型 */
		else if (type->getTypeClass() == 34) {
			QualType pointType = type->getPointeeType();
			if (pointType.getTypePtr()->isStructureOrClassType()) {
				// return
				// pointType.getTypePtr()->getAsCXXRecordDecl()->getNameAsString() +
				// "->";
				return valueDecl->getNameAsString() + "->";
			}
		}
		//auto a = type->getTypeClass();
		//auto d = declRefExpr->isLValue();
		/*else if (type->getTypeClass() == 21) {
				bool isConstStIsCallExpr = judgeConstStIsCallExpr(stmt);
				if (isConstStIsCallExpr) {
						return valueDecl->getNameAsString();
				}
				else {
						return "";
				}
		}*/
		auto valueDeclStr = valueDecl->getNameAsString();
		if (valueDeclStr == "operator=") {
			return "=";
		}
		else if (type->getTypeClass() == 24) {
			return "*" + valueDecl->getNameAsString();
		}
		else {
			return valueDecl->getNameAsString();
		}
	}
	case Expr::WhileStmtClass: {
		const WhileStmt *whileStmt = dyn_cast<WhileStmt>(stmt);
		const Expr *whileCond = whileStmt->getCond();
		std::string whileCondStr = parserInitExpr(whileCond);
		const Stmt *whileBody = whileStmt->getBody();
		std::string whileBodyStr = parserInitExpr(whileBody);
		return "while(" + whileCondStr + "){\n" + whileBodyStr + "}";
	}
	case Expr::ReturnStmtClass: {
		const ReturnStmt *returnStmt = dyn_cast<ReturnStmt>(stmt);
		const Expr *expr = returnStmt->getRetValue();
		if (expr) {
			return "return " + parserInitExpr(expr) + ";";
		}
		else {
			return "return;\n";
		}
	}
								/*case Expr::CXXMemberCallExprClass: {
										CXXMemberCallExpr* cXXMemberCallExpr =
								dyn_cast<CXXMemberCallExpr>(stmt); FunctionDecl* cXX_func_decl
								=cXXMemberCallExpr->getDirectCallee();
								}*/
	
	case Expr::CXXConstructExprClass: {
		const CXXConstructExpr *cxxConstructExpr = dyn_cast<CXXConstructExpr>(stmt);
		//auto a = cxxConstructExpr->getArgs();
		std::string constructExprStr = "";
		for (auto subConstructExpr = cxxConstructExpr->child_begin();
			subConstructExpr != cxxConstructExpr->child_end();
			subConstructExpr++) {
			const Stmt *subConStmt = *subConstructExpr;
			constructExprStr += parserInitExpr(subConStmt);
		}
		return constructExprStr;
	}
	case Expr::UnaryExprOrTypeTraitExprClass: {
		const UnaryExprOrTypeTraitExpr *unaryExprOrTypeTraitExpr =
			dyn_cast<UnaryExprOrTypeTraitExpr>(stmt);
		if (unaryExprOrTypeTraitExpr->isArgumentType())
		{
			auto sizeType = unaryExprOrTypeTraitExpr->getArgumentType();
			if (sizeType.getTypePtr()) {
				auto sizeNameStr =
					sizeType.getTypePtr()->getAsCXXRecordDecl()->getNameAsString();
				return "sizeof(" + sizeNameStr + ")";
			}
		}
		else {
			auto unaryExprOrTypeTraitExprArgu = unaryExprOrTypeTraitExpr->getArgumentExpr();
			auto sizeNameStr = parserInitExpr(unaryExprOrTypeTraitExprArgu);
			return "sizeof(" + sizeNameStr + ")";
		}

		// Expr*
		// 
		// clang::Expr::EvalResult integer;
		// unaryExprOrTypeTraitExpr.
		// unaryExprOrTypeTraitExpr->EvaluateAsInt(integer,*Context,);

	}

	default: { return ""; }
	}
}