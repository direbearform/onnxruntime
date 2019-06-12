// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "lstm.h"
#include "core/providers/cpu/rnn/rnn_helpers.h"
#include "core/providers/common.h"

namespace onnxruntime {
namespace cuda {

#define REGISTER_KERNEL_TYPED(T)                                                \
  ONNX_OPERATOR_TYPED_KERNEL_EX(                                                \
      LSTM,                                                                     \
      kOnnxDomain,                                                              \
      7,                                                                        \
      T,                                                                        \
      kCudaExecutionProvider,                                                   \
      KernelDefBuilder()                                                        \
          .TypeConstraint("T", DataTypeImpl::GetTensorType<T>())                \
          .TypeConstraint("T1", DataTypeImpl::GetTensorType<int32_t>())         \
          .InputMemoryType<OrtMemTypeCPUInput>(RNN_Input_Index::sequence_lens), \
      LSTM<T>);

REGISTER_KERNEL_TYPED(float);
REGISTER_KERNEL_TYPED(double);
REGISTER_KERNEL_TYPED(MLFloat16);

}  // namespace cuda
}  // namespace onnxruntime
