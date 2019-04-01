// Copyright (c) 2019 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "source/reduce/conditional_branch_to_simple_conditional_branch_opportunity_finder.h"

#include "source/reduce/conditional_branch_to_simple_conditional_branch_reduction_opportunity.h"
#include "source/reduce/reduction_util.h"

namespace spvtools {
namespace reduce {

using namespace opt;

std::vector<std::unique_ptr<ReductionOpportunity>>
ConditionalBranchToSimpleConditionalBranchOpportunityFinder::
    GetAvailableOpportunities(IRContext* context) const {
  std::vector<std::unique_ptr<ReductionOpportunity>> result;

  // Find the opportunities for redirecting all false targets before the
  // opportunities for redirecting all true targets because the former
  // opportunities disable the latter, and vice versa, and the efficiency of the
  // reducer is improved by avoiding contiguous opportunities that disable one
  // another.
  for (bool redirect_to_true : {true, false}) {
    // Consider every function.
    for (auto& function : *context->module()) {
      // Consider every block in the function.
      for (auto& block : function) {
        // The terminator must be SpvOpBranchConditional.
        Instruction* terminator = block.terminator();
        if (terminator->opcode() != SpvOpBranchConditional) {
          continue;
        }
        // The conditional branch must not already be simplified.
        if (terminator->GetSingleWordInOperand(kTrueBranchOperandIndex) ==
            terminator->GetSingleWordInOperand(kFalseBranchOperandIndex)) {
          continue;
        }
        result.push_back(
            MakeUnique<
                ConditionalBranchToSimpleConditionalBranchReductionOpportunity>(
                block.terminator(), redirect_to_true));
      }
    }
  }
  return result;
}

std::string
ConditionalBranchToSimpleConditionalBranchOpportunityFinder::GetName() const {
  return "ConditionalBranchToSimpleConditionalBranchOpportunityFinder";
}

}  // namespace reduce
}  // namespace spvtools
