//==============================================================================
// DESCRIPTION: ArgStates
//
// USAGE:
//    1. As a loadable Clang plugin:
//      clang -cc1 -load <BUILD_DIR>/lib/libArgStates.dylib  -plugin  '\'
//      ArgStates -plugin-arg-ArgStates -class-name '\'
//      -plugin-arg-ArgStates Base  -plugin-arg-ArgStates -old-name '\'
//      -plugin-arg-ArgStates run  -plugin-arg-ArgStates -new-name '\'
//      -plugin-arg-ArgStates walk test/ArgStates_Class.cpp
//    2. As a standalone tool:
//       <BUILD_DIR>/bin/ct-code-refactor --class-name=Base --new-name=walk '\'
//        --old-name=run test/ArgStates_Class.cpp
//
//==============================================================================
#include "ArgStates.hpp"

#include "clang/AST/Expr.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/Tooling/Refactoring/Rename/RenamingAction.h"
#include "llvm/Support/raw_ostream.h"
#include <string>
#include <fstream>
#include <unordered_set>

using namespace clang;
using namespace ast_matchers;

//-----------------------------------------------------------------------------
// ArgStatesMatcher - implementation
// Add the suffix to matched items
//-----------------------------------------------------------------------------

void ArgStatesMatcher::matchArgs(
  const MatchFinder::MatchResult &result, std::string bindName) {

}

void ArgStatesMatcher::run(const MatchFinder::MatchResult &result) {
    const auto srcMgr = result.SourceManager;

    //const auto *func = result.Nodes.getNodeAs<FunctionDecl>("FUNC");
    const auto *call = result.Nodes.getNodeAs<CallExpr>("CALL");

    if (call) {
      //const auto location = srcMgr->getFileLoc(call->getEndLoc());

      #if DEBUG_AST
        call->dumpColor();
      #endif
    }
}

void ArgStatesMatcher::onEndOfTranslationUnit() {
  // Output to stdout
  //ArgStatesRewriter
  //    .getEditBuffer(ArgStatesRewriter.getSourceMgr().getMainFileID())
  //    .write(llvm::outs());
}

//-----------------------------------------------------------------------------
// ArgStatesASTConsumer- implementation
//  https://clang.llvm.org/docs/LibASTMatchersTutorial.html
// Specifies the node patterns that we want to analyze further in ::run()
//-----------------------------------------------------------------------------

ArgStatesASTConsumer::ArgStatesASTConsumer(
    Rewriter &R, std::vector<std::string> Names
    ) : ArgStatesHandler(R), Names(Names) {
  // We want to match agianst all variable refernces which are later passed
  // to one of the changed functions in the Names array
  //
  // As a starting point, we want to match the FunctionDecl nodes of the enclosing
  // functions for any call to a changed function. From this node we can then
  // continue downwards until we reach the actual call of the changed function,
  // while recording all declared variables and saving the state of those which end up being used
  //
  // If we match the call experssions directly we would need to backtrack in the AST to find information
  // on what each variable holds

  #if DEBUG_AST
  llvm::errs() << "\033[33m!>\033[0m Processing " <<  Names[0] << "\n";
  #endif

  // To access the parameters to a call we need to match the actual call experssion
  // The first child of the call expression is a declRefExpr to the function being invoked
  // Match references to the changed function
  const auto matcherForCall = callExpr(
    callee(
      functionDecl(hasName(Names[0])).bind("FUNC")
    )
  ).bind("CALL");

  this->Finder.addMatcher(matcherForCall, &(this->ArgStatesHandler));


}

//-----------------------------------------------------------------------------
// FrontendAction
//-----------------------------------------------------------------------------
class ArgStatesAddPluginAction : public PluginASTAction {
public:
  bool ParseArgs(const CompilerInstance &CI,
                 const std::vector<std::string> &args) override {

    DiagnosticsEngine &diagnostics = CI.getDiagnostics();

    unsigned namesDiagID = diagnostics.getCustomDiagID(
      DiagnosticsEngine::Error, "missing -names-file"
    );

    for (size_t i = 0, size = args.size(); i != size; ++i) {

      if (args[i] == "-names-file") {
         if (parseArg(diagnostics, namesDiagID, size, args, i)){
             auto NamesFile = args[++i];
             this->readNamesFromFile(NamesFile);
         } else {
             return false;
         }
      }
      if (!args.empty() && args[0] == "help") {
        llvm::errs() << "No help available";
      }
    }

    return true;
  }

  // Returns our ASTConsumer per translation unit.
  std::unique_ptr<ASTConsumer>
    CreateASTConsumer(CompilerInstance &CI, StringRef file) override {

    RewriterForArgStates.setSourceMgr(CI.getSourceManager(),
              CI.getLangOpts());
    return std::make_unique<ArgStatesASTConsumer>(
  RewriterForArgStates, this->Names);
  }

private:
  void readNamesFromFile(std::string filename) {
    std::ifstream file(filename);

    if (file.is_open()) {
      std::string line;

      while (std::getline(file,line)) {
        this->Names.push_back(line);
      }
      file.close();
    }
  }

  bool parseArg(DiagnosticsEngine &diagnostics, unsigned diagID, int size,
      const std::vector<std::string> &args, int i) {

        if (i + 1 >= size) {
          diagnostics.Report(diagID);
          return false;
        }
  if (args[i+1].empty()) {
    diagnostics.Report(diagID);
    return false;
  }

  return true;
  }

  Rewriter RewriterForArgStates;
  std::vector<std::string> Names;
};

//-----------------------------------------------------------------------------
// Registration
//-----------------------------------------------------------------------------
static FrontendPluginRegistry::Add<ArgStatesAddPluginAction>
    X(/*NamesFile=*/"ArgStates",
      /*Desc=*/"Enumerate the possible states for arguments to calls of the functions given in the -names-file argument.");