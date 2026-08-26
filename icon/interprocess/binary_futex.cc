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

#include "icon/interprocess/binary_futex.h"

#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <optional>
#include <utility>

#include "icon/utils/strerror.h"

namespace intrinsic {

namespace {

// Makes a futex() syscall with the given parameters.
//
// Check `man futex` for the semantics of the parameters – they differ based on
// `op`.
//
// The only non-standard parameter is `private_futex`. If that is true, this
// function sets the `FUTEX_PRIVATE_FLAG` bit in `op` before making the syscall.
//
// Returns different values depending on `op`, or -1 on error. Again, check
// `man futex` for details.
inline long futex(std::atomic<uint32_t>* uaddr, int futex_op, uint32_t val,
                  bool private_futex, const struct timespec* timeout = nullptr,
                  uint32_t* uaddr2 = nullptr, uint32_t val3 = 0) {
  if (private_futex) {
    // This option bit can be employed with all futex operations. It tells the
    // kernel that the futex is process-private and not shared with another
    // process (i.e., it is being used for synchronization only between threads
    // of the same process). This allows the kernel to make some
    // additional performance optimizations.
    futex_op |= FUTEX_PRIVATE_FLAG;
  }
  return ::syscall(SYS_futex, uaddr, futex_op, val, timeout, uaddr2,
                   FUTEX_BITSET_MATCH_ANY);
}

// Atomically reads from `val` and sets `val` to kReady *if* it was kPosted.
//
// Returns the value of `val` before the reset.
uint32_t TryWait(std::atomic<uint32_t>& val) {
  uint32_t expected = BinaryFutex::kPosted;
  // If `val != expected`, then this writes the _actual_ value of `val` into
  // `expected`.
  std::ignore =
      val.compare_exchange_strong(expected, BinaryFutex::kReady,
                                  /*success=*/std::memory_order_acq_rel,
                                  /*failure=*/std::memory_order_acquire);
  return expected;
}

// Wait until `val` changes from kReady to something else, or until
// `ts_absolute`.
//
// `ts_absolute` must be an absolute timeout in CLOCK_MONOTONIC.
RealtimeStatus Wait(std::atomic<uint32_t>& val, const timespec* ts_absolute,
                    bool private_futex) {
  const Time start_time = Now();
  while (true) {
    uint32_t value = TryWait(val);
    switch (value) {
      case BinaryFutex::kReady: {
        break;
      }
      case BinaryFutex::kClosed: {
        return FormatRealtimeStatus(StatusCode::kAborted,
                                    "BinaryFutex is closed, aborting Wait()");
      }
      case BinaryFutex::kPosted: {
        return RtOkStatus();
      }
      default: {
        return FormatRealtimeStatus(StatusCode::kInternal,
                                    "BinaryFutex took unexpected value: {}",
                                    value);
      }
    }

    // The value is not yet what we expect, but still kReady. Let's wait for it
    // to change. FUTEX_WAIT_BITSET sleeps for as long as the value is still
    // equal to kReady.
    //
    // N.B.: FUTEX_WAIT_BITSET interprets `ts_absolute` as an absolute timeout
    // in CLOCK_MONOTONIC (it would use CLOCK_REALTIME if we set
    // FUTEX_CLOCK_REALTIME).
    auto ret = futex(&val, FUTEX_WAIT_BITSET, BinaryFutex::kReady,
                     private_futex, ts_absolute);
    if (ret == -1 && errno == ETIMEDOUT) {
      return FormatRealtimeStatus(
          StatusCode::kDeadlineExceeded, "Timeout after {} ms",
          std::chrono::duration_cast<std::chrono::milliseconds>(Now() -
                                                                start_time)
              .count());
    }
    if (ret == -1 && errno != EAGAIN && errno != EINTR) {
      return FormatRealtimeStatus(StatusCode::kInternal,
                                  "Futex wait failed with error: {:s}",
                                  StrError(errno).data());
    }
  }
}

}  // namespace

BinaryFutex::BinaryFutex(bool posted, bool private_futex) noexcept
    : val_(posted ? kPosted : kReady), private_futex_(private_futex) {}

// Make sure to transfer the futex value when moving a BinaryFutex. We cannot
// transmit the actual futex (since that operates on a fixed address, and we do
// not want a heap-allocated std::unique_ptr here), but we _can_ transfer the
// value.
BinaryFutex::BinaryFutex(BinaryFutex&& other) noexcept
    : val_(other.val_.load(std::memory_order_acquire)),
      private_futex_(other.private_futex_.load(std::memory_order_acquire)) {}
BinaryFutex& BinaryFutex::operator=(BinaryFutex&& other) noexcept {
  if (this != &other) {
    val_.store(other.val_.load(std::memory_order_acquire));
    private_futex_.store(other.private_futex_.load(std::memory_order_acquire));
  }
  return *this;
}

BinaryFutex::~BinaryFutex() noexcept { Close(); }

RealtimeStatus BinaryFutex::Post() {
  uint32_t expected = kReady;
  // We need to make a copy of the private_futex_ member variable. Otherwise,
  // we might end up with a data race if the thread destructing the futex
  // reads `val_` between the `compare_exchange_strong` and the
  // `futex` call.
  const bool private_futex = private_futex_;
  // Take the address before, since the class instance could be destroyed before
  // `futex()` is called.
  std::atomic<uint32_t>* val_addr = &val_;
  if (val_.compare_exchange_strong(expected, kPosted,
                                   /*success=*/std::memory_order_acq_rel,
                                   /*failure=*/std::memory_order_acquire)) {
    // `futex` could fail with EFAULT, if `val_addr` is not a valid
    // user-space address anymore. This can happen if another thread destroyed
    // the BinaryFutex between the previous line and this one. Therefore, we
    // ignore the return value here. Another error value should only be EINVAL,
    // which should never happen here
    // ("EINVAL: The kernel detected an inconsistency between the user-space
    // state at uaddr and the kernel state—that is, it detected a waiter which
    // waits in FUTEX_LOCK_PI or FUTEX_LOCK_PI2 on uaddr.").
    //
    // One indicating that we wake up at most 1 other client.
    std::ignore = futex(val_addr, FUTEX_WAKE, 1, private_futex);
  } else if (expected == kClosed) {
    // When compare_exchange_strong returns false, it writes the current value
    // of the atomic into `expected`. We can check this to see if the
    // BinaryFutex is closed.
    return FormatRealtimeStatus(StatusCode::kAborted,
                                "BinaryFutex is closed, aborting Post()");
  }
  return RtOkStatus();
}

RealtimeStatus BinaryFutex::WaitUntil(Time deadline) const {
  if (deadline == Time::max()) {
    return Wait(val_, nullptr, private_futex_);
  }

  // Convert `deadline` from a chrono timepoint (in the steady_clock domain)
  // to a timespec (in the CLOCK_MONOTONIC domain).

  // First calculate how far in the future `deadline` is
  auto duration = deadline - Now();

  struct ::timespec now_ts;
  if (clock_gettime(CLOCK_MONOTONIC, &now_ts) != 0) {
    return FormatRealtimeStatus(StatusCode::kInternal,
                                "clock_gettime failed with error: {:s}",
                                StrError(errno).data());
  }
  // Let chrono handle the computation using a std::chrono::duration.
  const auto now_duration = std::chrono::seconds(now_ts.tv_sec) +
                            std::chrono::nanoseconds(now_ts.tv_nsec);
  const auto deadline_duration = now_duration + duration;
  const auto secs =
      std::chrono::duration_cast<std::chrono::seconds>(deadline_duration);
  const auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
      deadline_duration - secs);
  struct ::timespec deadline_ts;
  deadline_ts.tv_sec = std::max(0ll, static_cast<long long>(secs.count()));
  deadline_ts.tv_nsec = std::max(0ll, static_cast<long long>(ns.count()));

  return Wait(val_, &deadline_ts, private_futex_);
}

RealtimeStatus BinaryFutex::WaitFor(std::chrono::nanoseconds timeout) const {
  if (timeout == std::chrono::nanoseconds::max()) {
    return Wait(val_, nullptr, private_futex_);
  }

  return WaitUntil(Now() + timeout);
}

std::optional<bool> BinaryFutex::TryWait() const {
  switch (::intrinsic::TryWait(val_)) {
    case kReady:
      return false;
    case kPosted:
      return true;
    case kClosed:
    default:
      return std::nullopt;
  }
}

uint32_t BinaryFutex::Value() const noexcept { return val_.load(); }

void BinaryFutex::Close() noexcept {
  // We need to make a copy of the private_futex_ member variable. Otherwise,
  // we might end up with a data race if the thread destructing the futex
  // reads `val_` between the `compare_exchange_strong` and the
  // `futex` call.
  const bool private_futex = private_futex_;
  // Take the address before, since the class instance could be destroyed before
  // `futex()` is called.
  std::atomic<uint32_t>* val_addr = &val_;
  // Only signal waiters if the value changed (i.e. if it *wasn't already*
  // BinaryFutex::kClosed).
  if (val_.exchange(kClosed) != kClosed) {
    // `futex` could fail with EFAULT, if `val_addr` is not a valid
    // user-space address anymore. This can happen if another thread destroyed
    // the BinaryFutex between the previous line and this one. Therefore, we
    // ignore the return value here. Another error value should only be EINVAL,
    // which should never happen here
    // ("EINVAL: The kernel detected an inconsistency between the user-space
    // state at uaddr and the kernel state—that is, it detected a waiter which
    // waits in FUTEX_LOCK_PI or FUTEX_LOCK_PI2 on uaddr.").
    //
    // INT_MAX indicates that we wake up *all* waiters. We don't do this in
    // `Post()`, because waking an unknown number of waiters could take unbound
    // time and compromise realtime correctness, but since `Close()` happens on
    // shutdown, we're not worried about that here.
    std::ignore = futex(val_addr, FUTEX_WAKE, INT_MAX, private_futex);
  }
}

}  // namespace intrinsic
