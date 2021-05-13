#pragma once
#include "clang/Frontend/TextDiagnosticPrinter.h"

using namespace clang;

/*函数错误信息列表 */
struct FileErrorInfo {
	const char *errorLoc;  /*错误位置*/
	const char *errorInfo; /*错误信息*/
};

class MyDiagnosticConsumer : public TextDiagnosticPrinter {
public:
	MyDiagnosticConsumer(raw_ostream &os, DiagnosticOptions *diags) : TextDiagnosticPrinter(os, diags) {}

	void HandleDiagnostic(DiagnosticsEngine::Level DiagLevel, const Diagnostic &Info) override;
};

/*错误信息*/
extern std::vector<FileErrorInfo> fileErrors;

