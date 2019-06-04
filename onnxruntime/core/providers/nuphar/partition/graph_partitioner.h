// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once
#include "core/codegen/common/common.h"
#include "core/framework/compute_capability.h"
#include "core/providers/nuphar/partition/partitioner.h"

#include <functional>
#include <unordered_set>
#include <vector>

namespace onnxruntime {
namespace nuphar {

using IsOpTypeSupportedFunc = std::function<bool(const Node& node)>;

// GraphPartitioner partitions Ort graph and generates FuseNodes.
class GraphPartitioner : public Partitioner {
 public:
  GraphPartitioner(IsOpTypeSupportedFunc is_op_type_supported_func)
      : is_op_type_supported_func_(is_op_type_supported_func), Partitioner() {}

  Status Partition(const onnxruntime::GraphViewer& graph,
                   std::vector<std::unique_ptr<ComputeCapability>>& result);

 private:
  IsOpTypeSupportedFunc is_op_type_supported_func_;

  bool IsNodeSupported(const Node& node) override;

  std::unordered_set<NodeKey> unsupported_nodes_;

  // FORCE_ONE_SUBGRAPH is a marco to generate single subgraph partition
  // It is mainly for debug and reproducing older version
#ifdef FORCE_ONE_SUBGRAPH
  bool ForcePartition(const NodeIndex& n_idx, const int topology_idx,
                      const Node& node, const std::vector<NodeIndex>& candiates,
                      const std::vector<NodeIndex>& rejected_partitions) override;
#endif
};

}  // namespace nuphar
}  // namespace onnxruntime