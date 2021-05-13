#pragma once
#include "clang/Frontend/TextDiagnosticPrinter.h"

using namespace clang;

/*����������Ϣ�б� */
struct FileErrorInfo {
	const char *errorLoc;  /*����λ��*/
	const char *errorInfo; /*������Ϣ*/
};

class MyDiagnosticConsumer : public TextDiagnosticPrinter {
public:
	MyDiagnosticConsumer(raw_ostream &os, DiagnosticOptions *diags) : TextDiagnosticPrinter(os, diags) {}

	void HandleDiagnostic(DiagnosticsEngine::Level DiagLevel, const Diagnostic &Info) override;
};

/*������Ϣ*/
extern std::vector<FileErrorInfo> fileErrors;

