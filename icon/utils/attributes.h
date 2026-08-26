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

#ifndef ICON_UTILS_ATTRIBUTES_H_
#define ICON_UTILS_ATTRIBUTES_H_

#ifdef __has_attribute
#define INTR_HAS_ATTRIBUTE(x) __has_attribute(x)
#else
#define INTR_HAS_ATTRIBUTE(x) 0
#endif

#ifdef __has_cpp_attribute
#define INTR_HAS_CPP_ATTRIBUTE(x) __has_cpp_attribute(x)
#else
#define INTR_HAS_CPP_ATTRIBUTE(x) 0
#endif

#if INTR_HAS_CPP_ATTRIBUTE(nodiscard)
#define INTR_MUST_USE_RESULT [[nodiscard]]
#elif INTR_HAS_ATTRIBUTE(warn_unused_result)
#define INTR_MUST_USE_RESULT __attribute__((warn_unused_result))
#else
#define INTR_MUST_USE_RESULT
#endif

#if INTR_HAS_CPP_ATTRIBUTE(clang::lifetimebound)
#define INTR_ATTRIBUTE_LIFETIME_BOUND [[clang::lifetimebound]]
#elif INTR_HAS_CPP_ATTRIBUTE(msvc::lifetimebound)
#define INTR_ATTRIBUTE_LIFETIME_BOUND [[msvc::lifetimebound]]
#elif INTR_HAS_ATTRIBUTE(lifetimebound)
#define INTR_ATTRIBUTE_LIFETIME_BOUND __attribute__((lifetimebound))
#else
#define INTR_ATTRIBUTE_LIFETIME_BOUND
#endif

#if INTR_HAS_CPP_ATTRIBUTE(clang::lifetime_capture_by)
#define INTR_ATTRIBUTE_LIFETIME_CAPTURE_BY(x) [[clang::lifetime_capture_by(x)]]
#else
#define INTR_ATTRIBUTE_LIFETIME_CAPTURE_BY(x)
#endif

#if INTR_HAS_ATTRIBUTE(guarded_by)
#define INTR_GUARDED_BY(x) __attribute__((guarded_by(x)))
#else
#define INTR_GUARDED_BY(x)
#endif

#if INTR_HAS_ATTRIBUTE(lockable)
#define INTR_LOCKABLE __attribute__((lockable))
#else
#define INTR_LOCKABLE
#endif

#if INTR_HAS_ATTRIBUTE(scoped_lockable)
#define INTR_SCOPED_LOCKABLE __attribute__((scoped_lockable))
#else
#define INTR_SCOPED_LOCKABLE
#endif

#if INTR_HAS_ATTRIBUTE(unlock_function)
#define INTR_UNLOCK_FUNCTION(...) __attribute__((unlock_function(__VA_ARGS__)))
#else
#define INTR_UNLOCK_FUNCTION(...)
#endif

#if INTR_HAS_ATTRIBUTE(exclusive_lock_function)
#define INTR_EXCLUSIVE_LOCK_FUNCTION(...) \
  __attribute__((exclusive_lock_function(__VA_ARGS__)))
#else
#define INTR_EXCLUSIVE_LOCK_FUNCTION(...)
#endif

#if INTR_HAS_ATTRIBUTE(exclusive_trylock_function)
#define INTR_EXCLUSIVE_TRYLOCK_FUNCTION(...) \
  __attribute__((exclusive_trylock_function(__VA_ARGS__)))
#else
#define INTR_EXCLUSIVE_TRYLOCK_FUNCTION(...)
#endif

#if INTR_HAS_ATTRIBUTE(assert_exclusive_lock)
#define INTR_ASSERT_EXCLUSIVE_LOCK(...) \
  __attribute__((assert_exclusive_lock(__VA_ARGS__)))
#else
#define INTR_ASSERT_EXCLUSIVE_LOCK(...)
#endif

#if INTR_HAS_ATTRIBUTE(locks_excluded)
#define INTR_LOCKS_EXCLUDED(...) __attribute__((locks_excluded(__VA_ARGS__)))
#else
#define INTR_LOCKS_EXCLUDED(...)
#endif

#if INTR_HAS_ATTRIBUTE(exclusive_locks_required)
#define INTR_EXCLUSIVE_LOCKS_REQUIRED(...) \
  __attribute__((exclusive_locks_required(__VA_ARGS__)))
#else
#define INTR_EXCLUSIVE_LOCKS_REQUIRED(...)
#endif

#if INTR_HAS_ATTRIBUTE(no_thread_safety_analysis)
#define INTR_NO_THREAD_SAFETY_ANALYSIS \
  __attribute__((no_thread_safety_analysis))
#else
#define INTR_NO_THREAD_SAFETY_ANALYSIS
#endif

namespace intrinsic::thread_safety_internal {
// Takes a reference to a guarded data member, and returns an unguarded
// reference.
// Do not use this function directly, use INTR_TS_UNCHECKED_READ instead.
template <typename T>
inline const T& ts_unchecked_read(const T& v) INTR_NO_THREAD_SAFETY_ANALYSIS {
  return v;
}

template <typename T>
inline T& ts_unchecked_read(T& v) INTR_NO_THREAD_SAFETY_ANALYSIS {
  return v;
}
}  // namespace intrinsic::thread_safety_internal

#define INTR_TS_UNCHECKED_READ(x) \
  ::intrinsic::thread_safety_internal::ts_unchecked_read(x)

#endif  // ICON_UTILS_ATTRIBUTES_H_
