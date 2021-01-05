/* Copyright 2019 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "tensorflow/lite/delegates/gpu/common/tasks/softmax1x1.h"

#include <cmath>
#include <cstdlib>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "tensorflow/lite/delegates/gpu/cl/kernels/cl_test.h"
#include "tensorflow/lite/delegates/gpu/common/operations.h"
#include "tensorflow/lite/delegates/gpu/common/status.h"

using ::testing::FloatNear;
using ::testing::Pointwise;

namespace tflite {
namespace gpu {
namespace cl {
namespace {

TEST_F(OpenCLOperationTest, Softmax1x1) {
  TensorFloat32 src_tensor;
  src_tensor.shape = BHWC(1, 1, 1, 4);
  src_tensor.data = {std::log(1.0f), std::log(2.0f), std::log(3.0f),
                     std::log(4.0f)};

  for (auto storage : env_.GetSupportedStorages()) {
    for (auto precision : env_.GetSupportedPrecisions()) {
      const float eps = precision == CalculationsPrecision::F32 ? 1e-6f : 1e-3f;
      OperationDef op_def;
      op_def.precision = precision;
      auto data_type = DeduceDataTypeFromPrecision(precision);
      op_def.src_tensors.push_back({data_type, storage, Layout::HWC});
      op_def.dst_tensors.push_back({data_type, storage, Layout::HWC});
      TensorFloat32 dst_tensor;
      Softmax1x1 operation = CreateSoftmax1x1(op_def);
      ASSERT_OK(ExecuteGPUOperation(
          src_tensor, creation_context_,
          absl::make_unique<Softmax1x1>(std::move(operation)), BHWC(1, 1, 1, 4),
          &dst_tensor));
      EXPECT_THAT(dst_tensor.data,
                  Pointwise(FloatNear(eps), {0.1f, 0.2f, 0.3f, 0.4f}));
    }
  }
}

TEST_F(OpenCLOperationTest, Softmax1x1BigNumber) {
  TensorFloat32 src_tensor;
  src_tensor.shape = BHWC(1, 1, 1, 4);
  double doubles[4] = {1.0, 2.0, 3.0, 100.0};
  // exp(100) is inf in float (32 bit) but representable in double (64 bit)
  src_tensor.data.resize(4);
  src_tensor.data[0] = doubles[0];
  src_tensor.data[1] = doubles[1];
  src_tensor.data[2] = doubles[2];
  src_tensor.data[3] = doubles[3];
  EXPECT_TRUE(std::isinf(std::exp(src_tensor.data[3])));
  EXPECT_FALSE(std::isinf(std::exp(doubles[3])));
  double s0 = std::exp(doubles[0]) + std::exp(doubles[1]) +
              std::exp(doubles[2]) + std::exp(doubles[3]);

  for (auto storage : env_.GetSupportedStorages()) {
    for (auto precision : env_.GetSupportedPrecisions()) {
      const float eps = precision == CalculationsPrecision::F32 ? 1e-6f : 1e-3f;
      OperationDef op_def;
      op_def.precision = precision;
      auto data_type = DeduceDataTypeFromPrecision(precision);
      op_def.src_tensors.push_back({data_type, storage, Layout::HWC});
      op_def.dst_tensors.push_back({data_type, storage, Layout::HWC});
      TensorFloat32 dst_tensor;
      Softmax1x1 operation = CreateSoftmax1x1(op_def);
      ASSERT_OK(ExecuteGPUOperation(
          src_tensor, creation_context_,
          absl::make_unique<Softmax1x1>(std::move(operation)), BHWC(1, 1, 1, 4),
          &dst_tensor));
      EXPECT_THAT(
          dst_tensor.data,
          Pointwise(FloatNear(eps),
                    {std::exp(doubles[0]) / s0, std::exp(doubles[1]) / s0,
                     std::exp(doubles[2]) / s0, std::exp(doubles[3]) / s0}));
    }
  }
}

}  // namespace
}  // namespace cl
}  // namespace gpu
}  // namespace tflite
