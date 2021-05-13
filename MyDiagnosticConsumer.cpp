#include "MyDiagnosticConsumer.h"

/*¥ÌŒÛ–≈œ¢*/
std::vector<FileErrorInfo> fileErrors;

void MyDiagnosticConsumer::HandleDiagnostic(DiagnosticsEngine::Level DiagLevel, const Diagnostic &Info) {
	TextDiagnosticPrinter::HandleDiagnostic(DiagLevel, Info);
	const DiagnosticsEngine *diagnosticsEngine = Info.getDiags();
	auto diagnosticOptions = diagnosticsEngine->getDiagnosticOptions();
	SmallString<100> smallErrorStr;

	StringRef errorStrRef = Info.getDiags()->getDiagnosticIDs()->getDescription(Info.getID());
	Info.FormatDiagnostic(errorStrRef.begin(), errorStrRef.end(), smallErrorStr);
	std::string errorInfo = smallErrorStr.c_str();
	std::string errorLoc = Info.getLocation().printToString(Info.getSourceManager());
	FileErrorInfo fileErrorInfo;
	fileErrorInfo.errorInfo = strcpy(new char[errorInfo.length() + 1], errorInfo.data());
	fileErrorInfo.errorLoc = strcpy(new char[errorLoc.length() + 1], errorLoc.data());
	fileErrors.push_back(fileErrorInfo);

	return;
}
