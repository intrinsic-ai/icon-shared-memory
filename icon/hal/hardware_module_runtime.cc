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

#include "icon/hal/hardware_module_runtime.h"

#include <pthread.h>
#include <sched.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <list>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#include "flatbuffer_definitions/icon/hal/interfaces/control_period.fbs.h"
#include "flatbuffer_definitions/icon/hal/interfaces/hardware_module_state.fbs.h"
#include "flatbuffer_definitions/icon/hal/interfaces/icon_state.fbs.h"
#include "icon/hal/control_period_register.h"  // IWYU pragma: keep
#include "icon/hal/hardware_interface_handle.h"
#include "icon/hal/hardware_interface_registry.h"
#include "icon/hal/hardware_interface_traits.h"
#include "icon/hal/hardware_module_init_context.h"
#include "icon/hal/hardware_module_interface.h"
#include "icon/hal/icon_state_register.h"  // IWYU pragma: keep
#include "icon/hal/interfaces/control_period_utils.h"
#include "icon/hal/interfaces/hardware_module_state_utils.h"
#include "icon/interprocess/remote_trigger/remote_trigger_server.h"
#include "icon/interprocess/shared_memory_manager/domain_socket_server.h"
#include "icon/interprocess/shared_memory_manager/domain_socket_utils.h"
#include "icon/interprocess/shared_memory_manager/shared_memory_manager.h"
#include "icon/testing/realtime_annotations.h"
#include "icon/utils/async_buffer.h"
#include "icon/utils/async_request.h"
#include "icon/utils/attributes.h"
#include "icon/utils/cleanup.h"
#include "icon/utils/log.h"
#include "icon/utils/mutex.h"
#include "icon/utils/realtime_guard.h"
#include "icon/utils/status.h"
#include "icon/utils/status_and_expected_macros.h"
#include "icon/utils/time.h"
#include "platform/common/buffers/rt_promise.h"
#include "platform/common/buffers/rt_queue.h"
#include "platform/common/buffers/rt_queue_multi_writer.h"
#include "util/thread/thread_options.h"
#include "tl/expected.hpp"

namespace intrinsic::icon {

namespace hardware_interface_traits {
INTRINSIC_ADD_HARDWARE_INTERFACE(intrinsic_fbs::HardwareModuleState,
                                 intrinsic_fbs::BuildHardwareModuleState,
                                 "intrinsic_fbs.HardwareModuleState")
}  // namespace hardware_interface_traits

namespace {

// Maximum length of a thread name for `pthread_setname_p` (it accepts a C-style
// string up to 16 characters, but that includes the terminating zero).
constexpr int kMaxThreadNameLength = 15;

// Factory for a thread prelude functor.
//
// When a new thread starts, executing the returned functor inside that thread
// applies the requested `ThreadOptions` (thread name via `pthread_setname_np`,
// real-time scheduling policy and priority via `pthread_setschedparam`, and CPU
// affinity mask via `pthread_setaffinity_np`).
// Returns `OkStatus()` on success, or an error Status if configuration fails.
RemoteTriggerServer::Prelude MakeThreadPrelude(
    const ThreadOptions& options,
    const log::Logger* logger INTR_ATTRIBUTE_LIFETIME_BOUND) {
  return [options, logger]() -> Status {
    const pthread_t thread_handle = ::pthread_self();
    if (!options.name.empty()) {
      const std::string short_name =
          options.name.substr(0, kMaxThreadNameLength);
      if (int err = ::pthread_setname_np(thread_handle, short_name.c_str());
          err != 0) {
        return FormatStatus(StatusCode::kInternal,
                            "Failed to set thread name: {:s}", strerror(err));
      }
    }
    std::optional<int> policy;
    std::optional<int> priority;
    if (options.is_realtime) {
      if (options.schedule_policy == std::nullopt) {
        INTRINSIC_SHARED_MEMORY_LOG_WARNING(
            logger,
            "Realtime thread '{}' does not specify a scheduler. This is likely "
            "a bug! Falling back to default realtime policy ({}).",
            options.name, ThreadOptions::kDefaultRealtimeScheduler);
        policy = ThreadOptions::kDefaultRealtimeScheduler;
      } else {
        policy = options.schedule_policy.value();

        if (policy == SCHED_OTHER) {
          INTRINSIC_SHARED_MEMORY_LOG_WARNING(
              logger,
              "Realtime thread '{}' uses SCHED_OTHER. This is likely a bug!",
              options.name);
        }
      }

      if (options.priority == std::nullopt) {
        INTRINSIC_SHARED_MEMORY_LOG_WARNING(
            logger,
            "Realtime thread '{}' does not specify a priority. This may impact "
            "performance. Defaulting to {}",
            options.name, ThreadOptions::kDefaultRealtimePriority);
        priority = ThreadOptions::kDefaultRealtimePriority;
      } else {
        priority = options.priority.value();
      }
    }
    // Only set scheduling parameters if at least one of `priority` and `policy`
    // is set in the ThreadOptions.
    if (priority.has_value() || policy.has_value()) {
      sched_param sch;
      // If the thread were a realtime one, both `priority` and `policy` would
      // be populated already (see above).
      //
      // That means it's safe to fall back to the non-realtime defaults for any
      // unset values.
      sch.sched_priority =
          priority.value_or(ThreadOptions::kDefaultNonRealtimePriority);
      if (int err = ::pthread_setschedparam(
              thread_handle,
              policy.value_or(ThreadOptions::kDefaultNonRealtimeScheduler),
              &sch);
          err != 0) {
        if (err == EPERM) {
          return FormatStatus(
              StatusCode::kPermissionDenied,
              "Permission denied setting priority and scheduler");
        }
        return FormatStatus(StatusCode::kInternal,
                            "Failed to set thread scheduling: {:s}",
                            strerror(err));
      }
    }

    if (!options.cpu_affinity.empty()) {
      cpu_set_t cpuset;
      CPU_ZERO(&cpuset);
      for (int cpu : options.cpu_affinity) {
        CPU_SET(cpu, &cpuset);
      }
      if (int err = ::pthread_setaffinity_np(thread_handle, sizeof(cpu_set_t),
                                             &cpuset);
          err != 0) {
        return FormatStatus(StatusCode::kInternal,
                            "Failed to set CPU affinity: {:s}", strerror(err));
      }
    }
    return OkStatus();
  };
}

constexpr size_t kMaxPendingRequests = 10;

}  // namespace

class HardwareModuleRuntime::CallbackHandler final {
  struct AsyncRequestData {
    intrinsic_fbs::StateCode from = intrinsic_fbs::StateCode::kDeactivated;
    intrinsic_fbs::StateCode to = intrinsic_fbs::StateCode::kDeactivated;
    std::array<char, intrinsic_fbs::kMaxFaultReasonLength> message{};
    Time timestamp{};
  };
  using AsyncRequestType =
      intrinsic::icon::AsyncRequest<AsyncRequestData, RealtimeStatus>;

 public:
  explicit CallbackHandler(
      std::string_view name, HardwareModuleInterface* instance,
      intrinsic_fbs::HardwareModuleState* hardware_module_state,
      const log::Logger* logger INTR_ATTRIBUTE_LIFETIME_BOUND) noexcept
      : name_(name),
        instance_(instance),
        shared_memory_hardware_module_state_(hardware_module_state),
        request_queue_(kMaxPendingRequests),
        logger_(logger) {
    SetStateDirectly(intrinsic_fbs::StateCode::kDeactivated, RtOkStatus(),
                     /*force=*/true);
  }

  std::string_view name() const { return name_; }
  ~CallbackHandler() {
    Shutdown();
    if (!action_lock_.TryLock()) {
      INTRINSIC_SHARED_MEMORY_LOG(
          FATAL, logger_,
          "CallbackHandler destroyed while an action is still ongoing - this "
          "is likely a bug in the HardwareModuleRuntime shutdown logic.");
    }
    action_lock_.Unlock();
  }

  // Server callback to trigger `Prepare` on the hardware module.
  void OnPrepare() {
    switch (hardware_module_state_code_.load(std::memory_order_acquire)) {
      case intrinsic_fbs::StateCode::kActivated:
      case intrinsic_fbs::StateCode::kMotionEnabled:
      case intrinsic_fbs::StateCode::kMotionEnabling:
      case intrinsic_fbs::StateCode::kMotionDisabling:
      case intrinsic_fbs::StateCode::kFaulted:
      case intrinsic_fbs::StateCode::kClearingFaults:
      case intrinsic_fbs::StateCode::kPreparing:
        OnDeactivate();
        break;
      default:
        break;
    }

    if (!SetStateDirectly(intrinsic_fbs::StateCode::kPreparing)) {
      return;
    }
    CancelPendingRequests("Request cancelled by a call to Prepare()");
    if (auto ret = instance_->Prepare(); !ret.ok()) {
      INTRINSIC_SHARED_MEMORY_LOG(ERROR, logger_,
                                  "PUBLIC: Call to 'Prepare' failed: {:s}",
                                  ret.message);
      SetStateDirectly(intrinsic_fbs::StateCode::kFatallyFaulted,
                       FormatRealtimeStatus(ret.code, "{:s}", ret.message));
    } else {
      SetStateDirectly(intrinsic_fbs::StateCode::kPrepared);
    }
  }

  // Server callback for trigger `Activate` on the hardware module.
  void OnActivate() {
    // The ICON main loop shall not be running yet, so we can and must set the
    // shared memory state directly.
    if (!SetStateDirectly(intrinsic_fbs::StateCode::kActivating)) {
      return;
    }
    CancelPendingRequests("Request cancelled due to activation");
    if (auto ret = instance_->Activate(); !ret.ok()) {
      INTRINSIC_SHARED_MEMORY_LOG(ERROR, logger_,
                                  "PUBLIC: Call to 'Activate' failed: {:s}",
                                  ret.GetMessage());
      SetStateDirectly(intrinsic_fbs::StateCode::kFatallyFaulted, ret);
    } else {
      SetStateDirectly(intrinsic_fbs::StateCode::kActivated);
    }
    reject_new_requests_.store(false, std::memory_order_release);
  }

  // Server callback for trigger `Deactivate` on the hardware module.
  void OnDeactivate() {
    // The ICON main loop shall not be running anymore, so we
    // can and must set the shared memory state directly.
    if (!SetStateDirectly(intrinsic_fbs::StateCode::kDeactivating)) {
      return;
    }
    // It is possible that ongoing calls (e.g. EnableMotion) might miss the
    // `CancelPendingRequests()`, but we cannot get the `non_rt_buffer_lock_`
    // here. The worst that could happen is that unlucky requests will time
    // out and the remaining data will be cleaned up safely in the destructor or
    // on the next call to `SetStateAndWait()`.
    reject_new_requests_.store(true, std::memory_order_release);
    CancelPendingRequests("Request cancelled due to deactivation");

    if (auto ret = instance_->Deactivate(); !ret.ok()) {
      INTRINSIC_SHARED_MEMORY_LOG(ERROR, logger_,
                                  "PUBLIC: Call to 'Deactivate' failed: {:s}",
                                  ret.GetMessage());
      SetStateDirectly(intrinsic_fbs::StateCode::kFatallyFaulted, ret);
    } else {
      SetStateDirectly(intrinsic_fbs::StateCode::kDeactivated);
    }
  }

  // Server callback for trigger `EnableMotion` on the hardware module.
  void OnEnableMotion() {
    MutexLock lock(&action_lock_);
    if (!SetStateAndWait(
            hardware_module_state_code_.load(std::memory_order_acquire),
            intrinsic_fbs::StateCode::kMotionEnabling)) {
      return;
    }
    if (auto ret = instance_->EnableMotion(); !ret.ok()) {
      INTRINSIC_SHARED_MEMORY_LOG(ERROR, logger_,
                                  "PUBLIC: Call to 'EnableMotion' failed: {:s}",
                                  ret.message);
      SetStateAndWait(intrinsic_fbs::StateCode::kMotionEnabling,
                      ret.code == StatusCode::kAborted
                          ? intrinsic_fbs::StateCode::kFatallyFaulted
                          : intrinsic_fbs::StateCode::kFaulted,
                      ret.message);
    } else {
      SetStateAndWait(intrinsic_fbs::StateCode::kMotionEnabling,
                      intrinsic_fbs::StateCode::kMotionEnabled, "");
    }
  }

  // Server callback for trigger `DisableMotion` on the hardware module.
  void OnDisableMotion() {
    MutexLock lock(&action_lock_);

    if (!SetStateAndWait(
            hardware_module_state_code_.load(std::memory_order_acquire),
            intrinsic_fbs::StateCode::kMotionDisabling)) {
      return;
    }
    INTRINSIC_SHARED_MEMORY_LOG(INFO, logger_,
                                "PUBLIC: 'DisableMotion' called.");
    if (auto ret = instance_->DisableMotion(); !ret.ok()) {
      INTRINSIC_SHARED_MEMORY_LOG(
          ERROR, logger_, "PUBLIC: Call to 'DisableMotion' failed: {:s}",
          ret.message);
      SetStateAndWait(intrinsic_fbs::StateCode::kMotionDisabling,
                      ret.code == StatusCode::kAborted
                          ? intrinsic_fbs::StateCode::kFatallyFaulted
                          : intrinsic_fbs::StateCode::kFaulted,
                      ret.message);
    } else {
      SetStateAndWait(intrinsic_fbs::StateCode::kMotionDisabling,
                      intrinsic_fbs::StateCode::kActivated, "");
    }
  }

  // Server callback for trigger `ClearFaults` on the hardware module.
  void OnClearFaults() {
    MutexLock lock(&action_lock_);
    if (!SetStateAndWait(
            hardware_module_state_code_.load(std::memory_order_acquire),
            intrinsic_fbs::StateCode::kClearingFaults)) {
      return;
    }
    if (auto ret = instance_->ClearFaults(); !ret.ok()) {
      INTRINSIC_SHARED_MEMORY_LOG(ERROR, logger_,
                                  "PUBLIC: Call to 'ClearFaults' failed: {:s}",
                                  ret.message);
      SetStateAndWait(intrinsic_fbs::StateCode::kClearingFaults,
                      ret.code == StatusCode::kAborted
                          ? intrinsic_fbs::StateCode::kFatallyFaulted
                          : intrinsic_fbs::StateCode::kFaulted,
                      ret.message);
    } else {
      SetStateAndWait(intrinsic_fbs::StateCode::kClearingFaults,
                      intrinsic_fbs::StateCode::kActivated, "");
    }
  }

  // Server callback for trigger `ReadStatus` on the hardware module.
  void OnReadStatus() INTRINSIC_CHECK_REALTIME_SAFE {
    // TODO: Re-enable MallocGuard once available externally via
    // BCR. ScopedThreadLocalReaction scoped_malloc_reaction(
    //     MallocGuardReaction::kStoreViolationWithTrace);

    // The HWM state must only be written in the RT thread, when the HWM is
    // activated. Therefore, the processing of requests must take place in this
    // function, which is always called when the HWM is activated.
    ProcessNextPendingRequest();

    // Trigger the transition hook in the first cycle where the current state is
    // `kMotionEnabled` (set by the Runtime in the previous cycle), so that ICON
    // calls`ApplyCommand()` as well in this cycle. Don't call this function
    // directly after setting `kMotionEnabled` since ICON won't know about this
    // state change until the next cycle and won't call `ApplyCommand()` in this
    // cycle yet.
    CheckAndTriggerEnabledTransitionHook(
        previous_cycle_hardware_module_state_code_.load(
            std::memory_order_acquire),
        hardware_module_state_code_.load(std::memory_order_acquire));

    if (auto ret = instance_->ReadStatus();
        !ret.ok() &&
        hardware_module_state_code_.load(std::memory_order_acquire) !=
            intrinsic_fbs::StateCode::kClearingFaults) {
      INTRINSIC_SHARED_MEMORY_LOG(ERROR, logger_,
                                  "PUBLIC: Call to 'ReadStatus' failed: {:s}",
                                  ret.GetMessage());
      if (SetStateDirectly(ret.code == StatusCode::kAborted
                               ? intrinsic_fbs::StateCode::kFatallyFaulted
                               : intrinsic_fbs::StateCode::kFaulted,
                           ret)) {
        // Cancel all requests since we got a new error.
        CancelPendingRequests("Request cancelled due to error in ReadStatus");
      }
    }

#if 0
    // TODO: Re-enable MallocGuard once available externally via
    // BCR.
    if (icon::GetThreadLocalMallocViolations().num_violations > 0) {
      const auto message = FormatRealtimeStatus(
          StatusCode::kResourceExhausted,
          "ReadStatus() allocated {:d} bytes on the heap.",
          icon::GetThreadLocalMallocViolations().allocated_bytes.load());
      if (SetStateDirectly(intrinsic_fbs::StateCode::kFaulted, message)) {
        // Cancel all requests since we got a new error
        CancelPendingRequests(
            "Request cancelled due to realtime violation in ReadStatus");
      }
    }
#endif
  }

  // Server callback for trigger `ApplyCommand` on the hardware module.
  void OnApplyCommand() INTRINSIC_CHECK_REALTIME_SAFE {
    const auto current_state =
        hardware_module_state_code_.load(std::memory_order_acquire);
    if (current_state == intrinsic_fbs::StateCode::kMotionDisabling) {
      return;
    } else if (current_state != intrinsic_fbs::StateCode::kMotionEnabled)
        [[unlikely]] {
      const auto message = "PUBLIC: 'ApplyCommand' called while not enabled.";
      INTRINSIC_SHARED_MEMORY_LOG(WARNING, logger_, "{:s}", message);
      if (SetStateDirectly(intrinsic_fbs::StateCode::kFaulted,
                           FormatRealtimeStatus(StatusCode::kFailedPrecondition,
                                                "{:s}", message))) {
        // Cancel all requests since we got a new error
        CancelPendingRequests("Request cancelled due to error in ApplyCommand");
      }
      return;
    }

#if 0
    // TODO: Re-enable MallocGuard once available externally via
    // BCR.
     ScopedThreadLocalReaction scoped_malloc_reaction(
         icon::MallocGuardReaction::kStoreViolationWithTrace);
#endif
    if (auto ret = instance_->ApplyCommand(); !ret.ok()) {
      INTRINSIC_SHARED_MEMORY_LOG(ERROR, logger_,
                                  "PUBLIC: Call to 'ApplyCommand' failed: {:s}",
                                  ret.GetMessage());
      if (SetStateDirectly(ret.code == StatusCode::kAborted
                               ? intrinsic_fbs::StateCode::kFatallyFaulted
                               : intrinsic_fbs::StateCode::kFaulted,
                           ret)) {
        // Cancel all requests since we got a new error
        CancelPendingRequests("Request cancelled due to error in ApplyCommand");
      }
    }
#if 0
    // TODO: Re-enable MallocGuard once available externally via
    // BCR.
    if (icon::GetThreadLocalMallocViolations().num_violations > 0) {
      const auto message = FormatRealtimeStatus(
          StatusCode::kResourceExhausted,
          "ApplyCommand() allocated {:d} bytes on the heap.",
          icon::GetThreadLocalMallocViolations().allocated_bytes.load());
      if (SetStateDirectly(intrinsic_fbs::StateCode::kFaulted, message)) {
        // Cancel all requests since we got a new error.
        CancelPendingRequests(
            "Request cancelled due to realtime violation in ApplyCommand");
      }
    }
#endif
  }

  // Sets the internal state *and* the state in shared memory directly. Only
  // call this, when you *know* that no other thread/process might be reading
  // the state in shared memory at the same time.
  // Returns true, if the state was set and different than before.
  bool SetStateDirectly(intrinsic_fbs::StateCode state,
                        RealtimeStatus status = RtOkStatus(),
                        bool force = false,
                        bool silent = false) INTRINSIC_CHECK_REALTIME_SAFE {
    const auto current_state =
        hardware_module_state_code_.load(std::memory_order_acquire);
    if (auto result = HardwareModuleTransitionGuard(current_state, state);
        !force && result != TransitionGuardResult::kAllowed) {
      if (!silent && result == TransitionGuardResult::kProhibited) {
        INTRINSIC_SHARED_MEMORY_LOG(
            ERROR, logger_, "Switching from {:s} to {:s} is prohibited!",
            intrinsic_fbs::EnumNameStateCode(current_state),
            intrinsic_fbs::EnumNameStateCode(state));
      }
      return false;
    }
    const std::string_view fault_reason = status.GetMessage();
    if (!silent && current_state != state) {
      if (fault_reason.empty()) {
        INTRINSIC_SHARED_MEMORY_LOG(
            INFO, logger_, "Switching from {:s} to {:s}",
            intrinsic_fbs::EnumNameStateCode(current_state),
            intrinsic_fbs::EnumNameStateCode(state));
      } else {
        INTRINSIC_SHARED_MEMORY_LOG(
            INFO, logger_, "Switching from {:s} to {:s} with message '{:s}'",
            intrinsic_fbs::EnumNameStateCode(current_state),
            intrinsic_fbs::EnumNameStateCode(state), fault_reason);
      }
    }
    if (current_state == state &&
        intrinsic_fbs::GetMessage(shared_memory_hardware_module_state_) ==
            fault_reason) {
      return false;
    }

    const bool state_changed = current_state != state;
    RealtimeStatus current_status = status;
    if (state_changed) {
      if (auto hook_status =
              CheckAndTriggerDisabledTransitionHook(current_state);
          !hook_status.ok()) {
        if (state != intrinsic_fbs::StateCode::kInitFailed &&
            state != intrinsic_fbs::StateCode::kFatallyFaulted &&
            state != intrinsic_fbs::StateCode::kFaulted) {
          INTRINSIC_SHARED_MEMORY_LOG(ERROR, logger_,
                                      "PUBLIC: Disabled() failed: {:s}",
                                      hook_status.GetMessage());
          current_status =
              FormatRealtimeStatus(hook_status.code, "Disabled() failed: {:s}",
                                   hook_status.GetMessage());
          state = intrinsic_fbs::StateCode::kFaulted;
        }
      }
    }
    hardware_module_state_code_.store(state, std::memory_order_release);
    hardware_module_state_update_time_ = Now();
    intrinsic_fbs::SetState(shared_memory_hardware_module_state_, state,
                            current_status.GetMessage());
    // Publish the state for non-rt threads. We can use it here without a lock
    // since this function should not be called in parallel.
    auto& hwm_state_buffer = INTR_TS_UNCHECKED_READ(hwm_state_buffer_);
    auto* free_buffer = hwm_state_buffer.GetFreeBuffer();
    intrinsic_fbs::SetState(free_buffer, state, current_status.GetMessage());
    return hwm_state_buffer.CommitFreeBuffer() && state_changed;
  }

  // Cancels all pending requests. Call only from ICON lockstep thread or when
  // you *know* that the ICON lockstep thread is not running.
  void CancelPendingRequests(std::string_view cancel_reason)
      INTRINSIC_CHECK_REALTIME_SAFE {
    while (!request_queue_.reader()->Empty()) {
      AsyncRequestType async_request = std::move(
          *request_queue_.reader()
               ->Front());  // Needs to be moved so that it will get
                            // destroyed when leaving this scope. Otherwise
                            // the future in the other thread will wait
                            // forever for this promise to get destroyed.
      INTRINSIC_SHARED_MEMORY_LOG(
          INFO, logger_, "Canceling request to switch to {:s}: {:s}",
          intrinsic_fbs::EnumNameStateCode(async_request.GetRequest().to),
          cancel_reason);
      std::ignore = async_request.SetResponse(
          FormatRealtimeStatus(StatusCode::kCancelled, "{:s}", cancel_reason));
      request_queue_.reader()->DropFront();
    }
  }

  void Shutdown() {
    MutexLock lock(
        &non_rt_buffer_lock_);  // Lock the write buffer so that no new request
                                // can be inserted in the meanwhile.
    reject_new_requests_.store(true, std::memory_order_release);
    CancelPendingRequests("Request cancelled due to shutdown");
  }

  intrinsic_fbs::HardwareModuleState GetHardwareModuleState() {
    MutexLock lock(&non_rt_buffer_lock_);
    intrinsic_fbs::HardwareModuleState* state = nullptr;

    if (!hwm_state_buffer_.GetActiveBuffer(&state) || state == nullptr) {
      return intrinsic_fbs::HardwareModuleState();
    }
    return *state;
  }

 private:
  // Sets the state to `to` and waits until the state has been processed.
  // The functions queues a new rt-promise as a request to change the state in
  // the rt thread and waits for the completion of the promise using a future.
  // Attaches `fault_reason` to the new state.
  //
  // Returns true if the state was set successfully.
  // It checks whether the transition `from` to `to` is allowed. If not, returns
  // false.
  // If the transition would be a no-op, it returns false.
  // Returns false on any error as well.
  bool SetStateAndWait(intrinsic_fbs::StateCode from,
                       intrinsic_fbs::StateCode to,
                       std::string_view fault_reason = "")
      INTR_EXCLUSIVE_LOCKS_REQUIRED(
          action_lock_)  // Should only be called from non-rt actions such as
                         // EnableMotion, so the lock should be held.
      INTRINSIC_NON_REALTIME_ONLY {
    if (auto result = HardwareModuleTransitionGuard(from, to);
        result != TransitionGuardResult::kAllowed) {
      if (result == TransitionGuardResult::kProhibited) {
        INTRINSIC_SHARED_MEMORY_LOG(
            ERROR, logger_, "Switching from {:s} to {:s} is prohibited!",
            intrinsic_fbs::EnumNameStateCode(from),
            intrinsic_fbs::EnumNameStateCode(to));
      }
      return false;
    }

    {
      // Check all abandoned futures to see if they can be destroyed. This can
      // only happen when `Deactivate()` is called while another Action is
      // active and the timing is very unlucky. See comments in
      // `OnDeactivate()` for more details.
      MutexLock lock(&non_rt_buffer_lock_);
      std::erase_if(future_hospice_, [](const auto& future_ptr) {
        // Using `IsWaitFreeDestructible()` is safe here, since we never get
        // promises from futures in the hospice.
        return future_ptr->IsWaitFreeDestructible();
      });
      constexpr size_t kAbandonedFutureWarnLimit = 100;
      if (future_hospice_.size() >= kAbandonedFutureWarnLimit) {
        INTRINSIC_SHARED_MEMORY_LOG(
            WARNING, logger_,
            "Found {:d} abandoned futures. This indicates a bug in the "
            "HardwareModuleRuntime::CallbackHandler.",
            future_hospice_.size());
      }
    }

    auto state_change_status = [&]() -> Status {
      auto future =
          std::make_unique<intrinsic::RealtimeFuture<RealtimeStatus>>();
      auto promise_res = future->GetPromise();
      if (!promise_res.has_value()) {
        return ToStatus(promise_res.error());
      }
      auto promise = std::move(promise_res.value());
      {
        MutexLock lock(&non_rt_buffer_lock_);
        if (reject_new_requests_.load(std::memory_order_acquire)) {
          return FormatStatus(StatusCode::kFailedPrecondition,
                              "Request cancelled due to deactivation");
        }
        AsyncRequestData req_data{from, to, {}, Now()};
        const size_t copy_len =
            std::min(sizeof(req_data.message) - 1, fault_reason.size());
        std::memcpy(req_data.message.data(), fault_reason.data(), copy_len);
        req_data.message[copy_len] = '\0';

        INTR_RETURN_STATUS_IF_ERROR(request_queue_writer_.Insert(
            AsyncRequestType(req_data, std::move(promise))));
      }

      // Timeout until the state should have been processed. The state is
      // processed in every realtime cycle, so 10 seconds should be sufficient
      // and never be reached.
      constexpr auto kStatechangeRequestTimeout = std::chrono::seconds(10);
      auto status = future->WaitForAndGet(kStatechangeRequestTimeout);

      // If we get a timeout, it is likely that the future can also not be
      // destroyed. This can happen when `Deactivate()` is called while another
      // action is active and the timing is very unlucky. See comments in
      // `OnDeactivate()` for more details. To not block  until shutdown, we
      // move the future away. Using `IsWaitFreeDestructible()` is safe here,
      // since this function is the sole owner of the future, the future will be
      // destroyed just after the function returns and no new promise will be
      // attached until then.
      if (!future->IsWaitFreeDestructible()) {
        MutexLock lock(&non_rt_buffer_lock_);
        future_hospice_.push_back(std::move(future));
      }

      INTR_RETURN_STATUS_IF_ERROR(status);
      return ToStatus(status.value());
    }();
    if (!state_change_status.ok()) {
      INTRINSIC_SHARED_MEMORY_LOG(
          ERROR, logger_, "State change request to {:s} failed: {:s}",
          intrinsic_fbs::EnumNameStateCode(to), state_change_status.message);
      return false;
    }
    return true;
  }

  // Processes the next pending request in the queue to change the HWM state.
  // Reports the result back via the rt-promise.
  //
  // If the request is outdated, cancels the request.
  // Checks if the request is valid. If the request is valid, applies it to the
  // HWM state. Otherwise, reports the error via the rt-promise.
  void ProcessNextPendingRequest() INTRINSIC_CHECK_REALTIME_SAFE {
    previous_cycle_hardware_module_state_code_.store(
        hardware_module_state_code_.load(std::memory_order_acquire),
        std::memory_order_release);
    if (!request_queue_.reader()->Empty()) {
      // We need to move the promise (contained in `AsyncRequest`) so that it
      // will get destroyed when leaving this scope. Otherwise the future in the
      // other thread will wait forever for this promise to get destroyed.
      AsyncRequestType item = std::move(*request_queue_.reader()->Front());
      const AsyncRequestData newest_data = item.GetRequest();
      request_queue_.reader()->DropFront();

      RealtimeStatus status;
      const auto current_state =
          hardware_module_state_code_.load(std::memory_order_acquire);
      if (newest_data.timestamp >= hardware_module_state_update_time_ &&
          newest_data.from == current_state) {
        const RealtimeStatus req_status = FormatRealtimeStatus(
            StatusCode::kUnknown, "{:s}", newest_data.message.data());
        const bool allowed = SetStateDirectly(
            newest_data.to, req_status, /*force=*/false, /*silent=*/false);
        status = item.SetResponse(
            allowed ? RtOkStatus()
                    : FormatRealtimeStatus(
                          StatusCode::kFailedPrecondition,
                          "Transition from {:s} to {:s} is prohibited!",
                          intrinsic_fbs::EnumNameStateCode(current_state),
                          intrinsic_fbs::EnumNameStateCode(newest_data.to)));
      } else {
        status = item.SetResponse(FormatRealtimeStatus(
            StatusCode::kCancelled, "Request cancelled due to newer request"));
      }
      if (!status.ok()) {
        INTRINSIC_SHARED_MEMORY_LOG(ERROR, logger_,
                                    "Failed to set reply to non rt-call: {:s}",
                                    status.GetMessage());
      }
    }
  }

  // Checks if the transition from `from` to `to` is `kMotionEnabling` to
  // `kMotionEnabled` and, if so, calls `Enabled()` on the hardware
  // module. Must be called from the rt thread and just after
  // `SetStateDirectly()`.
  void CheckAndTriggerEnabledTransitionHook(intrinsic_fbs::StateCode from,
                                            intrinsic_fbs::StateCode to)
      INTRINSIC_CHECK_REALTIME_SAFE {
    if (from == intrinsic_fbs::StateCode::kMotionEnabling &&
        to == intrinsic_fbs::StateCode::kMotionEnabled) {
      if (auto status = instance_->Enabled(); !status.ok()) {
        SetStateDirectly(
            intrinsic_fbs::StateCode::kFaulted,
            FormatRealtimeStatus(status.code, "Enabled() callback failed: {:s}",
                                 status.GetMessage()),
            /*force=*/false, /*silent=*/true);
      } else {
        INTRINSIC_SHARED_MEMORY_LOG(INFO, logger_, "Motion Enabled");
      }
    }
  }

  // Checks if `from` state is `kMotionEnabled` and, if so, calls `Disabled()`
  // on the hardware module. It only needs to check the `from` state, since
  // `Disabled()` needs to be called for every transition from `kMotionEnabled`.
  RealtimeStatus CheckAndTriggerDisabledTransitionHook(
      intrinsic_fbs::StateCode from) INTRINSIC_CHECK_REALTIME_SAFE {
    if (from == intrinsic_fbs::StateCode::kMotionEnabled) {
      INTR_RETURN_STATUS_IF_ERROR(instance_->Disabled());
      INTRINSIC_SHARED_MEMORY_LOG(INFO, logger_, "Motion Disabled");
    }
    return RtOkStatus();
  }

  std::string name_;
  HardwareModuleInterface* instance_;
  // action_lock_ synchronizes state change operations (OnEnableMotion,
  // OnDisableMotion, OnClearFaults) to prevent concurrent transitions.
  Mutex action_lock_;
  // Current state of the HWM that should only be used from the RT thread and
  // resides in the shared memory. ICON reads this state.
  intrinsic_fbs::HardwareModuleState* shared_memory_hardware_module_state_;
  // Current state of the HWM that can be used from multiple threads.
  std::atomic<intrinsic_fbs::StateCode> hardware_module_state_code_ =
      intrinsic_fbs::StateCode::kDeactivated;
  // State of the HWM from the previous cycle. `ProcessNextPendingRequest()`
  // updates this variable before it calls `ReadStatus()` on the HWM. Therefore,
  // this variable only updates while the ICON main loop is running.
  std::atomic<intrinsic_fbs::StateCode>
      previous_cycle_hardware_module_state_code_ =
          intrinsic_fbs::StateCode::kDeactivated;
  // Provides the HWM state from rt threads to non-rt threads.
  AsyncBuffer<intrinsic_fbs::HardwareModuleState> hwm_state_buffer_
      INTR_GUARDED_BY(non_rt_buffer_lock_);

  // The thread safe queue of pending requests that. The rt thread will read
  // from the queue in `OnReadStatus()`.
  intrinsic::RealtimeQueue<AsyncRequestType> request_queue_;
  // The thread safe writer for the `request_queue_` that can be written from
  // all non rt-callbacks (onEnable, etc.).
  Mutex non_rt_buffer_lock_;
  intrinsic::RealtimeQueueMultiWriter<AsyncRequestType> request_queue_writer_
      INTR_GUARDED_BY(non_rt_buffer_lock_){*request_queue_.writer()};
  // While this is true, HardwareModuleRuntime rejects any new non-rt requests
  // (e.g. EnableMotion()).
  std::atomic<bool> reject_new_requests_ = false;

  // Timestamp of when the last update of the hwm state was executed. Used to
  // prevent applying updates that are outdated.
  Time hardware_module_state_update_time_ = Now();
  // Container that holds futures that were not ready to be destroyed when the
  // request ended.
  // `SetStateAndWait()` and the destructor will clean up this container if the
  // futures are ready to be destroyed.
  std::list<std::unique_ptr<intrinsic::RealtimeFuture<RealtimeStatus>>>
      future_hospice_ INTR_GUARDED_BY(non_rt_buffer_lock_);

  const log::Logger* logger_ = nullptr;
};

tl::expected<std::unique_ptr<HardwareModuleRuntime>, Status>
HardwareModuleRuntime::Create(
    std::string_view name, std::chrono::nanoseconds control_period,
    std::unique_ptr<SharedMemoryManager> shared_memory_manager,
    std::unique_ptr<HardwareModuleInterface> hardware_module,
    const log::Logger* logger,
    std::weak_ptr<SharedPromiseWrapper<HardwareModuleExitCode>>
        exit_code_promise) {
  if (shared_memory_manager == nullptr) {
    return tl::unexpected(FormatStatus(StatusCode::kInvalidArgument,
                                       "shared_memory_manager cannot be null"));
  }
  if (hardware_module == nullptr) {
    return tl::unexpected(FormatStatus(StatusCode::kInvalidArgument,
                                       "hardware_module cannot be null"));
  }
  if (control_period <= std::chrono::nanoseconds::zero()) {
    return tl::unexpected(
        FormatStatus(StatusCode::kInvalidArgument,
                     "Control period must be positive, got: {:d} ns",
                     control_period.count()));
  }
  // Locks the name used by this module. Ensures only a single instance can
  // run at a time. Fails if the lock can't be acquired within the timeout.
  INTR_ASSIGN_OR_RETURN_UNEXPECTED(
      auto domain_socket_server,
      DomainSocketServer::Create(
          SocketDirectoryFromNamespace(
              shared_memory_manager->SharedMemoryNamespace()),
          shared_memory_manager->ModuleName(),
          DomainSocketServer::kDefaultLockAcquireTimeout, logger));

  // Cannot use `std::make_unique` because this constructor is private.
  auto runtime =
      std::unique_ptr<HardwareModuleRuntime>(new HardwareModuleRuntime(
          std::move(hardware_module), std::move(shared_memory_manager),
          std::move(domain_socket_server), logger));
  INTR_RETURN_UNEXPECTED_IF_ERROR(
      runtime->Connect(name, control_period, exit_code_promise));
  return runtime;
}

HardwareModuleRuntime::HardwareModuleRuntime(
    std::unique_ptr<HardwareModuleInterface> hardware_module,
    std::unique_ptr<SharedMemoryManager> shared_memory_manager,
    std::unique_ptr<DomainSocketServer> domain_socket_server,
    const log::Logger* logger)
    : shared_memory_manager_(std::move(shared_memory_manager)),
      interface_registry_(*shared_memory_manager_),
      hardware_module_(std::move(hardware_module)),
      domain_socket_server_(std::move(domain_socket_server)),
      callback_handler_(nullptr),
      restart_server_(nullptr),
      activate_server_(nullptr),
      deactivate_server_(nullptr),
      prepare_server_(nullptr),
      enable_motion_server_(nullptr),
      disable_motion_server_(nullptr),
      clear_faults_server_(nullptr),
      read_status_server_(nullptr),
      apply_command_server_(nullptr),
      logger_(logger) {}

HardwareModuleRuntime::~HardwareModuleRuntime() {
  if (callback_handler_) {
    callback_handler_->Shutdown();
  }

  if (state_change_thread_.joinable()) {
    INTRINSIC_SHARED_MEMORY_LOG(
        INFO, logger_,
        "Joining state change thread - this could be blocked by frozen "
        "callbacks such as EnableMotion");
    state_change_thread_.request_stop();
    state_change_thread_.join();
  }
}

Status HardwareModuleRuntime::Connect(
    std::string_view name, std::chrono::nanoseconds control_period,
    std::weak_ptr<SharedPromiseWrapper<HardwareModuleExitCode>>
        exit_code_promise) {
  // Adds an "inbuilt" status segment for the hardware module state.
  INTR_ASSIGN_OR_RETURN_STATUS(
      hardware_module_state_interface_,
      interface_registry_
          .AdvertiseMutableInterface<intrinsic_fbs::HardwareModuleState>(
              "hardware_module_state", logger_));

  callback_handler_ = std::make_unique<CallbackHandler>(
      name, hardware_module_.get(), *hardware_module_state_interface_, logger_);

  // Adds an "inbuilt" status segment for ICON to publish its state (e.g.
  // current cycle).
  INTR_ASSIGN_OR_RETURN_STATUS(
      icon_state_interface_,
      interface_registry_.AdvertiseInterface<intrinsic_fbs::IconState>(
          kIconStateInterfaceName, logger_));

  INTR_ASSIGN_OR_RETURN_STATUS(
      control_period_interface_,
      interface_registry_
          .AdvertiseMutableInterface<intrinsic_fbs::ControlPeriod>(
              kControlPeriodInterfaceName, logger_));

  if (control_period <= std::chrono::nanoseconds::zero()) {
    return FormatStatus(StatusCode::kInvalidArgument,
                        "Control period must be positive, got: {:d} ns",
                        control_period.count());
  }
  INTR_RETURN_STATUS_IF_ERROR(
      UpdateControlPeriod(control_period_interface_, control_period, logger_));

  INTR_ASSIGN_OR_RETURN_STATUS(
      auto restart_server,
      RemoteTriggerServer::Create(
          *shared_memory_manager_, "restart", logger_,
          [this,
           // Since `exit_code_promise` is a weak pointer, we can and must copy
           // it into the lambda.
           exit_code_promise] {
            if (auto promise_wrapper = exit_code_promise.lock();
                promise_wrapper != nullptr && !promise_wrapper->HasBeenSet()) {
              // Its safe to use callback_handler_ here because
              // callback_handler_ is destroyed after restart_server_ is
              // destroyed.

              HardwareModuleExitCode exit_code;
              switch (callback_handler_->GetHardwareModuleState().code()) {
                case intrinsic_fbs::StateCode::kMotionEnabled:
                  INTRINSIC_SHARED_MEMORY_LOG(
                      WARNING, logger_,
                      "Restarting the hardware module is not allowed "
                      "while the module is enabled.");
                  return;  // Reject the restart request. Do not set the exit
                           // code promise in this case and return directly.
                           // Since there is no return value, we can only log
                           // the issue.
                case intrinsic_fbs::StateCode::kInitFailed:
                  exit_code = HardwareModuleExitCode::kFatalFaultDuringInit;
                  break;
                case intrinsic_fbs::StateCode::kFatallyFaulted:
                  exit_code = HardwareModuleExitCode::kFatalFaultDuringExec;
                  break;
                // All other cases are handled as a restart request.
                default:
                  exit_code = HardwareModuleExitCode::kRestartRequested;
              }
              INTRINSIC_SHARED_MEMORY_LOG(
                  INFO, logger_,
                  "Restarting hardware module with exit code: {}",
                  static_cast<int>(exit_code));
              // We need to destroy the domain socket server before we set
              // the exit code promise, because we need to prevent new
              // connections to the domain socket server after we set the exit
              // code promise. In practice, this is needed since the ICON
              // server usually restarts faster than the hardware module.
              // Otherwise, ICON connects to the old instance and then loses the
              // connection when the new HWM instance is started.
              domain_socket_server_.reset();
              // Set the exit code promise to indicate that the module
              // should be restarted.
              if (auto status = promise_wrapper->SetValue(exit_code);
                  !status.ok()) {
                INTRINSIC_SHARED_MEMORY_LOG(ERROR, logger_,
                                            "Failed to set exit code: {:s}",
                                            ToString(status));
              }

            } else {
              INTRINSIC_SHARED_MEMORY_LOG(
                  ERROR, logger_,
                  "Exit code promise wrapper is already set/deleted. "
                  "Cannot request to restart the module again.");
            }
          }));
  restart_server_ =
      std::make_unique<RemoteTriggerServer>(std::move(restart_server));

  INTR_ASSIGN_OR_RETURN_STATUS(
      auto activate_server,
      RemoteTriggerServer::Create(*shared_memory_manager_, "activate", logger_,
                                  [this] { callback_handler_->OnActivate(); }));
  activate_server_ =
      std::make_unique<RemoteTriggerServer>(std::move(activate_server));

  INTR_ASSIGN_OR_RETURN_STATUS(
      auto deactivate_server,
      RemoteTriggerServer::Create(
          *shared_memory_manager_, "deactivate", logger_,
          [this] { callback_handler_->OnDeactivate(); }));
  deactivate_server_ =
      std::make_unique<RemoteTriggerServer>(std::move(deactivate_server));

  INTR_ASSIGN_OR_RETURN_STATUS(
      auto prepare_server,
      RemoteTriggerServer::Create(*shared_memory_manager_, "prepare", logger_,
                                  [this] { callback_handler_->OnPrepare(); }));
  prepare_server_ =
      std::make_unique<RemoteTriggerServer>(std::move(prepare_server));

  INTR_ASSIGN_OR_RETURN_STATUS(
      auto enable_motion_server,
      RemoteTriggerServer::Create(
          *shared_memory_manager_, "enable_motion", logger_,
          [this] { callback_handler_->OnEnableMotion(); }));
  enable_motion_server_ =
      std::make_unique<RemoteTriggerServer>(std::move(enable_motion_server));

  INTR_ASSIGN_OR_RETURN_STATUS(
      auto disable_motion_server,
      RemoteTriggerServer::Create(
          *shared_memory_manager_, "disable_motion", logger_,
          [this] { callback_handler_->OnDisableMotion(); }));
  disable_motion_server_ =
      std::make_unique<RemoteTriggerServer>(std::move(disable_motion_server));

  INTR_ASSIGN_OR_RETURN_STATUS(
      auto clear_faults_server,
      RemoteTriggerServer::Create(
          *shared_memory_manager_, "clear_faults", logger_,
          [this] { callback_handler_->OnClearFaults(); }));
  clear_faults_server_ =
      std::make_unique<RemoteTriggerServer>(std::move(clear_faults_server));

  INTR_ASSIGN_OR_RETURN_STATUS(
      auto read_status_server,
      RemoteTriggerServer::Create(
          *shared_memory_manager_, "read_status", logger_,
          [this] { callback_handler_->OnReadStatus(); }));
  read_status_server_ =
      std::make_unique<RemoteTriggerServer>(std::move(read_status_server));

  INTR_ASSIGN_OR_RETURN_STATUS(
      auto apply_command_server,
      RemoteTriggerServer::Create(
          *shared_memory_manager_, "apply_command", logger_,
          [this] { callback_handler_->OnApplyCommand(); }));
  apply_command_server_ =
      std::make_unique<RemoteTriggerServer>(std::move(apply_command_server));

  return OkStatus();
}

Status HardwareModuleRuntime::Run(bool is_realtime,
                                  const std::vector<int>& cpu_affinity) {
  if (has_stopped_) {
    return FormatStatus(StatusCode::kFailedPrecondition,
                        "Cannot restart HardwareModuleRuntime after Stop(), or "
                        "failed attempts at Run()");
  }
  auto stop_on_error = Cleanup([&]() noexcept { std::ignore = Stop(); });
  if (activate_server_ == nullptr) {
    return FormatStatus(
        StatusCode::kInternal,
        "PUBLIC: Hardware module does not seem to be connected. Did you call "
        "`Connect()`?");
  }

  // We need to start the restart-server before the HWM init function is
  // called so that the HWM can be restarted even if it fails to
  // initialize the HWM.
  ThreadOptions restart_thread_options;
  restart_thread_options.SetName("Restart");
  if (is_realtime) {
    restart_thread_options.SetLowRealtimePriorityAndScheduler();
    restart_thread_options.SetAffinity(cpu_affinity);
  }
  INTR_RETURN_STATUS_IF_ERROR(restart_server_->StartAsync(
      logger_, MakeThreadPrelude(restart_thread_options, logger_)));

  // Helper lambda to set the state to `kInitFailed` if any of the
  // initialization steps below fail.
  auto set_init_failed_on_error = [this](Status status) -> Status {
    if (!status.ok()) {
      callback_handler_->SetStateDirectly(
          intrinsic_fbs::StateCode::kInitFailed,
          FormatRealtimeStatus(status.code, "{:s}", status.message));
    }
    return status;
  };

  HardwareModuleInitContext context{
      .interface_registry = interface_registry_,
      .logger = logger_,
  };
  const auto init_status =
      set_init_failed_on_error(hardware_module_->Init(context));
  if (!init_status.ok()) {
    INTRINSIC_SHARED_MEMORY_LOG(ERROR, logger_,
                                "Initializing the module failed with: {:s}",
                                ToString(init_status));
  }

  if (!domain_socket_server_) {
    return FormatStatus(
        StatusCode::kInternal,
        "Run was called, but domain_socket_server_ is nullptr. This should "
        "never happen.");
  }

  // Segments added after this call will not be visible to `DomainSocketServer`
  // and its clients (like ICON).
  INTR_RETURN_STATUS_IF_ERROR(
      domain_socket_server_->AddSegmentInfoServeShmDescriptors(
          *shared_memory_manager_));

  // Ensures that no methods on the uninitialized module can be called.
  INTR_RETURN_STATUS_IF_ERROR(init_status);

  ThreadOptions state_change_thread_options;
  state_change_thread_options.SetName("StateChange");

  ThreadOptions activate_thread_options;
  activate_thread_options.SetName("Activate");

  ThreadOptions read_status_thread_options;
  read_status_thread_options.SetName("ReadStatus");

  ThreadOptions apply_command_thread_options;
  apply_command_thread_options.SetName("ApplyCommand");

  if (is_realtime) {
    state_change_thread_options.SetLowRealtimePriorityAndScheduler();
    state_change_thread_options.SetAffinity(cpu_affinity);
    activate_thread_options.SetLowRealtimePriorityAndScheduler();
    activate_thread_options.SetAffinity(cpu_affinity);
    read_status_thread_options.SetHighRealtimePriorityAndScheduler();
    read_status_thread_options.SetAffinity(cpu_affinity);
    apply_command_thread_options.SetHighRealtimePriorityAndScheduler();
    apply_command_thread_options.SetAffinity(cpu_affinity);
  }

// TODO: Re-enable MallocGuard options once available via BCR.
#if 0
   read_status_thread_options.SetMallocGuarded();
   apply_command_thread_options.SetMallocGuarded();
#endif
  ThreadOptions deactivate_thread_options = activate_thread_options;
  deactivate_thread_options.SetName("Deactivate");

  auto state_change_prelude =
      MakeThreadPrelude(state_change_thread_options, logger_);
  state_change_thread_ =
      std::jthread([this, state_change_prelude](std::stop_token stop_token) {
        if (auto status = state_change_prelude(); !status.ok()) {
          INTRINSIC_SHARED_MEMORY_LOG(ERROR, logger_,
                                      "StateChange thread prelude failed: {:s}",
                                      ToString(status));
        }
        while (!stop_token.stop_requested()) {
          std::ignore = prepare_server_->Query(logger_);
          std::ignore = enable_motion_server_->Query(logger_);
          std::ignore = disable_motion_server_->Query(logger_);
          std::ignore = clear_faults_server_->Query(logger_);
          std::this_thread::yield();
        }
      });

  INTR_RETURN_STATUS_IF_ERROR(
      set_init_failed_on_error(activate_server_->StartAsync(
          logger_, MakeThreadPrelude(activate_thread_options, logger_))));
  INTR_RETURN_STATUS_IF_ERROR(
      set_init_failed_on_error(deactivate_server_->StartAsync(
          logger_, MakeThreadPrelude(deactivate_thread_options, logger_))));
  INTR_RETURN_STATUS_IF_ERROR(
      set_init_failed_on_error(read_status_server_->StartAsync(
          logger_, MakeThreadPrelude(read_status_thread_options, logger_))));
  INTR_RETURN_STATUS_IF_ERROR(
      set_init_failed_on_error(apply_command_server_->StartAsync(
          logger_, MakeThreadPrelude(apply_command_thread_options, logger_))));

  std::move(stop_on_error).Cancel();
  return OkStatus();
}

Status HardwareModuleRuntime::Stop() {
  INTRINSIC_SHARED_MEMORY_LOG(INFO, logger_,
                              "Stopping hardware module runtime.");
  callback_handler_->Shutdown();
  apply_command_server_->RequestStop();
  read_status_server_->RequestStop();
  deactivate_server_->RequestStop();
  enable_motion_server_->RequestStop();
  clear_faults_server_->RequestStop();
  prepare_server_->RequestStop();
  activate_server_->RequestStop();
  restart_server_->RequestStop();
  auto status = hardware_module_->Shutdown();
  apply_command_server_->JoinAsyncThread();
  read_status_server_->JoinAsyncThread();
  deactivate_server_->JoinAsyncThread();
  activate_server_->JoinAsyncThread();
  restart_server_->JoinAsyncThread();
  if (state_change_thread_.joinable()) {
    state_change_thread_.request_stop();
    state_change_thread_.join();
  }
  has_stopped_ = true;
  return status;
}

bool HardwareModuleRuntime::IsStarted() const {
  bool started = state_change_thread_.joinable();
  if (read_status_server_) {
    started &= read_status_server_->IsStarted();
  }
  if (apply_command_server_) {
    started &= apply_command_server_->IsStarted();
  }
  return started;
}

const HardwareModuleInterface& HardwareModuleRuntime::GetHardwareModule()
    const {
  return *hardware_module_;
}

HardwareModuleInterface& HardwareModuleRuntime::GetHardwareModule() {
  return *hardware_module_;
}

tl::expected<intrinsic_fbs::HardwareModuleState, Status>
HardwareModuleRuntime::GetHardwareModuleState() const {
  if (callback_handler_ == nullptr) {
    return tl::unexpected(
        FormatStatus(StatusCode::kInternal,
                     "Hardware Module Runtime callback_handler is null"));
  }
  return callback_handler_->GetHardwareModuleState();
}

void HardwareModuleRuntime::SetStateTestOnly(intrinsic_fbs::StateCode state,
                                             RealtimeStatus status) {
  callback_handler_->SetStateDirectly(state, status, /*force=*/true);
}

void HardwareModuleRuntime::SetStateTestOnly(intrinsic_fbs::StateCode state,
                                             std::string_view fault_reason) {
  SetStateTestOnly(state, fault_reason.empty()
                              ? RtOkStatus()
                              : FormatRealtimeStatus(StatusCode::kUnknown,
                                                     "{:s}", fault_reason));
}

}  // namespace intrinsic::icon
