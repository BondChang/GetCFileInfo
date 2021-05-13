#pragma once

#include "CEntityDeal.h"

using namespace clang;
using namespace clang::ast_matchers; 


class CInfoMatcher : public MatchFinder::MatchCallback {
public:
	virtual void run(const MatchFinder::MatchResult &Result);
};

extern int varIndex;
extern int funIndex;
extern int structIndex;

extern CFileInfoStruct *cfileInfo;
