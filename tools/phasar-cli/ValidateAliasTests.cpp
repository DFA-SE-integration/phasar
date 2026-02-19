/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include "ValidateAliasTests.h"
#include "phasar/PhasarLLVM/DB/LLVMProjectIRDB.h"
#include "phasar/PhasarLLVM/HelperAnalyses.h"
#include "phasar/PhasarLLVM/Pointer/LLVMAliasSet.h"
#include "phasar/PhasarLLVM/Utils/LLVMIRToSrc.h"
#include "phasar/Pointer/AliasResult.h"

#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"

#include <optional>
#include <string>
#include <unordered_set>

using namespace psr;

namespace {

const std::unordered_set<std::string> &getAliasCheckNames() {
  static const std::unordered_set<std::string> Names = {
      "MAYALIAS", "NOALIAS", "MUSTALIAS", "PARTIALALIAS",
      "_Z8MAYALIASPvS_", "_Z8MAYALIASPvS0_",
      "_Z7NOALIASPvS_",  "_Z7NOALIASPvS0_",
      "_Z9MUSTALIASPvS_", "_Z9MUSTALIASPvS0_",
      "_Z12PARTIALALIASPvS_", "_Z12PARTIALALIASPvS0_",
  };
  return Names;
}

std::string formatSourceLoc(const llvm::Instruction *I) {
  std::string file = getFilePathFromIR(I);
  unsigned line = getLineFromIR(I);
  unsigned col = getColumnFromIR(I);
  if (file.empty() && line == 0)
    return "(unknown)";
  return file + ":" + std::to_string(line) + ":" + std::to_string(col);
}

bool isAliasCheckCall(const llvm::Function *F) {
  if (!F)
    return false;
  return getAliasCheckNames().count(F->getName().str()) != 0;
}

enum class CheckKind { MAYALIAS, NOALIAS, MUSTALIAS, PARTIALALIAS };

std::optional<CheckKind> getCheckKind(llvm::StringRef Name) {
  if (Name == "MAYALIAS" || Name.startswith("_Z8MAYALIAS"))
    return CheckKind::MAYALIAS;
  if (Name == "NOALIAS" || Name.startswith("_Z7NOALIAS"))
    return CheckKind::NOALIAS;
  if (Name == "MUSTALIAS" || Name.startswith("_Z9MUSTALIAS"))
    return CheckKind::MUSTALIAS;
  if (Name == "PARTIALALIAS" || Name.startswith("_Z12PARTIALALIAS"))
    return CheckKind::PARTIALALIAS;
  return std::nullopt;
}

bool checkSucceeded(CheckKind Kind, AliasResult Result) {
  switch (Kind) {
  case CheckKind::MAYALIAS:
  case CheckKind::MUSTALIAS:
    return Result == AliasResult::MayAlias || Result == AliasResult::MustAlias;
  case CheckKind::NOALIAS:
    return Result == AliasResult::NoAlias;
  case CheckKind::PARTIALALIAS:
    return Result == AliasResult::MayAlias || Result == AliasResult::PartialAlias;
  }
  return false;
}

const char *checkKindStr(CheckKind Kind) {
  switch (Kind) {
  case CheckKind::MAYALIAS:    return "MAYALIAS";
  case CheckKind::NOALIAS:     return "NOALIAS";
  case CheckKind::MUSTALIAS:   return "MUSTALIAS";
  case CheckKind::PARTIALALIAS: return "PARTIALALIAS";
  }
  return "?";
}

} // namespace

bool psr::runValidateAliasTests(HelperAnalyses &HA) {
  llvm::Module *M = HA.getProjectIRDB().getModule();
  if (!M)
    return true;

  auto &AliasInfo = HA.getAliasInfo();
  bool anyFailure = false;

  for (llvm::Function &F : *M) {
    for (llvm::BasicBlock &BB : F) {
      for (llvm::Instruction &I : BB) {
        auto *CI = llvm::dyn_cast<llvm::CallBase>(&I);
        if (!CI)
          continue;
        const llvm::Function *Callee = CI->getCalledFunction();
        if (!Callee || !isAliasCheckCall(Callee))
          continue;
        if (CI->arg_size() < 2)
          continue;

        std::optional<CheckKind> Kind = getCheckKind(Callee->getName());
        if (!Kind)
          continue;

        const llvm::Value *V1 = CI->getArgOperand(0);
        const llvm::Value *V2 = CI->getArgOperand(1);
        AliasResult Result = AliasInfo.alias(V1, V2, &I);
        bool success = checkSucceeded(*Kind, Result);
        std::string Loc = formatSourceLoc(&I);

        if (success) {
          llvm::outs() << "\t SUCCESS :" << checkKindStr(*Kind)
                       << " check at (" << Loc << ")\n";
        } else {
          llvm::errs() << "\t FAILURE :" << checkKindStr(*Kind)
                       << " check at (" << Loc << ")\n";
          anyFailure = true;
        }
      }
    }
  }

  return !anyFailure;
}
