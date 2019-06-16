// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "core/common/common.h"
#include "core/framework/data_transfer_manager.h"
#include "core/framework/tensor.h"

namespace onnxruntime {
using namespace common;

int32_t BuildKey(OrtDevice::DeviceType src_device_type,
                 OrtDevice::MemoryType src_memory_type,
                 OrtDevice::DeviceType dst_device_type,
                 OrtDevice::MemoryType dst_memory_type) {
  return (static_cast<int32_t>(src_device_type) << 24) | (static_cast<int32_t>(src_memory_type) << 16) | static_cast<int32_t>(dst_device_type) << 8 | dst_memory_type;
}

const DataTransferManager& DataTransferManager::Instance() {
  static DataTransferManager data_transfer_mgr;
  return data_transfer_mgr;
}

common::Status DataTransferManager::RegisterDataTransfer(
    OrtDevice::DeviceType src_device_type,
    OrtDevice::DeviceType dst_device_type,
    const DataTransfer& data_transfer) {
  return RegisterDataTransfer(src_device_type, OrtDevice::Default, dst_device_type, OrtDevice::Default, data_transfer);
}

common::Status DataTransferManager::RegisterDataTransfer(
    OrtDevice::DeviceType src_device_type,
    OrtDevice::MemoryType src_memory_type,
    OrtDevice::DeviceType dst_device_type,
    OrtDevice::MemoryType dst_memory_type,
    const DataTransfer& data_transfer) {
  int32_t id_key = BuildKey(src_device_type, src_memory_type, dst_device_type, dst_memory_type);
  auto iter = devicetypes_datatransfer_map_.find(id_key);
  if (devicetypes_datatransfer_map_.end() != iter) {
    return ORT_MAKE_STATUS(ONNXRUNTIME,
                           FAIL,
                           "Copy tensor function has already been registered for src (",
                           src_device_type,
                           ") to dst (",
                           dst_device_type,
                           ")");
  }
  devicetypes_datatransfer_map_.insert({id_key, data_transfer});

  return Status::OK();
}

common::Status DataTransferManager::CopyTensor(const Tensor& src, Tensor& dst) const {
  return CopyTensor(src, dst, 0);
}

common::Status DataTransferManager::CopyTensor(const Tensor& src, Tensor& dst, int exec_queue_id) const {
  int32_t id_key = BuildKey(src.Location().device.Type(),
                            src.Location().device.MemType(),
                            dst.Location().device.Type(),
                            dst.Location().device.MemType());
  auto iter = devicetypes_datatransfer_map_.find(id_key);
  if (devicetypes_datatransfer_map_.end() == iter) {
    return ORT_MAKE_STATUS(ONNXRUNTIME,
                           FAIL,
                           "Copy tensor failed due to no copy function found for src (",
                           src.Location().device.Type(),
                           ") to dst (",
                           src.Location().device.Type(),
                           ")");
  }

  if (src.Shape().Size() != dst.Shape().Size()) {
    return Status(ONNXRUNTIME, FAIL, "Tensor size mismatch");
  }

  size_t bytes = src.Size();
  const void* src_data = src.DataRaw();
  void* dst_data = dst.MutableDataRaw();

  return iter->second(src_data, dst_data, bytes, exec_queue_id);
}

}  // namespace onnxruntime