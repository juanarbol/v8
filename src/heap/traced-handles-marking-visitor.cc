// Copyright 2022 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/heap/traced-handles-marking-visitor.h"

#include "src/heap/marking-state-inl.h"
#include "src/heap/marking-worklist-inl.h"

namespace v8 {
namespace internal {

ConservativeTracedHandlesMarkingVisitor::
    ConservativeTracedHandlesMarkingVisitor(
        Heap& heap, MarkingWorklists::Local& local_marking_worklist,
        cppgc::internal::CollectionType collection_type)
    : heap_(heap),
      has_shared_space_(heap.isolate()->has_shared_space()),
      is_shared_space_isolate_(heap.isolate()->is_shared_space_isolate()),
      marking_state_(*heap_.marking_state()),
      local_marking_worklist_(local_marking_worklist),
      traced_node_bounds_(heap.isolate()->traced_handles()->GetNodeBounds()),
      mark_mode_(collection_type == cppgc::internal::CollectionType::kMinor
                     ? TracedHandles::MarkMode::kOnlyYoung
                     : TracedHandles::MarkMode::kAll) {}

void ConservativeTracedHandlesMarkingVisitor::VisitPointer(
    const void* address) {
  const auto upper_it = std::upper_bound(
      traced_node_bounds_.begin(), traced_node_bounds_.end(), address,
      [](const void* needle, const auto& pair) { return needle < pair.first; });
  // Also checks emptiness as begin() == end() on empty bounds.
  if (upper_it == traced_node_bounds_.begin()) return;

  const auto bounds = std::next(upper_it, -1);
  if (address < bounds->second) {
    auto object = TracedHandles::MarkConservatively(
        const_cast<Address*>(reinterpret_cast<const Address*>(address)),
        const_cast<Address*>(reinterpret_cast<const Address*>(bounds->first)),
        mark_mode_);
    if (!object.IsHeapObject()) {
      // The embedder is not aware of whether numbers are materialized as heap
      // objects are just passed around as Smis. This branch also filters out
      // intentionally passed `Smi::zero()` that indicate that there's no
      // object to mark.
      return;
    }
    HeapObject heap_object = HeapObject::cast(object);
    if (heap_object.InReadOnlySpace()) return;
    if (marking_state_.TryMark(heap_object)) {
      local_marking_worklist_.Push(heap_object);
    }
    if (V8_UNLIKELY(v8_flags.track_retaining_path)) {
      heap_.AddRetainingRoot(Root::kTracedHandles, heap_object);
    }
  }
}

bool ConservativeTracedHandlesMarkingVisitor::ShouldMarkObject(
    HeapObject object) const {
  // Keep up-to-date with MarkCompactCollector::ShouldMarkObject.
  if (V8_LIKELY(!has_shared_space_)) return true;
  if (is_shared_space_isolate_) return true;
  return !object.InAnySharedSpace();
}

}  // namespace internal
}  // namespace v8
