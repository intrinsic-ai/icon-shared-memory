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

#ifndef ICON_INTERPROCESS_LOCKABLE_BINARY_FUTEX_H_
#define ICON_INTERPROCESS_LOCKABLE_BINARY_FUTEX_H_

#include <cstddef>
#include <iostream>
#include <optional>

#include "icon/interprocess/binary_futex.h"
#include "icon/testing/realtime_annotations.h"
#include "icon/utils/attributes.h"
#include "icon/utils/check.h"
#include "icon/utils/status.h"
#include "icon/utils/time.h"

namespace intrinsic {

// Wrapper around `BinaryFutex` so that it can be used like a common mutex. It
// also implements the compiler attributes so that the mutex compile time checks
// work (e.g. `INTR_GUARDED_BY`).
//
// Caution: This class does not check if `Lock()`/`Unlock()` was called from the
// same thread! Whenever possible, use the `ScopedLock`!
class INTR_LOCKABLE LockableBinaryFutex {
 public:
  // If `private_futex` is true, the futex can only be used from the current
  // process. This can have performance benefits. If the futex is used in a
  // shared memory segment, set `private_futex` to false.
  explicit LockableBinaryFutex(bool private_futex = false)
      : futex_(/*posted = */ true,  // Set `posted` so that the user can and
                                    // must call `Lock()` before `Unlock()`.
               private_futex) {}

  LockableBinaryFutex(const LockableBinaryFutex&) = delete;
  LockableBinaryFutex& operator=(const LockableBinaryFutex&) = delete;
  LockableBinaryFutex(LockableBinaryFutex&&) = delete;
  LockableBinaryFutex& operator=(LockableBinaryFutex&&) = delete;

  ~LockableBinaryFutex() {
    // Check that the futex was unlocked.
    INTR_CHECK(futex_.Value() == 1,
               "Futex was not unlocked before destruction");
  }

  // Blocks the calling thread, if necessary, until this `BinaryFutex` is free,
  // and then acquires it exclusively (This lock is also known as a "write
  // lock.").
  //
  // Forwards error of `BinaryFutex::WaitUntil()`.
  RealtimeStatus Lock()
      INTR_EXCLUSIVE_LOCK_FUNCTION() INTRINSIC_NON_REALTIME_ONLY {
    return futex_.WaitUntil(Time::max());
  }

  // Releases this `BinaryFutex` and returns it from the exclusive/write state
  // to the free state. Calling thread must hold the `BinaryFutex` exclusively.
  // Forwards error of `BinaryFutex::Post()` and returns an error if the futex
  // is not held at the start of the function.
  RealtimeStatus Unlock() INTR_UNLOCK_FUNCTION() INTRINSIC_CHECK_REALTIME_SAFE {
    if (!IsHeld()) {
      return FormatRealtimeStatus(StatusCode::kFailedPrecondition,
                                  "Futex is not locked");
    }
    return futex_.Post();
  }

  // If the mutex can be acquired without blocking, does so exclusively and
  // returns `true`. Otherwise, returns `false`.
  INTR_MUST_USE_RESULT bool TryLock() INTRINSIC_CHECK_REALTIME_SAFE
      INTR_EXCLUSIVE_TRYLOCK_FUNCTION(true) {
    // There's no reason why `futex_` would be closed (this class is fully in
    // control of `futex_` and doesn't close it until it's destroyed), so treat
    // a `std::nullopt` return as `false` (if it ever happens, this is correct –
    // we did not lock the futex in this case).
    return futex_.TryWait().value_or(false);
  }

  // Returns if the `BinaryFutex` is held.
  bool IsHeld() const INTRINSIC_CHECK_REALTIME_SAFE {
    return futex_.Value() == 0;
  }

  // Asserts that the `BinaryFutex` is held.
  void AssertHeld() const INTR_ASSERT_EXCLUSIVE_LOCK() {
    INTR_CHECK(IsHeld(), "LockableBinaryFutex is not held when it should be");
  }

 private:
  BinaryFutex futex_;
};

// RAII wrapper around `LockableBinaryFutex`.
class INTR_SCOPED_LOCKABLE BinaryFutexLock {
 public:
  // Waits indefinitely for `mutex` to become available and locks it. There is
  // no order who will get the next lock in case multiple parties wait for the
  // lock. `mutex` must be non-null. Will fail fatally, if the futex syscall
  // fails.
  explicit BinaryFutexLock(LockableBinaryFutex& mutex)
      INTR_EXCLUSIVE_LOCK_FUNCTION(mutex)
      : mutex_(mutex) {
    INTR_CHECK(mutex_.Lock().ok(),
               "LockableBinaryFutexLock failed to lock futex");
  }

  BinaryFutexLock(const BinaryFutexLock&) = delete;
  BinaryFutexLock(BinaryFutexLock&&) = delete;
  BinaryFutexLock& operator=(const BinaryFutexLock&) = delete;
  BinaryFutexLock& operator=(BinaryFutexLock&&) = delete;

  ~BinaryFutexLock() INTR_UNLOCK_FUNCTION() {
    INTR_CHECK(mutex_.Unlock().ok(),
               "LockableBinaryFutexLock failed to unlock futex");
  }
  // This class should *not* live on the heap. It should only live in the
  // current scope.
  void* operator new(std::size_t) = delete;
  void operator delete(void*) = delete;

 private:
  LockableBinaryFutex& mutex_;
};

}  // namespace intrinsic

#endif  // ICON_INTERPROCESS_LOCKABLE_BINARY_FUTEX_H_
