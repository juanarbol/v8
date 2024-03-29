// Copyright 2017 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_OBJECTS_JS_ARRAY_INL_H_
#define V8_OBJECTS_JS_ARRAY_INL_H_

#include "src/objects/js-array.h"

#include "src/objects/objects-inl.h"  // Needed for write barriers

// Has to be the last include (doesn't have include guards):
#include "src/objects/object-macros.h"

namespace v8 {
namespace internal {

#include "torque-generated/src/objects/js-array-tq-inl.inc"

TQ_OBJECT_CONSTRUCTORS_IMPL(JSArray)
TQ_OBJECT_CONSTRUCTORS_IMPL(JSArrayIterator)
TQ_OBJECT_CONSTRUCTORS_IMPL(TemplateLiteralObject)

DEF_GETTER(JSArray, length, Object) {
  return TaggedField<Object, kLengthOffset>::load(cage_base, *this);
}

void JSArray::set_length(Object value, WriteBarrierMode mode) {
  // Note the relaxed atomic store.
  TaggedField<Object, kLengthOffset>::Relaxed_Store(*this, value);
  CONDITIONAL_WRITE_BARRIER(*this, kLengthOffset, value, mode);
}

Object JSArray::length(PtrComprCageBase cage_base, RelaxedLoadTag tag) const {
  return TaggedField<Object, kLengthOffset>::Relaxed_Load(cage_base, *this);
}

void JSArray::set_length(Smi length) {
  // Don't need a write barrier for a Smi.
  set_length(Object(length.ptr()), SKIP_WRITE_BARRIER);
}

bool JSArray::SetLengthWouldNormalize(Heap* heap, uint32_t new_length) {
  return new_length > kMaxFastArrayLength;
}

void JSArray::SetContent(Handle<JSArray> array,
                         Handle<FixedArrayBase> storage) {
  EnsureCanContainElements(array, storage, storage->length(),
                           ALLOW_COPIED_DOUBLE_ELEMENTS);

  DCHECK(
      (storage->map() == array->GetReadOnlyRoots().fixed_double_array_map() &&
       IsDoubleElementsKind(array->GetElementsKind())) ||
      ((storage->map() != array->GetReadOnlyRoots().fixed_double_array_map()) &&
       (IsObjectElementsKind(array->GetElementsKind()) ||
        (IsSmiElementsKind(array->GetElementsKind()) &&
         Handle<FixedArray>::cast(storage)->ContainsOnlySmisOrHoles()))));
  array->set_elements(*storage);
  array->set_length(Smi::FromInt(storage->length()));
}

bool JSArray::HasArrayPrototype(Isolate* isolate) {
  return map()->prototype() == *isolate->initial_array_prototype();
}

SMI_ACCESSORS(JSArrayIterator, raw_kind, kKindOffset)

IterationKind JSArrayIterator::kind() const {
  return static_cast<IterationKind>(raw_kind());
}

void JSArrayIterator::set_kind(IterationKind kind) {
  set_raw_kind(static_cast<int>(kind));
}

CAST_ACCESSOR(TemplateLiteralObject)

}  // namespace internal
}  // namespace v8

#include "src/objects/object-macros-undef.h"

#endif  // V8_OBJECTS_JS_ARRAY_INL_H_
