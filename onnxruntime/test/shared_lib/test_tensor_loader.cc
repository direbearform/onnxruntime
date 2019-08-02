// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "core/session/onnxruntime_cxx_api.h"
#include "core/common/common.h"
#include "core/framework/callback.h"
#include "core/framework/tensorprotoutils.h"
#include "test_fixture.h"
#include "file_util.h"

#ifdef _WIN32
#include <Windows.h>
#endif

namespace onnxruntime {
namespace test {

TEST_F(CApiTest, load_simple_float_tensor_not_enough_space) {
  // construct a tensor proto
  onnx::TensorProto p;
  p.mutable_float_data()->Add(1.0f);
  p.mutable_float_data()->Add(2.2f);
  p.mutable_dims()->Add(2);
  p.set_data_type(onnx::TensorProto_DataType_FLOAT);
  std::string s;
  // save it to a buffer
  ASSERT_TRUE(p.SerializeToString(&s));
  // deserialize it
  std::vector<float> output(1);
  OrtValue value;
  std::unique_ptr<onnxruntime::OrtCallback> deleter = std::make_unique<onnxruntime::OrtCallback>();
  OrtAllocatorInfo cpu_allocator_info(onnxruntime::CPU, OrtDeviceAllocator, OrtDevice(), 0, OrtMemTypeDefault);
  auto st = utils::TensorProtoToMLValue(Env::Default(), nullptr, p,
                                        MemBuffer(output.data(), output.size() * sizeof(float), cpu_allocator_info), value, *deleter);
  // check the result
  ASSERT_FALSE(st.IsOK());
  OrtRunCallback(deleter.release());
}

TEST_F(CApiTest, load_simple_float_tensor) {
  // construct a tensor proto
  onnx::TensorProto p;
  p.mutable_float_data()->Add(1.0f);
  p.mutable_float_data()->Add(2.2f);
  p.mutable_float_data()->Add(3.5f);
  p.mutable_dims()->Add(3);
  p.set_data_type(onnx::TensorProto_DataType_FLOAT);
  std::string s;
  // save it to a buffer
  ASSERT_TRUE(p.SerializeToString(&s));
  // deserialize it
  std::vector<float> output(3);
  OrtValue value;
  std::unique_ptr<onnxruntime::OrtCallback> deleter = std::make_unique<onnxruntime::OrtCallback>();
  OrtAllocatorInfo cpu_allocator_info(onnxruntime::CPU, OrtDeviceAllocator, OrtDevice(), 0, OrtMemTypeDefault);
  auto st = utils::TensorProtoToMLValue(Env::Default(), nullptr, p,
                                        MemBuffer(output.data(), output.size() * sizeof(float), cpu_allocator_info), value, *deleter);
  ASSERT_TRUE(st.IsOK()) << st.ErrorMessage();
  float* real_output;
  auto ort_st = OrtGetTensorMutableData(&value, (void**)&real_output);
  ASSERT_EQ(ort_st, nullptr) << OrtGetErrorMessage(ort_st);
  // check the result
  ASSERT_EQ(real_output[0], 1.0f);
  ASSERT_EQ(real_output[1], 2.2f);
  ASSERT_EQ(real_output[2], 3.5f);
  OrtReleaseStatus(ort_st);
  OrtRunCallback(deleter.release());
}

template <bool use_current_dir>
static void run_external_data_test() {
  FILE* fp;
  std::basic_string<ORTCHAR_T> filename(ORT_TSTR("tensor_XXXXXX"));
  CreateTestFile(fp, filename);
  std::unique_ptr<ORTCHAR_T, decltype(&DeleteFileFromDisk)> file_deleter(const_cast<ORTCHAR_T*>(filename.c_str()),
                                                                         DeleteFileFromDisk);
  float test_data[] = {1.0f, 2.2f, 3.5f};
  ASSERT_EQ(sizeof(test_data), fwrite(test_data, 1, sizeof(test_data), fp));
  ASSERT_EQ(0, fclose(fp));
  // construct a tensor proto
  onnx::TensorProto p;
  onnx::StringStringEntryProto* location = p.mutable_external_data()->Add();
  location->set_key("location");
  location->set_value(ToMBString(filename));
  p.mutable_dims()->Add(3);
  p.set_data_location(onnx::TensorProto_DataLocation_EXTERNAL);
  p.set_data_type(onnx::TensorProto_DataType_FLOAT);
  std::string s;
  // save it to a buffer
  ASSERT_TRUE(p.SerializeToString(&s));
  // deserialize it
  std::vector<float> output(3);
  std::basic_string<ORTCHAR_T> cwd;
  if (use_current_dir) {
#ifdef _WIN32
    DWORD len = GetCurrentDirectory(0, nullptr);
    ASSERT_NE(len, (DWORD)0);
    cwd.resize(static_cast<size_t>(len) - 1, '\0');
    len = GetCurrentDirectoryW(len, (ORTCHAR_T*)cwd.data());
    ASSERT_NE(len, (DWORD)0);
    cwd.append(ORT_TSTR("\\fake.onnx"));
#else
    char* p = getcwd(nullptr, 0);
    ASSERT_NE(p, nullptr);
    cwd = p;
    free(p);
    cwd.append(ORT_TSTR("/fake.onnx"));
#endif
  }
  OrtValue value;
  std::unique_ptr<onnxruntime::OrtCallback> deleter = std::make_unique<onnxruntime::OrtCallback>();
  OrtAllocatorInfo cpu_allocator_info(onnxruntime::CPU, OrtDeviceAllocator, OrtDevice(), 0, OrtMemTypeDefault);
  auto st = utils::TensorProtoToMLValue(Env::Default(), nullptr, p,
                                        MemBuffer(output.data(), output.size() * sizeof(float), cpu_allocator_info), value, *deleter);
  ASSERT_TRUE(st.IsOK()) << st.ErrorMessage();
  float* real_output;
  auto ort_st = OrtGetTensorMutableData(&value, (void**)&real_output);
  ASSERT_EQ(ort_st, nullptr) << OrtGetErrorMessage(ort_st);
  // check the result
  ASSERT_EQ(real_output[0], 1.0f);
  ASSERT_EQ(real_output[1], 2.2f);
  ASSERT_EQ(real_output[2], 3.5f);
  OrtReleaseStatus(ort_st);
  OrtRunCallback(deleter.release());
}

TEST_F(CApiTest, load_float_tensor_with_external_data) {
  run_external_data_test<true>();
  run_external_data_test<false>();
}

#if defined(__amd64__) || defined(_M_X64)

TEST_F(CApiTest, load_huge_tensor_with_external_data) {
  FILE* fp;
  std::basic_string<ORTCHAR_T> filename(ORT_TSTR("tensor_XXXXXX"));
  CreateTestFile(fp, filename);
  std::unique_ptr<ORTCHAR_T, decltype(&DeleteFileFromDisk)> file_deleter(const_cast<ORTCHAR_T*>(filename.c_str()),
                                                                         DeleteFileFromDisk);
  std::vector<int> data(524288, 1);
  const size_t len = data.size() * sizeof(int);
  for (int i = 0; i != 1025; ++i) {
    ASSERT_EQ(len, fwrite(data.data(), 1, len, fp));
  }
  const size_t total_ele_count = 524288 * 1025;
  ASSERT_EQ(0, fclose(fp));
  // construct a tensor proto
  onnx::TensorProto p;
  onnx::StringStringEntryProto* location = p.mutable_external_data()->Add();
  location->set_key("location");
  location->set_value(ToMBString(filename));
  p.mutable_dims()->Add(524288);
  p.mutable_dims()->Add(1025);
  p.set_data_location(onnx::TensorProto_DataLocation_EXTERNAL);
  p.set_data_type(onnx::TensorProto_DataType_INT32);
  std::string s;
  // save it to a buffer
  ASSERT_TRUE(p.SerializeToString(&s));
  // deserialize it
  std::vector<int> output(total_ele_count);
  OrtValue value;
  std::unique_ptr<onnxruntime::OrtCallback> deleter = std::make_unique<onnxruntime::OrtCallback>();
  OrtAllocatorInfo cpu_allocator_info(onnxruntime::CPU, OrtDeviceAllocator, OrtDevice(), 0, OrtMemTypeDefault);
  auto st = utils::TensorProtoToMLValue(Env::Default(), nullptr, p,
                                        MemBuffer(output.data(), output.size() * sizeof(int), cpu_allocator_info), value, *deleter);

  // check the result
  ASSERT_TRUE(st.IsOK()) << st.ErrorMessage();
  int* buffer;
  auto ort_st = OrtGetTensorMutableData(&value, (void**)&buffer);
  ASSERT_EQ(ort_st, nullptr) << OrtGetErrorMessage(ort_st);
  for (size_t i = 0; i != total_ele_count; ++i) {
    ASSERT_EQ(1, buffer[i]);
  }
  OrtReleaseStatus(ort_st);
  OrtRunCallback(deleter.release());
}
#endif
}  // namespace test
}  // namespace onnxruntime
