#include "tinyxml/include/tinyxml/tinyxml.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Sema/SemaInternal.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/CommandLine.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include "GetCFileInfo.h"
#include "CInfoUtil.h"
#include "CInfoMatcher.h"
#include "parserInitExpr.h"
#include "CEntityDeal.h"
#include "MyDiagnosticConsumer.h"
#include "../SunwiseInfoUtil/SunwiseInfoUtil.h"

using namespace clang::tooling;
using namespace llvm;
using namespace clang;
using namespace clang::ast_matchers;

// using namespace std::filesystem;

// Apply a custom category to all command-line options so that they are the
// only ones displayed.
static llvm::cl::OptionCategory MyToolCategory("global-detect options");
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);
// A help message for this specific tool can be added afterwards.
static cl::extrahelp MoreHelp("\nMore help text...\n");

int paraIndex = -1;
std::string dimStr;

CFileInfoStruct *cfileInfoAll;

/* 输出获取的c文件信息 */
void printCFileInfo();

void printCFileInfo() {
  /* 输出所有的全局变量信息 */
  for (int i = 0; i <= varIndex; i++) {
    std::cout << "全局变量的名称是 " << cfileInfo->globalVars[i].name << "\n";
    std::cout << "全局变量的修饰符是 " << cfileInfo->globalVars[i].qualifier
              << "\n";
    std::cout << "全局变量的类型是 " << cfileInfo->globalVars[i].type << "\n";
    std::cout << "全局变量的维度是 " << cfileInfo->globalVars[i].dim << "\n";
    std::cout << "全局变量的初值是 " << cfileInfo->globalVars[i].initValue
              << "\n";
  }
  /* 输出所有的结构体信息 */
  for (int i = 0; i <= structIndex; i++) {

    std::cout << "结构体的名称是 " << cfileInfo->structs[i].name << "\n";
    std::cout << "结构体成员变量的数量是 " << cfileInfo->structs[i].fieldCount
              << "\n";
    std::cout << "结构体的修饰符是 " << cfileInfo->structs[i].qualifier << "\n";

    for (int j = 0; j < cfileInfo->structs[i].fieldCount; j++) {
      std::cout << "结构体成员变量的名称是 "
                << cfileInfo->structs[i].fields[j].name << "\n";
      std::cout << "结构体成员变量的修饰符是 "
                << cfileInfo->structs[i].fields[j].qualifier << "\n";
      std::cout << "结构体成员变量的类型是 "
                << cfileInfo->structs[i].fields[j].type << "\n";

      std::cout << "结构体成员变量的维度是 "
                << cfileInfo->structs[i].fields[j].dim << "\n";
    }
  }
  /* 输出所有的函数列表信息 */
  for (int i = 0; i <= funIndex; i++) {
    std::cout << "函数的名称为 " << cfileInfo->functions[i].name << "\n";
    std::cout << "函数的返回值类型为 " << cfileInfo->functions[i].type << "\n";
    std::cout << "函数的参数个数为 " << cfileInfo->functions[i].paraCount
              << "\n";
    for (int j = 0; j < cfileInfo->functions[i].paraCount; j++) {
      std::cout << "函数的参数列表名称为 "
                << cfileInfo->functions[i].paras[j].name << "\n";
      std::cout << "函数的参数列表修饰符为 "
                << cfileInfo->functions[i].paras[j].qualifier << "\n";
      std::cout << "函数的参数列表类型为 "
                << cfileInfo->functions[i].paras[j].type << "\n";
      std::cout << "函数的参数列表维度为 "
                << cfileInfo->functions[i].paras[j].dim << "\n";
    }
  }

  return;
}
/* 写入节点的位置信息 */
void writeNodeLocInfo(TiXmlElement *element,
                      std::vector<VariableInfoStruct>::iterator iter) {
  element->SetAttribute("startLine", (*iter).nodeLocInfo.StartLine);
  element->SetAttribute("endLine", (*iter).nodeLocInfo.EndLine);
  element->SetAttribute("startCol", (*iter).nodeLocInfo.StartCol);
  element->SetAttribute("endCol", (*iter).nodeLocInfo.EndCol);
}

void writeCOrHInfo2Xml(const char **filePathList, int fileCount, int argc,
                       const char **argv, const char *targetPathDir,
                       const char *writePath) {
  cfileInfoAll = new CFileInfoStruct[fileCount];
  basePath = targetPathDir;
  // CFileInfoStruct *cfileInfo = new CFileInfoStruct[sizeof(fileCount)];
  for (int i = 0; i < fileCount; i++) {
    cfileInfo = &cfileInfoAll[i];
    getCFileInfo(filePathList[i], argc, argv);
    // std::cout << "正在解析第" << i << "个" << "\n";
    for (auto iter = cfileInfo->everyFileExistPath.begin();
         iter != cfileInfo->everyFileExistPath.end(); iter++) {
      allExistPath.push_back((*iter));
    }

  }

  // write2xml
  using namespace std;
  const char *xmlFile = writePath;
  TiXmlDocument doc;
  TiXmlDeclaration *decl = new TiXmlDeclaration("1.0", "", "");
  TiXmlElement *resources = new TiXmlElement("CInfo");
  TiXmlElement *dictsElement = new TiXmlElement("dicts");
  TiXmlElement *structsElement = new TiXmlElement("structs");
  TiXmlElement *funsElement = new TiXmlElement("functions");
  TiXmlElement *errorsElement = new TiXmlElement("errors");
  for (int i = 0; i < fileCount; i++) {
    cfileInfo = &cfileInfoAll[i];
    /* 写入全局变量 */
    for (auto iter = cfileInfo->globalVars.begin();
         iter != cfileInfo->globalVars.end(); iter++) {
      TiXmlElement *dictElement = new TiXmlElement("DictItem");
      dictElement->SetAttribute("name", (*iter).name);
      dictElement->SetAttribute("qualifier", (*iter).qualifier);
      dictElement->SetAttribute("type", (*iter).type);
      dictElement->SetAttribute("dim", (*iter).dim);
      dictElement->SetAttribute("path", (*iter).path);
      dictElement->SetAttribute("hPath", (*iter).hPath);
      dictElement->SetAttribute("initValue", (*iter).initValue);
      dictElement->SetAttribute("isPointerType", (*iter).pointerType);
      if ((*iter).initAstStruct.size() == 1) {
        TiXmlElement *initValueAst = new TiXmlElement("initValueAst");
        std::vector<InitAstStruct>::iterator startValue =
            (*iter).initAstStruct.begin();
        if (startValue->isStructOrArrayBegin) {
          dealComplexValue(initValueAst, *startValue);
        } else {
          TiXmlElement *expression = new TiXmlElement("expression");
          expression->SetAttribute("initValue",
                                   (startValue)->initValueAst.c_str());
          expression->SetAttribute("startLine",
                                   (startValue)->nodeLocInfo.StartLine);
          expression->SetAttribute("endLine",
                                   (startValue)->nodeLocInfo.EndLine);
          expression->SetAttribute("startCol",
                                   (startValue)->nodeLocInfo.StartCol);
          expression->SetAttribute("endCol", (startValue)->nodeLocInfo.EndCol);
          initValueAst->LinkEndChild(expression);
        }
        dictElement->LinkEndChild(initValueAst);
      }

      writeNodeLocInfo(dictElement, iter);
      dictsElement->LinkEndChild(dictElement);
    }
  }

  /* 写入结构体 */
  for (int i = 0; i < fileCount; i++) {
    cfileInfo = &cfileInfoAll[i];
    for (auto iter = cfileInfo->structs.begin();
         iter != cfileInfo->structs.end(); iter++) {
      TiXmlElement *structElement = new TiXmlElement("StructItem");
      structElement->SetAttribute("name", (*iter).name);
      structElement->SetAttribute("type", (*iter).type);
      structElement->SetAttribute("qualifier", (*iter).qualifier);
      structElement->SetAttribute("path", (*iter).path);

      structElement->SetAttribute("startLine", (*iter).nodeLocInfo.StartLine);
      structElement->SetAttribute("endLine", (*iter).nodeLocInfo.EndLine);
      structElement->SetAttribute("startCol", (*iter).nodeLocInfo.StartCol);
      structElement->SetAttribute("endCol", (*iter).nodeLocInfo.EndCol);
      TiXmlElement *structItems = new TiXmlElement("children");
      for (auto subIter = (*iter).fields.begin();
           subIter != (*iter).fields.end(); subIter++) {
        TiXmlElement *structItem = new TiXmlElement("StructItem");
        structItem->SetAttribute("name", (*subIter).name);
        structItem->SetAttribute("qualifier", (*subIter).qualifier);
        structItem->SetAttribute("type", (*subIter).type);
        structItem->SetAttribute("dim", (*subIter).dim);
        structItem->SetAttribute("isPointerType", (*subIter).pointerType);
        writeNodeLocInfo(structItem, subIter);
        structItems->LinkEndChild(structItem);
      }
      structElement->LinkEndChild(structItems);
      structsElement->LinkEndChild(structElement);
    }
  }
  /* 写入函数信息 */
  for (int i = 0; i < fileCount; i++) {
    cfileInfo = &cfileInfoAll[i];
    for (auto iter = cfileInfo->functions.begin();
         iter != cfileInfo->functions.end(); iter++) {
      TiXmlElement *funElement = new TiXmlElement("FunctionsItem");
      funElement->SetAttribute("name", (*iter).name);
      funElement->SetAttribute("returnType", (*iter).type);
      funElement->SetAttribute("path", (*iter).path);
      funElement->SetAttribute("hPath", (*iter).hPath);
      funElement->SetAttribute("startLine", (*iter).nodeLocInfo.StartLine);
      funElement->SetAttribute("endLine", (*iter).nodeLocInfo.EndLine);
      funElement->SetAttribute("startCol", (*iter).nodeLocInfo.StartCol);
      funElement->SetAttribute("endCol", (*iter).nodeLocInfo.EndCol);
      TiXmlElement *parasItem = new TiXmlElement("parameters");
      for (auto subIter = (*iter).paras.begin(); subIter != (*iter).paras.end();
           subIter++) {
        TiXmlElement *paraItem = new TiXmlElement("ParametersItem");
        paraItem->SetAttribute("name", (*subIter).name);
        paraItem->SetAttribute("qualifier", (*subIter).qualifier);
        paraItem->SetAttribute("type", (*subIter).type);
        paraItem->SetAttribute("dim", (*subIter).dim);
        paraItem->SetAttribute("isPointerType", (*subIter).pointerType);
        writeNodeLocInfo(paraItem, subIter);
        parasItem->LinkEndChild(paraItem);
      }
      funElement->LinkEndChild(parasItem);
      funsElement->LinkEndChild(funElement);
    }
  }
  /* 写入错误信息 */
  for (int i = 0; i < fileCount; i++) {
    for (auto iter = fileErrors.begin();
         iter != fileErrors.end(); iter++) {
      TiXmlElement *ErrorItem = new TiXmlElement("ErrorItem");
      ErrorItem->SetAttribute("location", (*iter).errorLoc);
      ErrorItem->SetAttribute("description", (*iter).errorInfo);
      errorsElement->LinkEndChild(ErrorItem);
    }
  }
  resources->LinkEndChild(dictsElement);
  resources->LinkEndChild(structsElement);
  resources->LinkEndChild(funsElement);
  resources->LinkEndChild(errorsElement);
  doc.LinkEndChild(decl);
  doc.LinkEndChild(resources);
  doc.SaveFile(xmlFile);
  // delete cfileInfoAll;
  delete[] cfileInfoAll;
  allExistPath.clear();
  globalNames.clear();
}

void getCFileInfo(const char *filePath, int argc, const char **argv) {
  varIndex = -1;
  structIndex = -1;
  funIndex = -1;
  paraIndex = -1;
  //fieldCount = -1;
  // initStruct();
  std::vector<std::string> fileList;
  fileList.push_back(filePath);
  CommonOptionsParser OptionsParser(argc, argv, MyToolCategory);
  ClangTool Tool(OptionsParser.getCompilations(), fileList);
  CInfoMatcher GvarPrinter;
  MatchFinder Finder;

  //clang::ast_matchers::internal::BindableMatcher<clang::Decl> varDeclMatcher = varDecl(hasGlobalStorage());
  //clang::ast_matchers::DeclarationMatcher declMatcher = varDeclMatcher.bind("globalVars");
  Finder.addMatcher(varDecl(hasGlobalStorage()).bind("globalVars"), &GvarPrinter);

  Finder.addMatcher(functionDecl(unless(hasAncestor(recordDecl()))).bind("functionParas"), &GvarPrinter);

  Finder.addMatcher(recordDecl(recordDecl(isStruct()).bind("structs")).bind("fields"), &GvarPrinter);

  Finder.addMatcher(recordDecl(recordDecl(isUnion()).bind("unions")).bind("unionFields"), &GvarPrinter);
  clang::DiagnosticOptions *Options = new clang::DiagnosticOptions();
  MyDiagnosticConsumer *diagConsumer = new MyDiagnosticConsumer(llvm::errs(), Options);
  Tool.setDiagnosticConsumer(diagConsumer);
  std::unique_ptr<FrontendActionFactory> factory = newFrontendActionFactory(&Finder);
  clang::tooling::ToolAction *pToolAction = factory.get();
  Tool.run(pToolAction);

  cfileInfo->path = filePath;
  cfileInfo->globalVarsCount = varIndex + 1;
  cfileInfo->structCount = structIndex + 1;
  cfileInfo->funCount = funIndex + 1;

  return;
}
// int main(int argc, const char **argv) {
//
//  const char *parserFilePathList[1] = {
//
//      "C:\\Users\\bondc\\Desktop\\a\\a.c"
//     };
//  int parserFileCount = 1;
//  int preArgc = 3;
//  const char *preArgv[3] = {"",
//                             "",
//                             "--"};
//  const char *basicDir = "C:\\Users\\bondc\\Desktop\\a\\a.c";
//  // const char* basicDir =
//  // "C:\\workspaceVM\\llvm_vc2017\\tools\\clang\\tools\\extra\\"; const char*
//  // basicDir =
//  // "C:\\workspaceVM\\llvm_vc2017\\tools\\clang\\tools\\extra\\code4Test\\";
//  const char *writeFileInfoPath = "C:\\Users\\bondc\\Desktop\\cFileInfo222.xml";
//  // const char* writeCommentPath = "comment.xml";
//
//  // getFileInfo(parserFilePathList, parserFileCount, argc, argv, basicDir,
//  // writeFileInfoPath);
//
//  writeCOrHInfo2Xml(parserFilePathList, parserFileCount, preArgc, preArgv,
//                    basicDir, writeFileInfoPath);
//
//  // getFileComment(parserFilePathList, parserFileCount, argc, argv, basicDir,
//  // writeCommentPath);
//
//  return 0;
//}

int main(int argc, const char **argv) {
	const char *parserFilePathList[160] = {
	"C:/Users/bondc/Desktop/target/code/sys/arch/config/BSPInit.c",
  "C:/Users/bondc/Desktop/target/code/sys/arch/config/BSPPrint.c",
  "C:/Users/bondc/Desktop/target/code/sys/arch/sip2115/src/BSPInitCPU.c",
  "C:/Users/bondc/Desktop/target/code/sys/drv/src/DRV_ADDA.c",
  "C:/Users/bondc/Desktop/target/code/sys/drv/src/DRV_BIT.c",
  "C:/Users/bondc/Desktop/target/code/sys/drv/src/DRV_CAN.c",
  "C:/Users/bondc/Desktop/target/code/sys/drv/src/DRV_COM.c",
  "C:/Users/bondc/Desktop/target/code/sys/drv/src/DRV_EEPROM.c",
  "C:/Users/bondc/Desktop/target/code/sys/drv/src/DRV_NAND_FLASH.c",
  "C:/Users/bondc/Desktop/target/code/sys/drv/src/DRV_NorFLASH.c",
  "C:/Users/bondc/Desktop/target/code/sys/drv/src/DRV_PLUSE.c",
  "C:/Users/bondc/Desktop/target/code/sys/drv/src/DRV_TIME.c",
  "C:/Users/bondc/Desktop/target/code/sys/drv/src/DRV_UART.c",
  "C:/Users/bondc/Desktop/target/code/sys/drv/src/DRV_UsrDef.c",
  "C:/Users/bondc/Desktop/target/code/sys/lib/src/BSPLib.c",
  "C:/Users/bondc/Desktop/target/code/sys/lib/src/SYSCacheLib.c",
  "C:/Users/bondc/Desktop/target/code/sys/lib/src/SYSLib.c",
  "C:/Users/bondc/Desktop/target/code/sys/os/src/OSConfig.c",
  "C:/Users/bondc/Desktop/target/code/sys/os/src/OSConsole.c",
  "C:/Users/bondc/Desktop/target/code/sys/os/src/OSInfoRec.c",
  "C:/Users/bondc/Desktop/target/code/sys/os/src/OSInt.c",
  "C:/Users/bondc/Desktop/target/code/sys/os/src/OSKernel.c",
  "C:/Users/bondc/Desktop/target/code/sys/os/src/OSMem.c",
  "C:/Users/bondc/Desktop/target/code/sys/os/src/OSMq.c",
  "C:/Users/bondc/Desktop/target/code/sys/os/src/OSSem.c",
  "C:/Users/bondc/Desktop/target/code/sys/os/src/OSTask.c",
  "C:/Users/bondc/Desktop/target/code/sys/os/src/OSTime.c",
  "C:/Users/bondc/Desktop/target/code/sys/os/src/QLib.c",
  "C:/Users/bondc/Desktop/target/code/sys/sys/src/SYSEEprom.c",
  "C:/Users/bondc/Desktop/target/code/sys/sys/src/SYSHook.c",
  "C:/Users/bondc/Desktop/target/code/sys/sys/src/SYSInterface.c",
  "C:/Users/bondc/Desktop/target/code/sys/sys/src/SYSMain.c",
  "C:/Users/bondc/Desktop/target/code/sys/sys/src/SYSNandFlash.c",
  "C:/Users/bondc/Desktop/target/code/sys/sys/src/SYSNorFlash.c",
  "C:/Users/bondc/Desktop/target/code/sys/sys/src/SYSRom.c",
  "C:/Users/bondc/Desktop/target/code/sys/sys/src/SYSTask.c",
  "C:/Users/bondc/Desktop/target/code/sys/sys/src/SYSTmHandler.c",
  "C:/Users/bondc/Desktop/target/code/sys/sys/src/SYSTrapHandler.c",
  "C:/Users/bondc/Desktop/target/code/sys/sys/src/SYSTrapHook.c",
  "C:/Users/bondc/Desktop/target/code/sys/sys/src/SYSUsrDef.c",
  "C:/Users/bondc/Desktop/target/code/sys/sys/src/SYSVariables.c",
  "C:/Users/bondc/Desktop/target/code/sys/sys/src/SYSWatchDog.c",
  "C:/Users/bondc/Desktop/target/code/sys/util/src/UTILLib.c",
  "C:/Users/bondc/Desktop/target/code/usr/0.common/common.c",
  "C:/Users/bondc/Desktop/target/code/usr/0.common/std_lib/algor/std_ring.c",
  "C:/Users/bondc/Desktop/target/code/usr/0.common/std_lib/std_addr.c",
  "C:/Users/bondc/Desktop/target/code/usr/0.common/std_lib/std_const.c",
  "C:/Users/bondc/Desktop/target/code/usr/0.common/std_lib/std_queue.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/config/cfg_a1_orbit.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/config/cfg_a2_attidetermine.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/config/cfg_a3_ctrl.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/config/cfg_a5_wholeatticapture.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/config/cfg_a6_modeconvert.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/config/cfg_m1_part.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/config/cfg_m2_startime.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/config/cfg_m3_power.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/config/cfg_p1_tc.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/config/cfg_readme.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/proj/hongyan1sapp.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/proj/hongyan1s_indirectcmd.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/app/a1_Orbit.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/app/a2_AttiDetermine_IRES.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/app/a2_AttiDetermine_IRESSUN.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/app/a2_AttiDetermine_maneuver.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/app/a2_AttiDetermine_STS.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/app/a3_control_antenna.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/app/a3_control_bapta.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/app/a3_control_MT.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/app/a3_Control_MW.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/app/a3_control_propel.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/app/a4_Components_sensorbroken.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/app/a4_Components_sensorvalid.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/app/a5_wholeatticapture.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/app/a6_ModeConvert.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/app/a7_ModeCheck.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/app/a7_ModeExecute.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/app.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/arith/gfhacctrack.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/arith/gfhbangbang.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/arith/gfhBaptaCaptureAndTrace.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/arith/gfhBiasThreeAxisMWCtl.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/arith/gfhcalstation.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/arith/gfhestimateq.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/arith/gfheulerestimate.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/arith/gfhgravitygradienttorque.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/arith/gfhgyrodiagbywqs.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/arith/gfhGyroEquation.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/arith/gfhiresdss2v.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/arith/gfhjetfaultdiag.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/arith/gfhmagneticcalc.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/arith/gfhmtuninstallvolt.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/arith/gfhMWFaultJudge.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/arith/gfhorbit.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/arith/gfhOrbitLunarPos.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/arith/gfhphase.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/arith/gfhsgattacq.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/arith/gfhsgselect.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/arith/gfhsintrack.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/arith/gfhstructfilter.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/arith/gfhstsgyromodify.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/arith/gfhstsmodify.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/arith/gfhstsonboardcalbt.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/arith/gfhzeromomentumcalc.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/mission/DevControl.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/mission/mission.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/mission/modeMission.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/mission/module_DAP.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/mission/startime.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/tctm/t1_interface.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/tctm/t1_tc_critical.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/tctm/t1_tc_indirectcmd.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/tctm/t1_tc_inject.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/tctm/t2_tm_alltm.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/tctm/t2_tm_broad.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/tctm/t2_tm_critical.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/tctm/t2_tm_subtm.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/tctm/TMPackagingBasic.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/tctm/TMPackagingUserBuffer.c",
  "C:/Users/bondc/Desktop/target/code/usr/1.application/share/tctm/TMPackagingUserFunc.c",
  "C:/Users/bondc/Desktop/target/code/usr/2.device/config/cfg_aocc.c",
  "C:/Users/bondc/Desktop/target/code/usr/2.device/config/cfg_ass.c",
  "C:/Users/bondc/Desktop/target/code/usr/2.device/config/cfg_dss.c",
  "C:/Users/bondc/Desktop/target/code/usr/2.device/config/cfg_ground.c",
  "C:/Users/bondc/Desktop/target/code/usr/2.device/config/cfg_gyro.c",
  "C:/Users/bondc/Desktop/target/code/usr/2.device/config/cfg_ires.c",
  "C:/Users/bondc/Desktop/target/code/usr/2.device/config/cfg_magm.c",
  "C:/Users/bondc/Desktop/target/code/usr/2.device/config/cfg_mt.c",
  "C:/Users/bondc/Desktop/target/code/usr/2.device/config/cfg_mw.c",
  "C:/Users/bondc/Desktop/target/code/usr/2.device/config/cfg_pd.c",
  "C:/Users/bondc/Desktop/target/code/usr/2.device/config/cfg_ppcu.c",
  "C:/Users/bondc/Desktop/target/code/usr/2.device/config/cfg_prop.c",
  "C:/Users/bondc/Desktop/target/code/usr/2.device/config/cfg_sada.c",
  "C:/Users/bondc/Desktop/target/code/usr/2.device/config/cfg_sts.c",
  "C:/Users/bondc/Desktop/target/code/usr/2.device/share/aocc/aocc.c",
  "C:/Users/bondc/Desktop/target/code/usr/2.device/share/device.c",
  "C:/Users/bondc/Desktop/target/code/usr/2.device/share/device_common.c",
  "C:/Users/bondc/Desktop/target/code/usr/2.device/share/OBDH/OBDH.c",
  "C:/Users/bondc/Desktop/target/code/usr/2.device/share/OBDH/OBDHCfg.c",
  "C:/Users/bondc/Desktop/target/code/usr/2.device/share/part/ass/ass.c",
  "C:/Users/bondc/Desktop/target/code/usr/2.device/share/part/demo.c",
  "C:/Users/bondc/Desktop/target/code/usr/2.device/share/part/dss/dss.c",
  "C:/Users/bondc/Desktop/target/code/usr/2.device/share/part/ground/ground.c",
  "C:/Users/bondc/Desktop/target/code/usr/2.device/share/part/gyro/gyro.c",
  "C:/Users/bondc/Desktop/target/code/usr/2.device/share/part/ires/ires.c",
  "C:/Users/bondc/Desktop/target/code/usr/2.device/share/part/magm/magm.c",
  "C:/Users/bondc/Desktop/target/code/usr/2.device/share/part/mt/mt.c",
  "C:/Users/bondc/Desktop/target/code/usr/2.device/share/part/mw/mw.c",
  "C:/Users/bondc/Desktop/target/code/usr/2.device/share/part/pd/pd.c",
  "C:/Users/bondc/Desktop/target/code/usr/2.device/share/part/ppcu/ppcu.c",
  "C:/Users/bondc/Desktop/target/code/usr/2.device/share/part/prop/prop.c",
  "C:/Users/bondc/Desktop/target/code/usr/2.device/share/part/sada/sada.c",
  "C:/Users/bondc/Desktop/target/code/usr/2.device/share/part/sts/sts.c",
  "C:/Users/bondc/Desktop/target/code/usr/3.comif/share/adda.c",
  "C:/Users/bondc/Desktop/target/code/usr/3.comif/share/bit.c",
  "C:/Users/bondc/Desktop/target/code/usr/3.comif/share/can.c",
  "C:/Users/bondc/Desktop/target/code/usr/3.comif/share/ComIf.c",
  "C:/Users/bondc/Desktop/target/code/usr/3.comif/share/fpga.c",
  "C:/Users/bondc/Desktop/target/code/usr/3.comif/share/pulse.c",
  "C:/Users/bondc/Desktop/target/code/usr/3.comif/share/time.c",
  "C:/Users/bondc/Desktop/target/code/usr/3.comif/share/uart.c",
	};
	int parserFileCount = 160;
	int preArgc = 37;
	const char *preArgv[37] = {
		"",
		"",
		"--",
		"-IC:/Users/bondc/Desktop/target/code/sys/arch/config",
		"-IC:/Users/bondc/Desktop/target/code/sys/arch/sip2115/h",
		"-IC:/Users/bondc/Desktop/target/code/sys/drv/h",
		"-IC:/Users/bondc/Desktop/target/code/sys/lib/h",
		"-IC:/Users/bondc/Desktop/target/code/sys/os/h",
		"-IC:/Users/bondc/Desktop/target/code/sys/sys/h",
		"-IC:/Users/bondc/Desktop/target/code/sys/util/h",
		"-IC:/Users/bondc/Desktop/target/code/usr/0.common/std_lib/algor",
		"-IC:/Users/bondc/Desktop/target/code/usr/0.common/std_lib",
		"-IC:/Users/bondc/Desktop/target/code/usr/0.common",
		"-IC:/Users/bondc/Desktop/target/code/usr/1.application/config",
		"-IC:/Users/bondc/Desktop/target/code/usr/1.application/proj",
		"-IC:/Users/bondc/Desktop/target/code/usr/1.application/share/app",
		"-IC:/Users/bondc/Desktop/target/code/usr/1.application/share/arith",
		"-IC:/Users/bondc/Desktop/target/code/usr/1.application/share/mission",
		"-IC:/Users/bondc/Desktop/target/code/usr/1.application/share/tctm",
		"-IC:/Users/bondc/Desktop/target/code/usr/1.application/share",
		"-IC:/Users/bondc/Desktop/target/code/usr/2.device/share/aocc",
		"-IC:/Users/bondc/Desktop/target/code/usr/2.device/share/OBDH",
		"-IC:/Users/bondc/Desktop/target/code/usr/2.device/share/part/ass",
		"-IC:/Users/bondc/Desktop/target/code/usr/2.device/share/part/dss",
		"-IC:/Users/bondc/Desktop/target/code/usr/2.device/share/part/ground",
		"-IC:/Users/bondc/Desktop/target/code/usr/2.device/share/part/gyro",
		"-IC:/Users/bondc/Desktop/target/code/usr/2.device/share/part/ires",
		"-IC:/Users/bondc/Desktop/target/code/usr/2.device/share/part/magm",
		"-IC:/Users/bondc/Desktop/target/code/usr/2.device/share/part/mt",
		"-IC:/Users/bondc/Desktop/target/code/usr/2.device/share/part/mw",
		"-IC:/Users/bondc/Desktop/target/code/usr/2.device/share/part/pd",
		"-IC:/Users/bondc/Desktop/target/code/usr/2.device/share/part/ppcu",
		"-IC:/Users/bondc/Desktop/target/code/usr/2.device/share/part/prop",
		"-IC:/Users/bondc/Desktop/target/code/usr/2.device/share/part/sada",
		"-IC:/Users/bondc/Desktop/target/code/usr/2.device/share/part/sts",
		"-IC:/Users/bondc/Desktop/target/code/usr/2.device/share",
		"-IC:/Users/bondc/Desktop/target/code/usr/3.comif/share",
	};

	const char *basicDir = "C:/Users/bondc/Desktop/target/code";
	const char *writeFileInfoPath = "cFileInfo_misrsat_2.xml";

	writeCOrHInfo2Xml(parserFilePathList, parserFileCount, preArgc, preArgv,
		basicDir, writeFileInfoPath);

	return 0;
}


