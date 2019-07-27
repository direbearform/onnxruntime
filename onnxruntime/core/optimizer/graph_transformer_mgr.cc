// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "core/optimizer/graph_transformer_mgr.h"
#include "core/optimizer/rule_based_graph_transformer.h"
#include "core/common/logging/logging.h"
using namespace onnxruntime;
using namespace ::onnxruntime::common;

namespace onnxruntime {

common::Status GraphTransformerManager::ApplyTransformers(Graph& graph, TransformerLevel level) const {
  const auto& transformers = level_to_transformer_map_.find(level);
  if (transformers == level_to_transformer_map_.end()) {
    return Status::OK();
  }

  bool graph_changed = false;
  for (unsigned step = 0; step < steps_; ++step) {
    graph_changed = false;
    for (const auto& transformer : transformers->second) {
      bool modified = false;
      ORT_RETURN_IF_ERROR(transformer->Apply(graph, modified));
      graph_changed = graph_changed || modified;
    }

    if (!graph_changed) {
      break;
    }
  }

  if (graph_changed) {
    LOGS_DEFAULT(WARNING) << "Graph was still being optimized by transformers but ran out of steps.";
  }

  return Status::OK();
}

common::Status GraphTransformerManager::Register(std::unique_ptr<GraphTransformer> transformer, TransformerLevel level) {
  const auto& name = transformer->Name();
  if (transformers_info_.find(name) != transformers_info_.end()) {
    return Status(ONNXRUNTIME, FAIL, "This transformer is already registered " + name);
  }

  transformers_info_[name] = transformer.get();
  level_to_transformer_map_[level].push_back(std::move(transformer));
  return Status::OK();
}
}  // namespace onnxruntime
