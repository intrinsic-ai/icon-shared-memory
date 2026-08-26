// Copyright 2026 Intrinsic Innovation LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef PLATFORM_COMMON_BUFFERS_RT_QUEUE_BUFFER_H_
#define PLATFORM_COMMON_BUFFERS_RT_QUEUE_BUFFER_H_

#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>

#include "icon/utils/attributes.h"
#include "icon/utils/check.h"

namespace intrinsic {
namespace internal {

// A buffer for performing SPSC-queue style automatic operations.
//
// This buffer can run full, and will reject further insertions at that point.
template <typename T>
class RtQueueBuffer {
 public:
  explicit RtQueueBuffer(size_t capacity);

  RtQueueBuffer(size_t capacity, const std::function<void(T*)>& init_function);

  // Gets a pointer to the front element, or nullptr if empty. After a call to
  // `Front()`, must call `DropFront()` or `KeepFront()` prior to subsequent
  // calls to `Front()`.
  INTR_MUST_USE_RESULT T* Front();

  // Removes the front element; no-op if the queue is empty.
  void DropFront();

  // Keeps the front element.
  void KeepFront();

  // Gets a pointer to the next available element, or nullptr if the queue is
  // full. The element should be set and then `FinishInsert()` must be called.
  INTR_MUST_USE_RESULT T* PrepareInsert();

  // Make the element referenced by the return value of PrepareInsert
  // available to the reader.
  void FinishInsert();

  // Returns the number of elements in the buffer. Thread-safe.
  size_t Size() const { return size_.load(std::memory_order_acquire); }

  // Returns true when the buffer is empty. Thread-safe.
  bool Empty() const { return size_.load(std::memory_order_acquire) == 0; }

  // Returns true when the buffer is full. Thread-safe.
  bool Full() const {
    return size_.load(std::memory_order_acquire) == capacity_;
  }

  // Returns the capacity of the buffer.
  size_t Capacity() const { return capacity_; }

  void InitElements(const std::function<void(T*)>& init_function);

 private:
  // Increases the number of messages stored in the buffer by 1.
  void IncreaseSize() { size_.fetch_add(1, std::memory_order_seq_cst); }

  // Decreases the number of messages stored in the buffer by 1.
  void DecreaseSize() { size_.fetch_sub(1, std::memory_order_seq_cst); }

  // Used by the write methods (`PrepareInsert()`, `FinishInsert()`).
  bool insert_in_progress_ = false;
  size_t head_ = 0;

  // Only used by the read methods (`Front()`, `KeepFront()`, `DropFront()`).
  bool front_accessed_ = false;
  size_t tail_ = 0;

  // Memory used as a ring buffer.
  std::atomic_size_t size_ = 0;  // number of messages stored in the buffer
  const size_t capacity_;        // the length of the buffer
  std::unique_ptr<T[]> buffer_;
};

// Implementation of RealtimeQueue functions.
template <typename T>
RtQueueBuffer<T>::RtQueueBuffer(size_t capacity)
    : capacity_(capacity), buffer_(std::make_unique<T[]>(capacity_)) {}

template <typename T>
RtQueueBuffer<T>::RtQueueBuffer(size_t capacity,
                                const std::function<void(T*)>& init_function)
    : capacity_(capacity), buffer_(std::make_unique<T[]>(capacity_)) {
  InitElements(init_function);
}

template <typename T>
void RtQueueBuffer<T>::InitElements(
    const std::function<void(T*)>& init_function) {
  INTR_CHECK(init_function,
             "Trying to initialize RtQueueBuffer elements with an empty "
             "`init_function`!");
  for (size_t i = 0; i < Capacity(); ++i) {
    init_function(&buffer_[i]);
  }
}

// Implementation of RealtimeQueue::Reader functions.
template <typename T>
T* RtQueueBuffer<T>::Front() {
  INTR_CHECK(!front_accessed_,
             "KeepFront or DropFront must be called before another call to "
             "Front is allowed.");
  if (Empty()) {
    return nullptr;
  }
  front_accessed_ = true;
  return &buffer_[tail_];
}

template <typename T>
void RtQueueBuffer<T>::KeepFront() {
  INTR_CHECK(front_accessed_, "Front must be called before KeepFront.");
  front_accessed_ = false;
}

template <typename T>
void RtQueueBuffer<T>::DropFront() {
  INTR_CHECK(front_accessed_, "Front must be called before DropFront.");
  front_accessed_ = false;
  DecreaseSize();
  tail_ = (tail_ + 1) % Capacity();
}

template <typename T>
T* RtQueueBuffer<T>::PrepareInsert() {
  INTR_CHECK(!insert_in_progress_,
             "FinishInsert must be called before another call to PrepareInsert "
             "is allowed.");
  if (Full()) {
    return nullptr;
  }
  insert_in_progress_ = true;
  return &buffer_[head_];
}

template <typename T>
void RtQueueBuffer<T>::FinishInsert() {
  INTR_CHECK(insert_in_progress_,
             "PrepareInsert must be called before FinishInsert.");
  insert_in_progress_ = false;
  head_ = (head_ + 1) % Capacity();
  IncreaseSize();
}

}  // namespace internal
}  // namespace intrinsic

#endif  // PLATFORM_COMMON_BUFFERS_RT_QUEUE_BUFFER_H_
