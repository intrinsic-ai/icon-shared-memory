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

#include "util/thread/lockstep.h"

#include <chrono>
#include <utility>

namespace intrinsic {

Lockstep::Lockstep(Lockstep&& other) {
  a_finished_ = std::exchange(other.a_finished_, intrinsic::BinaryFutex(false));
  b_finished_ =
      std::exchange(other.b_finished_, intrinsic::BinaryFutex(/*posted=*/true));
  state_.store(other.state_.load());
}

Lockstep& Lockstep::operator=(Lockstep&& other) {
  if (this != &other) {
    a_finished_ =
        std::exchange(other.a_finished_, intrinsic::BinaryFutex(false));
    b_finished_ = std::exchange(other.b_finished_,
                                intrinsic::BinaryFutex(/*posted=*/true));
    state_.store(other.state_.load());
  }
  return *this;
}

RealtimeStatus Lockstep::StartOperationAWithDeadline(Time deadline) {
  // Wait for Operation B to finish
  if (auto status = b_finished_.WaitUntil(deadline); !status.ok()) {
    return status;
  }
  State expected = State::kBFinished;
  if (!state_.compare_exchange_strong(expected, State::kARunning,
                                      /*success=*/std::memory_order_acq_rel,
                                      /*failure=*/std::memory_order_acquire)) {
    if (expected == State::kCancelled) {
      // Ignore error because returning Aborted to the caller is more important.
      std::ignore = b_finished_.Post();

      return intrinsic::FormatRealtimeStatus(
          StatusCode::kAborted,
          "Not starting operation A: lockstep has been cancelled");
    } else {
      return intrinsic::FormatRealtimeStatus(
          StatusCode::kFailedPrecondition,
          "Not starting operation A: expected State::kBFinished");
    }
  }
  return RtOkStatus();
}

RealtimeStatus Lockstep::StartOperationAWithTimeout(
    std::chrono::nanoseconds timeout) {
  return StartOperationAWithDeadline(Now() + timeout);
}

RealtimeStatus Lockstep::EndOperationA() {
  if (state_ == State::kCancelled) {
    return RtOkStatus();
  }
  if (state_ != State::kARunning) {
    return intrinsic::FormatRealtimeStatus(
        StatusCode::kFailedPrecondition,
        "Not ending operation A: Did you call StartOperationA...?");
  }
  state_ = State::kAFinished;
  return a_finished_.Post();
}

RealtimeStatus Lockstep::StartOperationBWithDeadline(Time deadline) {
  if (auto status = a_finished_.WaitUntil(deadline); !status.ok()) {
    return status;
  }
  State expected = State::kAFinished;
  if (!state_.compare_exchange_strong(expected, State::kBRunning,
                                      /*success=*/std::memory_order_acq_rel,
                                      /*failure=*/std::memory_order_acquire)) {
    if (expected == State::kCancelled) {
      // Ignore error because returning Aborted to the caller is more important.
      std::ignore = a_finished_.Post();

      return intrinsic::FormatRealtimeStatus(
          StatusCode::kAborted,
          "Not starting operation B: lockstep has been cancelled");
    } else {
      return intrinsic::FormatRealtimeStatus(StatusCode::kFailedPrecondition,
                                             "Expected State::kAFinished");
    }
  }
  return RtOkStatus();
}

RealtimeStatus Lockstep::StartOperationBWithTimeout(
    std::chrono::nanoseconds timeout) {
  return StartOperationBWithDeadline(Now() + timeout);
}

RealtimeStatus Lockstep::EndOperationB() {
  State expected = State::kBRunning;
  if (!state_.compare_exchange_strong(expected, State::kBFinished,
                                      /*success=*/std::memory_order_acq_rel,
                                      /*failure=*/std::memory_order_acquire)) {
    if (expected == State::kCancelled) {
      return RtOkStatus();
    }

    return intrinsic::FormatRealtimeStatus(
        StatusCode::kFailedPrecondition,
        "Mismatched call to EndOperationB. Did you call StartOperationB...?");
  }
  return b_finished_.Post();
}

void Lockstep::Cancel(const log::Logger* logger) {
  state_ = State::kCancelled;
  if (auto status = a_finished_.Post(); !status.ok()) {
    auto message_view = status.GetMessage();
    INTRINSIC_SHARED_MEMORY_LOG(ERROR, logger, "{:.{}s}", message_view.data(),
                                message_view.size());
  }
  if (auto status = b_finished_.Post(); !status.ok()) {
    auto message_view = status.GetMessage();
    INTRINSIC_SHARED_MEMORY_LOG(ERROR, logger, "{:.{}s}", message_view.data(),
                                message_view.size());
  }
}

RealtimeStatus Lockstep::Reset(std::chrono::nanoseconds timeout) {
  if (state_ != State::kCancelled) {
    return intrinsic::FormatRealtimeStatus(
        StatusCode::kFailedPrecondition, "Reset expects a cancelled lockstep.");
  }
  // Acquire both futexes, so that any call to a `StartOperation...` function
  // will have to wait until the reset is done.
  auto deadline = Now() + timeout;
  if (auto status = a_finished_.WaitUntil(deadline); !status.ok()) {
    return status;
  }
  if (auto status = b_finished_.WaitUntil(deadline); !status.ok()) {
    return status;
  }
  state_ = State::kBFinished;
  // Let `StartOperationA...` be next.
  return b_finished_.Post();
}

}  // namespace intrinsic
