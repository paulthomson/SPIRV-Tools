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

#include "source/reduce/remove_module_instruction_reduction_opportunity_finder.h"

#include "source/opt/build_module.h"
#include "source/reduce/reduction_opportunity.h"
#include "source/util/make_unique.h"
#include "test/reduce/reduce_test_util.h"

namespace spvtools {
namespace reduce {
namespace {

const spv_target_env kEnv = SPV_ENV_UNIVERSAL_1_3;

TEST(RemoveModuleInstructionReductionPassTest, Referenced) {
  // A module with some unused global variables, constants, and types. Some will
  // not be removed because of the OpDecorate instructions. Others are used
  // initially but can be removed in later steps.

  const std::string shader = R"(
               OpCapability Shader
          %1 = OpExtInstImport "GLSL.std.450"
               OpMemoryModel Logical GLSL450
               OpEntryPoint Fragment %4 "main"
               OpExecutionMode %4 OriginUpperLeft
               OpSource ESSL 310
               OpName %4 "main"
               OpName %12 "a"
               OpDecorate %12 RelaxedPrecision
               OpDecorate %13 RelaxedPrecision
          %2 = OpTypeVoid
          %3 = OpTypeFunction %2
          %6 = OpTypeBool
          %7 = OpConstantTrue %6           ; unused: will be removed
         %10 = OpTypeInt 32 1
         %11 = OpTypePointer Private %10
         %12 = OpVariable %11 Private
         %13 = OpConstant %10 1
          %4 = OpFunction %2 None %3
          %5 = OpLabel
               OpReturn
               OpFunctionEnd
  )";

  auto context = BuildModule(kEnv, nullptr, shader, kReduceAssembleOption);

  CheckValid(kEnv, context.get());

  auto ops = RemoveModuleInstructionReductionOpportunityFinder()
                 .GetAvailableOpportunities(context.get());

  ASSERT_EQ(1, ops.size());

  ASSERT_TRUE(ops[0]->PreconditionHolds());
  ops[0]->TryToApply();
  CheckValid(kEnv, context.get());

  std::string after = R"(
               OpCapability Shader
          %1 = OpExtInstImport "GLSL.std.450"
               OpMemoryModel Logical GLSL450
               OpEntryPoint Fragment %4 "main"
               OpExecutionMode %4 OriginUpperLeft
               OpSource ESSL 310
               OpName %4 "main"
               OpName %12 "a"
               OpDecorate %12 RelaxedPrecision
               OpDecorate %13 RelaxedPrecision
          %2 = OpTypeVoid
          %3 = OpTypeFunction %2
          %6 = OpTypeBool              ; unused: will be removed
         %10 = OpTypeInt 32 1
         %11 = OpTypePointer Private %10
         %12 = OpVariable %11 Private
         %13 = OpConstant %10 1
          %4 = OpFunction %2 None %3
          %5 = OpLabel
               OpReturn
               OpFunctionEnd
    )";

  CheckEqual(kEnv, after, context.get());

  ops = RemoveModuleInstructionReductionOpportunityFinder()
            .GetAvailableOpportunities(context.get());

  ASSERT_EQ(1, ops.size());

  ASSERT_TRUE(ops[0]->PreconditionHolds());
  ops[0]->TryToApply();
  CheckValid(kEnv, context.get());

  // Nothing else can be removed.
  std::string after_2 = R"(
               OpCapability Shader
          %1 = OpExtInstImport "GLSL.std.450"
               OpMemoryModel Logical GLSL450
               OpEntryPoint Fragment %4 "main"
               OpExecutionMode %4 OriginUpperLeft
               OpSource ESSL 310
               OpName %4 "main"
               OpName %12 "a"
               OpDecorate %12 RelaxedPrecision
               OpDecorate %13 RelaxedPrecision
          %2 = OpTypeVoid
          %3 = OpTypeFunction %2
         %10 = OpTypeInt 32 1
         %11 = OpTypePointer Private %10
         %12 = OpVariable %11 Private
         %13 = OpConstant %10 1
          %4 = OpFunction %2 None %3
          %5 = OpLabel
               OpReturn
               OpFunctionEnd
    )";

  CheckEqual(kEnv, after_2, context.get());

  ops = RemoveModuleInstructionReductionOpportunityFinder()
            .GetAvailableOpportunities(context.get());

  ASSERT_EQ(0, ops.size());
}

TEST(RemoveModuleInstructionReductionPassTest, Unreferenced) {
  // A module with some unused global variables, constants, and types. It is the
  // same as the module in the above test, but with OpName and OpDecorate
  // instructions removed.

  RemoveModuleInstructionReductionOpportunityFinder finder;

  const std::string shader = R"(
               OpCapability Shader
          %1 = OpExtInstImport "GLSL.std.450"
               OpMemoryModel Logical GLSL450
               OpEntryPoint Fragment %4 "main"
               OpExecutionMode %4 OriginUpperLeft
               OpSource ESSL 310
               OpName %4 "main"
          %2 = OpTypeVoid
          %3 = OpTypeFunction %2
          %6 = OpTypeBool
          %7 = OpConstantTrue %6           ; unused: will be removed
         %10 = OpTypeInt 32 1
         %11 = OpTypePointer Private %10
         %12 = OpVariable %11 Private      ; unused: will be removed
         %13 = OpConstant %10 1            ; unused: will be removed
          %4 = OpFunction %2 None %3
          %5 = OpLabel
               OpReturn
               OpFunctionEnd
  )";

  auto context = BuildModule(kEnv, nullptr, shader, kReduceAssembleOption);

  CheckValid(kEnv, context.get());

  auto ops = finder.GetAvailableOpportunities(context.get());

  ASSERT_EQ(3, ops.size());

  for (auto& op : ops) {
    ASSERT_TRUE(op->PreconditionHolds());
    op->TryToApply();
    CheckValid(kEnv, context.get());
  }

  std::string after = R"(
               OpCapability Shader
          %1 = OpExtInstImport "GLSL.std.450"
               OpMemoryModel Logical GLSL450
               OpEntryPoint Fragment %4 "main"
               OpExecutionMode %4 OriginUpperLeft
               OpSource ESSL 310
               OpName %4 "main"
          %2 = OpTypeVoid
          %3 = OpTypeFunction %2
          %6 = OpTypeBool                ; unused: will be removed
         %10 = OpTypeInt 32 1
         %11 = OpTypePointer Private %10 ; unused: will be removed
          %4 = OpFunction %2 None %3
          %5 = OpLabel
               OpReturn
               OpFunctionEnd
  )";

  CheckEqual(kEnv, after, context.get());

  ops = finder.GetAvailableOpportunities(context.get());

  ASSERT_EQ(2, ops.size());

  for (auto& op : ops) {
    ASSERT_TRUE(op->PreconditionHolds());
    op->TryToApply();
    CheckValid(kEnv, context.get());
  }

  std::string after_2 = R"(
               OpCapability Shader
          %1 = OpExtInstImport "GLSL.std.450"
               OpMemoryModel Logical GLSL450
               OpEntryPoint Fragment %4 "main"
               OpExecutionMode %4 OriginUpperLeft
               OpSource ESSL 310
               OpName %4 "main"
          %2 = OpTypeVoid
          %3 = OpTypeFunction %2
         %10 = OpTypeInt 32 1        ; unused: will be removed
          %4 = OpFunction %2 None %3
          %5 = OpLabel
               OpReturn
               OpFunctionEnd
  )";

  CheckEqual(kEnv, after_2, context.get());

  ops = finder.GetAvailableOpportunities(context.get());

  ASSERT_EQ(1, ops.size());

  for (auto& op : ops) {
    ASSERT_TRUE(op->PreconditionHolds());
    op->TryToApply();
    CheckValid(kEnv, context.get());
  }

  // Nothing else can be removed.
  std::string after_3 = R"(
               OpCapability Shader
          %1 = OpExtInstImport "GLSL.std.450"
               OpMemoryModel Logical GLSL450
               OpEntryPoint Fragment %4 "main"
               OpExecutionMode %4 OriginUpperLeft
               OpSource ESSL 310
               OpName %4 "main"
          %2 = OpTypeVoid
          %3 = OpTypeFunction %2
          %4 = OpFunction %2 None %3
          %5 = OpLabel
               OpReturn
               OpFunctionEnd
  )";

  CheckEqual(kEnv, after_3, context.get());

  ops = finder.GetAvailableOpportunities(context.get());

  ASSERT_EQ(0, ops.size());
}

}  // namespace
}  // namespace reduce
}  // namespace spvtools
