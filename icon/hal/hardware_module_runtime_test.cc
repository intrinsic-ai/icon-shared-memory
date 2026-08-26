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

#include <chrono>
#include <future>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_set>
#include <utility>
#include <vector>

#include "flatbuffers/buffer.h"
#include "gmock/gmock.h"
#include "flatbuffer_definitions/icon/hal/interfaces/hardware_module_state.fbs.h"
#include "flatbuffer_definitions/icon/hal/interfaces/icon_state.fbs.h"
#include "flatbuffer_definitions/icon/hal/interfaces/joint_command.fbs.h"
#include "flatbuffer_definitions/icon/hal/interfaces/joint_limits.fbs.h"
#include "flatbuffer_definitions/icon/hal/interfaces/joint_state.fbs.h"
#include "gtest/gtest.h"
#include "icon/hal/control_period_register.h"
#include "icon/hal/hardware_interface_handle.h"
#include "icon/hal/hardware_interface_registry.h"
#include "icon/hal/hardware_interface_traits.h"
#include "icon/hal/hardware_module_interface.h"
#include "icon/hal/icon_state_register.h"
#include "icon/hal/interfaces/hardware_module_state_utils.h"
#include "icon/hal/interfaces/joint_command_utils.h"
#include "icon/hal/interfaces/joint_limits_utils.h"
#include "icon/hal/interfaces/joint_state_utils.h"
#include "icon/interprocess/binary_futex.h"
#include "icon/interprocess/remote_trigger/remote_trigger_constants.h"
#include "icon/interprocess/shared_memory_manager/domain_socket_utils.h"
#include "icon/interprocess/shared_memory_manager/memory_segment.h"
#include "icon/interprocess/shared_memory_manager/shared_memory_manager.h"
#include "icon/interprocess/shared_memory_manager/testing/unique_segment_name.h"
#include "icon/utils/log.h"
#include "icon/utils/mock_log_sink.h"
#include "icon/utils/mutex.h"
#include "icon/utils/status.h"
#include "icon/utils/status_and_expected_macros.h"
#include "icon/utils/status_and_expected_test_macros.h"
#include "icon/utils/status_matchers.h"
#include "tl/expected.hpp"

namespace intrinsic::icon {

namespace hardware_interface_traits {

INTRINSIC_ADD_HARDWARE_INTERFACE(intrinsic_fbs::JointPositionCommand,
                                 intrinsic_fbs::BuildJointPositionCommand,
                                 "intrinsic_fbs.JointPositionCommand")

INTRINSIC_ADD_HARDWARE_INTERFACE(intrinsic_fbs::JointPositionState,
                                 intrinsic_fbs::BuildJointPositionState,
                                 "intrinsic_fbs.JointPositionState")

INTRINSIC_ADD_HARDWARE_INTERFACE(intrinsic_fbs::JointLimits,
                                 intrinsic_fbs::BuildJointLimits,
                                 "intrinsic_fbs.JointLimits")

}  // namespace hardware_interface_traits

namespace {

using ::testing::AllOf;
using ::testing::HasSubstr;
using ::testing::Property;

constexpr std::string_view kModuleName = "my_test_hardware_module";
constexpr auto kShortSleepDuration = std::chrono::milliseconds(10);
constexpr auto kControlPeriod = std::chrono::milliseconds(1);
constexpr auto kLongRunningOpTimeout = std::chrono::seconds(5);

// Mock implementation of `HardwareModuleInterface` used for testing
// `HardwareModuleRuntime`.
class TestHardwareModule : public HardwareModuleInterface {
 public:
  enum class State : int8_t {
    kDeactivated = 0,
    kActivated = 1,
    kMotionEnabled = 2,
    kFaulted = 3,
  };

  // Types of transition requests for transition callbacks.
  enum class TransitionRequest {
    kPrepare,
    kActivate,
    kDeactivate,
    kEnableMotion,
    kDisableMotion,
    kClearFaults,
  };

  TestHardwareModule() = default;

  // Initializes the module. Returns `init_status_override_` if set.
  Status Init(HardwareModuleInitContext& context) override {
    if (init_status_override_.has_value()) {
      return *init_status_override_;
    }
    INTR_ASSIGN_OR_RETURN_STATUS(
        joint_position_command_,
        context.interface_registry
            .AdvertiseMutableInterface<intrinsic_fbs::JointPositionCommand>(
                "joint_position_command", context.logger, 1));
    INTR_ASSIGN_OR_RETURN_STATUS(
        joint_position_state_,
        context.interface_registry
            .AdvertiseMutableInterface<intrinsic_fbs::JointPositionState>(
                "joint_position_state", context.logger, 1));
    INTR_ASSIGN_OR_RETURN_STATUS(
        joint_limits_,
        context.interface_registry
            .AdvertiseMutableInterface<intrinsic_fbs::JointLimits>(
                "joint_limits", context.logger, 1));
    return OkStatus();
  }

  Status Prepare() override {
    if (transition_callback_) {
      INTR_RETURN_STATUS_IF_ERROR(
          transition_callback_(TransitionRequest::kPrepare));
    }
    return OkStatus();
  }

  RealtimeStatus Activate() override {
    if (transition_callback_) {
      const auto status = transition_callback_(TransitionRequest::kActivate);
      if (!status.ok()) {
        return FormatRealtimeStatus(status.code, "{:s}", status.message);
      }
    }
    current_state_ = State::kActivated;
    return RtOkStatus();
  }

  RealtimeStatus Deactivate() override {
    if (transition_callback_) {
      const auto status = transition_callback_(TransitionRequest::kDeactivate);
      if (!status.ok()) {
        return FormatRealtimeStatus(status.code, "{:s}", status.message);
      }
    }
    current_state_ = State::kDeactivated;
    return RtOkStatus();
  }

  Status EnableMotion() override {
    if (transition_callback_) {
      INTR_RETURN_STATUS_IF_ERROR(
          transition_callback_(TransitionRequest::kEnableMotion));
    }
    while (pause_operational_state_transitions_.load()) {
      std::this_thread::sleep_for(kShortSleepDuration);
    }
    if (failing_transitions_.has_value()) {
      return FormatStatus(*failing_transitions_,
                          "Failing EnableMotion transition");
    }
    return OkStatus();
  }

  // `HardwareModuleRuntime` calls this when enabling is done.
  // `HardwareModuleRuntime` only calls `ApplyCommand()` between the moment that
  // it calls `Enabled()` and the moment it calls `Disabled()`.
  RealtimeStatus Enabled() override {
    if (enabled_callback_) {
      const auto s = enabled_callback_(true);
      if (!s.ok()) {
        return FormatRealtimeStatus(s.code, "{:s}", s.message);
      }
    }
    current_state_ = State::kMotionEnabled;
    return RtOkStatus();
  }

  // `HardwareModuleRuntime` calls this when disabling.
  RealtimeStatus Disabled() override {
    if (enabled_callback_) {
      const auto s = enabled_callback_(false);
      if (!s.ok()) {
        return FormatRealtimeStatus(s.code, "{:s}", s.message);
      }
    }
    current_state_ = State::kActivated;
    return RtOkStatus();
  }

  Status DisableMotion() override {
    if (transition_callback_) {
      INTR_RETURN_STATUS_IF_ERROR(
          transition_callback_(TransitionRequest::kDisableMotion));
    }
    while (pause_operational_state_transitions_.load()) {
      std::this_thread::sleep_for(kShortSleepDuration);
    }
    if (failing_transitions_.has_value()) {
      return FormatStatus(*failing_transitions_,
                          "Failing DisableMotion transition");
    }
    return OkStatus();
  }

  Status ClearFaults() override {
    if (transition_callback_) {
      INTR_RETURN_STATUS_IF_ERROR(
          transition_callback_(TransitionRequest::kClearFaults));
    }
    while (pause_operational_state_transitions_.load()) {
      std::this_thread::sleep_for(kShortSleepDuration);
    }
    if (failing_transitions_.has_value()) {
      return FormatStatus(*failing_transitions_,
                          "Failing ClearFaults transition");
    }
    {
      MutexLock lock(&fault_reason_mutex_);
      fault_reason_.clear();
    }
    current_state_ = State::kActivated;
    return OkStatus();
  }

  Status Shutdown() override {
    pause_operational_state_transitions_ = false;
    return OkStatus();
  }

  // Reads status from hardware.
  // In this mock hardware module, we copy commands to the state while enabled.
  //
  // Because this test does not run in an actual realtime environment, we can
  // make copies of `fault_reason_`, for example.
  RealtimeStatus ReadStatus() override {
    if (fault_reason_mutex_.TryLock()) {
      const std::string reason = fault_reason_;
      fault_reason_mutex_.Unlock();
      if (!reason.empty()) {
        return FormatRealtimeStatus(StatusCode::kInternal, "{:s}", reason);
      }
    }
    if (joint_position_state_.has_value() &&
        joint_position_command_.has_value()) {
      if (current_state_ == State::kMotionEnabled) {
        auto* cmd = **joint_position_command_;
        auto* state = **joint_position_state_;
        if (cmd && state && cmd->position() && state->mutable_position()) {
          state->mutable_position()->Mutate(0, cmd->position()->Get(0));
        }
      }
    }
    return RtOkStatus();
  }

  // Applies control command to hardware. This is a no-op for this mock.
  //
  // Because this test does not run in an actual realtime environment, we can
  // make copies of `fault_reason_`, for example.
  RealtimeStatus ApplyCommand() override {
    if (fault_reason_mutex_.TryLock()) {
      const std::string reason = fault_reason_;
      fault_reason_mutex_.Unlock();
      if (!reason.empty()) {
        return FormatRealtimeStatus(StatusCode::kInternal, "{:s}", reason);
      }
    }
    return RtOkStatus();
  }

  // Overrides the return value of `Init()` for testing error propagation.
  void SetInitStatusOverride(Status status) {
    init_status_override_ = std::move(status);
  }

  // Sets an artificial fault reason to be returned by `ReadStatus()` /
  // `ApplyCommand()`.
  void SetFaultReason(std::string_view reason) {
    MutexLock lock(&fault_reason_mutex_);
    fault_reason_ = reason;
  }

  // Clears the fault reason, so subsequent calls to `ReadStatus()` /
  // `ApplyCommand()` will succeed.
  void ClearFaultReason() {
    MutexLock lock(&fault_reason_mutex_);
    fault_reason_ = "";
  }

  // Pauses asynchronous state transitions until unpaused.
  void PauseOperationalStateTransitions(bool pause = true) {
    pause_operational_state_transitions_ = pause;
  }

  // Configures all transitions to return this error status code.
  void SetFailingTransitions(StatusCode code) { failing_transitions_ = code; }
  // Clears the error status code so that subsequent transitions succeed.
  void ClearFailingTransitions() { failing_transitions_ = std::nullopt; }

  // Injects a custom callback invoked during transitions.
  void SetTransitionCallback(std::function<Status(TransitionRequest)> cb) {
    transition_callback_ = std::move(cb);
  }

  // Injects a custom callback invoked during Enabled() / Disabled() hooks.
  void SetEnabledCallback(std::function<Status(bool)> cb) {
    enabled_callback_ = std::move(cb);
  }

  // Returns the current mock state of the hardware module.
  State GetCurrentState() const { return current_state_; }

 private:
  std::optional<Status> init_status_override_;
  mutable Mutex fault_reason_mutex_;
  std::string fault_reason_ INTR_GUARDED_BY(fault_reason_mutex_);
  std::atomic<bool> pause_operational_state_transitions_{false};
  std::optional<StatusCode> failing_transitions_;
  std::function<Status(TransitionRequest)> transition_callback_;
  std::function<Status(bool)> enabled_callback_;
  std::atomic<State> current_state_{State::kDeactivated};

  std::optional<
      MutableHardwareInterfaceHandle<intrinsic_fbs::JointPositionCommand>>
      joint_position_command_;
  std::optional<
      MutableHardwareInterfaceHandle<intrinsic_fbs::JointPositionState>>
      joint_position_state_;
  std::optional<MutableHardwareInterfaceHandle<intrinsic_fbs::JointLimits>>
      joint_limits_;
};

// Client helper for triggering remote trigger servers via shared memory
// futexes.
class RemoteTriggerClient {
 public:
  RemoteTriggerClient() = default;

  static tl::expected<RemoteTriggerClient, Status> Create(
      const SegmentNameToFileDescriptorMap& segment_map,
      std::string_view server_name, const log::Logger* logger) {
    const std::string req_name = std::string(server_name) + kSemRequestSuffix;
    const std::string res_name = std::string(server_name) + kSemResponseSuffix;

    INTR_ASSIGN_OR_RETURN_UNEXPECTED(
        auto req, (ReadWriteMemorySegment<BinaryFutex>::Get(segment_map,
                                                            req_name, logger)));
    INTR_ASSIGN_OR_RETURN_UNEXPECTED(
        auto res, (ReadOnlyMemorySegment<BinaryFutex>::Get(segment_map,
                                                           res_name, logger)));

    return RemoteTriggerClient(std::move(req), std::move(res));
  }

  Status Trigger(std::chrono::milliseconds timeout = kLongRunningOpTimeout) {
    if (const auto s = request_futex_.GetValue().Post(); !s.ok()) {
      return ToStatus(s);
    }
    if (const auto s = response_futex_.GetValue().WaitFor(timeout); !s.ok()) {
      return ToStatus(s);
    }
    return OkStatus();
  }

 private:
  RemoteTriggerClient(ReadWriteMemorySegment<BinaryFutex>&& req,
                      ReadOnlyMemorySegment<BinaryFutex>&& res)
      : request_futex_(std::move(req)), response_futex_(std::move(res)) {}

  ReadWriteMemorySegment<BinaryFutex> request_futex_;
  ReadOnlyMemorySegment<BinaryFutex> response_futex_;
};

// Test fixture that
// * starts a HardwareModuleRuntime instance
// * sets up IPC trigger clients and shared memory handles for interacting with
//   it
// * provides helper functions to conveniently make synchronous and asynchronous
//   calls to the RemoteTriggerServers
class HardwareModuleRuntimeIpcFixture : public ::testing::Test {
 public:
  void SetUp() override {
    logger_ = std::make_unique<log::Logger>(log::Logger::Severity::kDebug,
                                            mock_log_sink_);

    INTR_ASSERT_OK_AND_ASSIGN(
        auto shm_manager, SharedMemoryManager::Create(
                              memory_namespace_, kModuleName, logger_.get()));

    auto test_module = std::make_unique<TestHardwareModule>();
    test_hardware_module_ = test_module.get();

    exit_code_promise_ =
        std::make_shared<SharedPromiseWrapper<HardwareModuleExitCode>>();

    INTR_ASSERT_OK_AND_ASSIGN(
        runtime_,
        HardwareModuleRuntime::Create(
            kModuleName, kControlPeriod, std::move(shm_manager),
            std::move(test_module), logger_.get(), exit_code_promise_));

    INTR_ASSERT_OK(runtime_->Run());

    INTR_ASSERT_OK_AND_ASSIGN(
        const auto segment_name_to_fd_map,
        GetSegmentNameToFileDescriptorMap(
            SocketDirectoryFromNamespace(memory_namespace_), kModuleName,
            kLongRunningOpTimeout, logger_.get()));

    INTR_ASSERT_OK_AND_ASSIGN(
        prepare_client_, RemoteTriggerClient::Create(segment_name_to_fd_map,
                                                     "prepare", logger_.get()));
    INTR_ASSERT_OK_AND_ASSIGN(activate_client_, RemoteTriggerClient::Create(
                                                    segment_name_to_fd_map,
                                                    "activate", logger_.get()));
    INTR_ASSERT_OK_AND_ASSIGN(
        deactivate_client_,
        RemoteTriggerClient::Create(segment_name_to_fd_map, "deactivate",
                                    logger_.get()));
    INTR_ASSERT_OK_AND_ASSIGN(
        enable_motion_client_,
        RemoteTriggerClient::Create(segment_name_to_fd_map, "enable_motion",
                                    logger_.get()));
    INTR_ASSERT_OK_AND_ASSIGN(
        disable_motion_client_,
        RemoteTriggerClient::Create(segment_name_to_fd_map, "disable_motion",
                                    logger_.get()));
    INTR_ASSERT_OK_AND_ASSIGN(
        clear_faults_client_,
        RemoteTriggerClient::Create(segment_name_to_fd_map, "clear_faults",
                                    logger_.get()));
    INTR_ASSERT_OK_AND_ASSIGN(
        read_status_client_,
        RemoteTriggerClient::Create(segment_name_to_fd_map, "read_status",
                                    logger_.get()));
    INTR_ASSERT_OK_AND_ASSIGN(
        apply_command_client_,
        RemoteTriggerClient::Create(segment_name_to_fd_map, "apply_command",
                                    logger_.get()));
    INTR_ASSERT_OK_AND_ASSIGN(
        restart_client_, RemoteTriggerClient::Create(segment_name_to_fd_map,
                                                     "restart", logger_.get()));

    INTR_ASSERT_OK_AND_ASSIGN(
        shm_joint_position_command_,
        (ReadWriteMemorySegment<intrinsic_fbs::JointPositionCommand>::Get(
            segment_name_to_fd_map, "joint_position_command", logger_.get())));
    joint_position_command_ =
        flatbuffers::GetMutableRoot<intrinsic_fbs::JointPositionCommand>(
            shm_joint_position_command_.GetRawValue());
    ASSERT_NE(joint_position_command_, nullptr);
    EXPECT_DOUBLE_EQ(joint_position_command_->position()->Get(0), 0.0);

    INTR_ASSERT_OK_AND_ASSIGN(
        shm_joint_position_state_,
        (ReadWriteMemorySegment<intrinsic_fbs::JointPositionState>::Get(
            segment_name_to_fd_map, "joint_position_state", logger_.get())));
    joint_position_state_ =
        flatbuffers::GetRoot<intrinsic_fbs::JointPositionState>(
            shm_joint_position_state_.GetRawValue());
    ASSERT_NE(joint_position_state_, nullptr);
    EXPECT_DOUBLE_EQ(joint_position_state_->position()->Get(0), 0.0);

    INTR_ASSERT_OK_AND_ASSIGN(
        shm_hardware_module_state_,
        (ReadWriteMemorySegment<intrinsic_fbs::HardwareModuleState>::Get(
            segment_name_to_fd_map, "hardware_module_state", logger_.get())));
    hardware_module_state_ =
        flatbuffers::GetRoot<intrinsic_fbs::HardwareModuleState>(
            shm_hardware_module_state_.GetRawValue());
    ASSERT_NE(hardware_module_state_, nullptr);
    EXPECT_EQ(hardware_module_state_->code(),
              intrinsic_fbs::StateCode::kDeactivated);
    mutable_hardware_module_state_ =
        flatbuffers::GetMutableRoot<intrinsic_fbs::HardwareModuleState>(
            shm_hardware_module_state_.GetRawValue());
    ASSERT_NE(mutable_hardware_module_state_, nullptr);
  }

  // Makes a blocking call to the synchronous Prepare IPC server.
  Status Prepare() { return prepare_client_.Trigger(); }

  // Makes a blocking call to the synchronous Activate IPC server.
  Status Activate() { return activate_client_.Trigger(); }

  // Makes a blocking call to the synchronous Deactivate IPC server.
  Status Deactivate() { return deactivate_client_.Trigger(); }

  std::future<Status> AsyncPrepare() {
    return std::async(std::launch::async,
                      [this] { return prepare_client_.Trigger(); });
  }

  std::future<Status> AsyncActivate() {
    return std::async(std::launch::async,
                      [this] { return activate_client_.Trigger(); });
  }

  std::future<Status> AsyncDeactivate() {
    return std::async(std::launch::async,
                      [this] { return deactivate_client_.Trigger(); });
  }

  std::future<Status> AsyncEnableMotion() {
    return std::async(std::launch::async,
                      [this] { return enable_motion_client_.Trigger(); });
  }

  std::future<Status> AsyncDisableMotion() {
    return std::async(std::launch::async,
                      [this] { return disable_motion_client_.Trigger(); });
  }

  std::future<Status> AsyncClearFaults() {
    return std::async(std::launch::async,
                      [this] { return clear_faults_client_.Trigger(); });
  }

  // Requests `EnableMotion` and calls `ReadStatus()` until the `RemoteTrigger`
  // call finishes.
  Status EnableMotion() {
    auto future = AsyncEnableMotion();
    while (future.wait_for(kControlPeriod) != std::future_status::ready) {
      INTR_RETURN_STATUS_IF_ERROR(ReadStatus());
    }
    INTR_RETURN_STATUS_IF_ERROR(ReadStatus());
    return future.get();
  }

  // Requests `DisableMotion` and calls `ReadStatus()` until the `RemoteTrigger`
  // call finishes.
  Status DisableMotion() {
    auto future = AsyncDisableMotion();
    while (future.wait_for(kControlPeriod) != std::future_status::ready) {
      INTR_RETURN_STATUS_IF_ERROR(ReadStatus());
    }
    return future.get();
  }

  // Requests `ClearFault` and calls `ReadStatus()` until the `RemoteTrigger`
  // call finishes.
  Status ClearFaults() {
    auto future = AsyncClearFaults();
    while (future.wait_for(kControlPeriod) != std::future_status::ready) {
      INTR_RETURN_STATUS_IF_ERROR(ReadStatus());
    }
    return future.get();
  }

  // Transitions the module to `kMotionEnabled` via `Prepare()`, `Activate()`,
  // and `EnableMotion()`.
  Status PrepareActivateAndEnableMotion() {
    INTR_RETURN_STATUS_IF_ERROR(Prepare());
    INTR_RETURN_STATUS_IF_ERROR(Activate());
    return EnableMotion();
  }

  // Simulates a single realtime cycle by calling `ReadStatus()` and
  // `ApplyCommand()`.
  Status Tick() {
    INTR_RETURN_STATUS_IF_ERROR(ReadStatus());
    return ApplyCommand();
  }

  // Triggers the `ReadStatus` IPC server.
  Status ReadStatus() { return read_status_client_.Trigger(); }

  // Triggers the `ApplyCommand` IPC server.
  Status ApplyCommand() { return apply_command_client_.Trigger(); }

  // Triggers the `Restart` IPC server.
  Status Restart() { return restart_client_.Trigger(); }

  // Returns a reference to the underlying test hardware module.
  TestHardwareModule& GetTestHardwareModule() { return *test_hardware_module_; }

  ~HardwareModuleRuntimeIpcFixture() override {
    if (runtime_) {
      (void)runtime_->Stop();
    }
  }

  const std::string memory_namespace_ = UniqueMemoryNamespace();
  log::MockLogSink mock_log_sink_;
  std::unique_ptr<log::Logger> logger_;
  std::unique_ptr<HardwareModuleRuntime> runtime_;
  TestHardwareModule* test_hardware_module_ = nullptr;
  std::shared_ptr<SharedPromiseWrapper<HardwareModuleExitCode>>
      exit_code_promise_;
  RemoteTriggerClient prepare_client_{};
  RemoteTriggerClient activate_client_{};
  RemoteTriggerClient deactivate_client_{};
  RemoteTriggerClient enable_motion_client_{};
  RemoteTriggerClient disable_motion_client_{};
  RemoteTriggerClient clear_faults_client_{};
  RemoteTriggerClient read_status_client_{};
  RemoteTriggerClient apply_command_client_{};
  RemoteTriggerClient restart_client_{};

  ReadWriteMemorySegment<intrinsic_fbs::JointPositionCommand>
      shm_joint_position_command_;
  ReadWriteMemorySegment<intrinsic_fbs::JointPositionState>
      shm_joint_position_state_;
  ReadWriteMemorySegment<intrinsic_fbs::HardwareModuleState>
      shm_hardware_module_state_;

  intrinsic_fbs::JointPositionCommand* joint_position_command_ = nullptr;
  const intrinsic_fbs::JointPositionState* joint_position_state_ = nullptr;
  const intrinsic_fbs::HardwareModuleState* hardware_module_state_ = nullptr;
  intrinsic_fbs::HardwareModuleState* mutable_hardware_module_state_ = nullptr;
};

std::unordered_set<intrinsic_fbs::StateCode> AllStates() {
  const auto& enum_values = intrinsic_fbs::EnumValuesStateCode();
  return std::unordered_set<intrinsic_fbs::StateCode>(std::begin(enum_values),
                                                      std::end(enum_values));
}

std::unordered_set<intrinsic_fbs::StateCode> NoopStates(
    const std::unordered_set<intrinsic_fbs::StateCode>& op_states) {
  auto noop_states = AllStates();
  for (auto k : op_states) {
    noop_states.erase(k);
  }
  return noop_states;
}

std::unordered_set<intrinsic_fbs::StateCode> OpStates(
    const std::unordered_set<intrinsic_fbs::StateCode>& noop_states) {
  auto op_states = AllStates();
  for (auto k : noop_states) {
    op_states.erase(k);
  }
  return op_states;
}

TEST(HardwareModuleRuntimeTest,
     HardwareModuleRuntimeRejectsSecondRunAttemptAfterFirstFailure) {
  log::MockLogSink mock_log_sink;
  const auto logger = std::make_unique<log::Logger>(
      log::Logger::Severity::kDebug, mock_log_sink);
  std::string memory_namespace = UniqueMemoryNamespace();
  INTR_ASSERT_OK_AND_ASSIGN(
      auto shm_manager,
      SharedMemoryManager::Create(memory_namespace, kModuleName, logger.get()));

  auto test_module = std::make_unique<TestHardwareModule>();
  auto* module = test_module.get();

  const Status injected_failure{.code = StatusCode::kInternal,
                                .message = "init failure"};
  module->SetInitStatusOverride(injected_failure);

  INTR_ASSERT_OK_AND_ASSIGN(
      auto runtime, HardwareModuleRuntime::Create(
                        kModuleName, kControlPeriod, std::move(shm_manager),
                        std::move(test_module), logger.get()));

  EXPECT_THAT(runtime->Run(), StatusIs(injected_failure.code,
                                       HasSubstr(injected_failure.message)));

  EXPECT_THAT(runtime->Run(), StatusIs(StatusCode::kFailedPrecondition,
                                       HasSubstr("Cannot restart")));
}

TEST(HardwareModuleRuntimeTest, HardwareModuleRuntimeExposesInitFailure) {
  log::MockLogSink mock_log_sink;
  const auto logger = std::make_unique<log::Logger>(
      log::Logger::Severity::kDebug, mock_log_sink);
  std::string memory_namespace = UniqueMemoryNamespace();
  INTR_ASSERT_OK_AND_ASSIGN(
      auto shm_manager,
      SharedMemoryManager::Create(memory_namespace, kModuleName, logger.get()));

  auto test_module = std::make_unique<TestHardwareModule>();
  auto* module = test_module.get();

  const Status injected_failure{.code = StatusCode::kInternal,
                                .message = "init failure"};
  module->SetInitStatusOverride(injected_failure);

  INTR_ASSERT_OK_AND_ASSIGN(
      auto runtime, HardwareModuleRuntime::Create(
                        kModuleName, kControlPeriod, std::move(shm_manager),
                        std::move(test_module), logger.get()));

  EXPECT_THAT(runtime->Run(), StatusIs(injected_failure.code,
                                       HasSubstr(injected_failure.message)));

  INTR_ASSERT_OK_AND_ASSIGN(const auto state,
                            runtime->GetHardwareModuleState());
  EXPECT_EQ(state.code(), intrinsic_fbs::StateCode::kInitFailed);
  EXPECT_EQ(injected_failure.message, intrinsic_fbs::GetMessage(&state));

  INTR_ASSERT_OK_AND_ASSIGN(
      const auto segment_name_to_fd_map,
      GetSegmentNameToFileDescriptorMap(
          SocketDirectoryFromNamespace(memory_namespace), kModuleName,
          kLongRunningOpTimeout, logger.get()));

  INTR_ASSERT_OK_AND_ASSIGN(
      auto shm_hardware_module_state,
      (ReadWriteMemorySegment<intrinsic_fbs::HardwareModuleState>::Get(
          segment_name_to_fd_map, "hardware_module_state", logger.get())));
  const auto* ipc_state =
      flatbuffers::GetRoot<intrinsic_fbs::HardwareModuleState>(
          shm_hardware_module_state.GetRawValue());
  ASSERT_NE(ipc_state, nullptr);
  EXPECT_EQ(ipc_state->code(), intrinsic_fbs::StateCode::kInitFailed);
  EXPECT_EQ(injected_failure.message, intrinsic_fbs::GetMessage(ipc_state));
}

TEST(HardwareModuleRuntimeTest,
     HardwareModuleRuntimeFailsOnNullSharedMemoryManager) {
  log::MockLogSink mock_log_sink;
  const auto logger = std::make_unique<log::Logger>(
      log::Logger::Severity::kDebug, mock_log_sink);

  const auto runtime_or = HardwareModuleRuntime::Create(
      kModuleName, kControlPeriod,
      /*shared_memory_manager=*/nullptr, std::make_unique<TestHardwareModule>(),
      logger.get());

  EXPECT_THAT(runtime_or,
              StatusIs(StatusCode::kInvalidArgument,
                       HasSubstr("shared_memory_manager cannot be null")));
}

TEST(HardwareModuleRuntimeTest,
     HardwareModuleRuntimeFailsOnNullHardwareModule) {
  log::MockLogSink mock_log_sink;
  const auto logger = std::make_unique<log::Logger>(
      log::Logger::Severity::kDebug, mock_log_sink);
  std::string memory_namespace = UniqueMemoryNamespace();
  INTR_ASSERT_OK_AND_ASSIGN(
      auto shm_manager,
      SharedMemoryManager::Create(memory_namespace, kModuleName, logger.get()));

  const auto runtime_or = HardwareModuleRuntime::Create(
      kModuleName, kControlPeriod, std::move(shm_manager),
      /*hardware_module=*/nullptr, logger.get());

  EXPECT_THAT(runtime_or,
              StatusIs(StatusCode::kInvalidArgument,
                       HasSubstr("hardware_module cannot be null")));
}

TEST(HardwareModuleRuntimeTest,
     HardwareModuleRuntimeFailsOnNegativeControlPeriod) {
  log::MockLogSink mock_log_sink;
  const auto logger = std::make_unique<log::Logger>(
      log::Logger::Severity::kDebug, mock_log_sink);
  std::string memory_namespace = UniqueMemoryNamespace();
  INTR_ASSERT_OK_AND_ASSIGN(
      auto shm_manager,
      SharedMemoryManager::Create(memory_namespace, kModuleName, logger.get()));

  const auto runtime_or = HardwareModuleRuntime::Create(
      kModuleName, std::chrono::milliseconds(-1), std::move(shm_manager),
      std::make_unique<TestHardwareModule>(), logger.get());

  EXPECT_THAT(runtime_or,
              StatusIs(StatusCode::kInvalidArgument,
                       HasSubstr("Control period must be positive")));
}

TEST(HardwareModuleRuntimeTest, HardwareModuleRuntimeRunsSuccessfully) {
  log::MockLogSink mock_log_sink;
  const auto logger = std::make_unique<log::Logger>(
      log::Logger::Severity::kDebug, mock_log_sink);
  std::string memory_namespace = UniqueMemoryNamespace();
  INTR_ASSERT_OK_AND_ASSIGN(
      auto shm_manager,
      SharedMemoryManager::Create(memory_namespace, kModuleName, logger.get()));
  INTR_ASSERT_OK_AND_ASSIGN(
      auto runtime, HardwareModuleRuntime::Create(
                        kModuleName, kControlPeriod, std::move(shm_manager),
                        std::make_unique<TestHardwareModule>(), logger.get()));
  INTR_EXPECT_OK(runtime->Run());
}

TEST(HardwareModuleRuntimeTest, IsStartedReportsCorrectLifecycleState) {
  log::MockLogSink mock_log_sink;
  const auto logger = std::make_unique<log::Logger>(
      log::Logger::Severity::kDebug, mock_log_sink);
  std::string memory_namespace = UniqueMemoryNamespace();
  INTR_ASSERT_OK_AND_ASSIGN(
      auto shm_manager,
      SharedMemoryManager::Create(memory_namespace, kModuleName, logger.get()));

  INTR_ASSERT_OK_AND_ASSIGN(
      auto runtime, HardwareModuleRuntime::Create(
                        kModuleName, kControlPeriod, std::move(shm_manager),
                        std::make_unique<TestHardwareModule>(), logger.get()));

  EXPECT_FALSE(runtime->IsStarted());
  INTR_ASSERT_OK(runtime->Run());
  EXPECT_TRUE(runtime->IsStarted());
  INTR_ASSERT_OK(runtime->Stop());
  EXPECT_FALSE(runtime->IsStarted());
}

TEST(HardwareModuleRuntimeTest, GetHardwareModuleReturnsModuleReference) {
  log::MockLogSink mock_log_sink;
  const auto logger = std::make_unique<log::Logger>(
      log::Logger::Severity::kDebug, mock_log_sink);
  std::string memory_namespace = UniqueMemoryNamespace();
  INTR_ASSERT_OK_AND_ASSIGN(
      auto shm_manager,
      SharedMemoryManager::Create(memory_namespace, kModuleName, logger.get()));

  auto test_module = std::make_unique<TestHardwareModule>();
  auto* raw_module = test_module.get();

  INTR_ASSERT_OK_AND_ASSIGN(
      auto runtime, HardwareModuleRuntime::Create(
                        kModuleName, kControlPeriod, std::move(shm_manager),
                        std::move(test_module), logger.get()));

  EXPECT_EQ(&runtime->GetHardwareModule(), raw_module);
  const auto& const_runtime = *runtime;
  EXPECT_EQ(&const_runtime.GetHardwareModule(), raw_module);
}

TEST(HardwareModuleRuntimeTest, InterfacesAreCorrectlyAdvertised) {
  log::MockLogSink mock_log_sink;
  const auto logger = std::make_unique<log::Logger>(
      log::Logger::Severity::kDebug, mock_log_sink);
  std::string memory_namespace = UniqueMemoryNamespace();
  INTR_ASSERT_OK_AND_ASSIGN(
      auto shm_manager,
      SharedMemoryManager::Create(memory_namespace, kModuleName, logger.get()));

  auto test_module = std::make_unique<TestHardwareModule>();

  INTR_ASSERT_OK_AND_ASSIGN(
      auto runtime, HardwareModuleRuntime::Create(
                        kModuleName, kControlPeriod, std::move(shm_manager),
                        std::move(test_module), logger.get()));
  INTR_EXPECT_OK(runtime->Run());

  INTR_ASSERT_OK_AND_ASSIGN(
      const auto segment_name_to_fd_map,
      GetSegmentNameToFileDescriptorMap(
          SocketDirectoryFromNamespace(memory_namespace), kModuleName,
          kLongRunningOpTimeout, logger.get()));

  const std::vector<std::string> expected_interfaces = {
      "hardware_module_state",
      "joint_position_command",
      "joint_position_state",
      "joint_limits",
      std::string(kIconStateInterfaceName),
      std::string(kControlPeriodInterfaceName)};

  for (const auto& name : expected_interfaces) {
    EXPECT_TRUE(segment_name_to_fd_map.contains(name))
        << "Expected interface " << name << " not found";
  }
}

TEST_F(HardwareModuleRuntimeIpcFixture, ReadStatusApplyCommandWork) {
  const auto noop_states =
      NoopStates(/*op_states=*/{intrinsic_fbs::StateCode::kMotionEnabled});

  EXPECT_DOUBLE_EQ(joint_position_state_->position()->Get(0), 0.0);
  joint_position_command_->mutable_position()->Mutate(0, 3.3);

  for (const auto& noop_state : noop_states) {
    runtime_->SetStateTestOnly(noop_state);
    INTR_EXPECT_OK(Tick());
    EXPECT_DOUBLE_EQ(joint_position_state_->position()->Get(0), 0.0);
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  // Resets the module state so the test doesn't depend on the order above.
  runtime_->SetStateTestOnly(intrinsic_fbs::StateCode::kDeactivated);
  INTR_EXPECT_OK(PrepareActivateAndEnableMotion());
  EXPECT_EQ(hardware_module_state_->code(),
            intrinsic_fbs::StateCode::kMotionEnabled);
  INTR_EXPECT_OK(Tick());
  EXPECT_DOUBLE_EQ(joint_position_state_->position()->Get(0), 3.3);
}

TEST_F(HardwareModuleRuntimeIpcFixture,
       DeactivatePreventsApplyCommandImmediately) {
  runtime_->SetStateTestOnly(intrinsic_fbs::StateCode::kDeactivated);
  INTR_EXPECT_OK(PrepareActivateAndEnableMotion());
  EXPECT_EQ(hardware_module_state_->code(),
            intrinsic_fbs::StateCode::kMotionEnabled);
  joint_position_command_->mutable_position()->Mutate(0, 3.3);
  INTR_EXPECT_OK(Tick());
  std::atomic<bool> disable_motion_called{false};
  EXPECT_DOUBLE_EQ(joint_position_state_->position()->Get(0), 3.3);
  GetTestHardwareModule().SetTransitionCallback(
      [&disable_motion_called](TestHardwareModule::TransitionRequest request) {
        if (request == TestHardwareModule::TransitionRequest::kDisableMotion) {
          disable_motion_called.store(true);
        }
        return OkStatus();
      });
  GetTestHardwareModule().PauseOperationalStateTransitions();
  auto disable_future = AsyncDisableMotion();

  while (!disable_motion_called.load()) {
    INTR_EXPECT_OK(Tick());
    std::this_thread::sleep_for(kShortSleepDuration);
  }
  auto deactivate_future = AsyncDeactivate();

  while (mutable_hardware_module_state_->code() !=
         intrinsic_fbs::StateCode::kDeactivated) {
    std::this_thread::sleep_for(kShortSleepDuration);
  }

  GetTestHardwareModule().PauseOperationalStateTransitions(false);
  INTR_EXPECT_OK(disable_future.get());
  INTR_EXPECT_OK(deactivate_future.get());
}

TEST_F(HardwareModuleRuntimeIpcFixture, Prepare) {
  const std::unordered_set<intrinsic_fbs::StateCode> op_states = {
      intrinsic_fbs::StateCode::kDeactivated,
      intrinsic_fbs::StateCode::kActivated,
      intrinsic_fbs::StateCode::kMotionEnabling,
      intrinsic_fbs::StateCode::kMotionEnabled,
      intrinsic_fbs::StateCode::kMotionDisabling,
      intrinsic_fbs::StateCode::kFaulted,
      intrinsic_fbs::StateCode::kClearingFaults,
  };

  const auto noop_states = NoopStates(op_states);

  for (const auto& state : noop_states) {
    const std::string expected_message = std::format(
        "secret message {}", intrinsic_fbs::EnumNameStateCode(state));
    runtime_->SetStateTestOnly(state, expected_message);
    INTR_EXPECT_OK(Prepare());
    EXPECT_EQ(hardware_module_state_->code(), state);
    EXPECT_EQ(GetMessage(hardware_module_state_), expected_message);
  }

  for (const auto& state : op_states) {
    runtime_->SetStateTestOnly(state);
    INTR_EXPECT_OK(Prepare());
    EXPECT_EQ(hardware_module_state_->code(),
              intrinsic_fbs::StateCode::kPrepared)
        << "Could not switch from " << intrinsic_fbs::EnumNameStateCode(state)
        << " to kPrepared - Current state "
        << intrinsic_fbs::EnumNameStateCode(hardware_module_state_->code());
  }
}

TEST_F(HardwareModuleRuntimeIpcFixture, FailingPrepareResultsInFatallyFaulted) {
  GetTestHardwareModule().SetTransitionCallback(
      [](TestHardwareModule::TransitionRequest request) {
        if (request == TestHardwareModule::TransitionRequest::kPrepare) {
          return FormatStatus(StatusCode::kInternal, "Prepare failed");
        }
        return OkStatus();
      });
  runtime_->SetStateTestOnly(intrinsic_fbs::StateCode::kDeactivated);
  INTR_EXPECT_OK(Prepare());
  INTR_ASSERT_OK_AND_ASSIGN(auto state, runtime_->GetHardwareModuleState());
  EXPECT_EQ(state.code(), intrinsic_fbs::StateCode::kFatallyFaulted);
  EXPECT_EQ(intrinsic_fbs::GetMessage(&state), "Prepare failed");
}

TEST_F(HardwareModuleRuntimeIpcFixture, Activate) {
  const std::unordered_set<intrinsic_fbs::StateCode> op_states = {
      intrinsic_fbs::StateCode::kPrepared,
  };

  const auto noop_states = NoopStates(op_states);

  for (const auto& state : noop_states) {
    runtime_->SetStateTestOnly(state);
    INTR_EXPECT_OK(Activate());
    EXPECT_EQ(hardware_module_state_->code(), state);
  }

  for (const auto& state : op_states) {
    runtime_->SetStateTestOnly(state);
    INTR_EXPECT_OK(Activate());
    EXPECT_EQ(hardware_module_state_->code(),
              intrinsic_fbs::StateCode::kActivated)
        << "Could not switch from " << intrinsic_fbs::EnumNameStateCode(state)
        << " to kActivated - Current state "
        << intrinsic_fbs::EnumNameStateCode(hardware_module_state_->code());
    EXPECT_EQ(GetTestHardwareModule().GetCurrentState(),
              TestHardwareModule::State::kActivated);
  }
}

TEST_F(HardwareModuleRuntimeIpcFixture,
       FailingActivateResultsInFatallyFaulted) {
  GetTestHardwareModule().SetTransitionCallback(
      [](TestHardwareModule::TransitionRequest request) {
        if (request == TestHardwareModule::TransitionRequest::kActivate) {
          return FormatStatus(StatusCode::kInternal, "Activate failed");
        }
        return OkStatus();
      });
  runtime_->SetStateTestOnly(intrinsic_fbs::StateCode::kPrepared);
  INTR_EXPECT_OK(Activate());
  INTR_ASSERT_OK_AND_ASSIGN(auto state, runtime_->GetHardwareModuleState());
  EXPECT_EQ(state.code(), intrinsic_fbs::StateCode::kFatallyFaulted);
  EXPECT_EQ(intrinsic_fbs::GetMessage(&state), "Activate failed");
}

TEST_F(HardwareModuleRuntimeIpcFixture, Deactivate) {
  const std::unordered_set<intrinsic_fbs::StateCode> noop_states = {
      intrinsic_fbs::StateCode::kDeactivating,
      intrinsic_fbs::StateCode::kDeactivated,
      intrinsic_fbs::StateCode::kActivating,
      intrinsic_fbs::StateCode::kPreparing,
      intrinsic_fbs::StateCode::kInitFailed,
      intrinsic_fbs::StateCode::kFatallyFaulted,
  };

  for (const auto& noop_state : noop_states) {
    runtime_->SetStateTestOnly(noop_state);
    INTR_EXPECT_OK(Deactivate());
    EXPECT_EQ(hardware_module_state_->code(), noop_state);
  }

  const auto op_states = OpStates(noop_states);

  for (const auto& state : op_states) {
    runtime_->SetStateTestOnly(state);
    INTR_EXPECT_OK(Deactivate());
    EXPECT_EQ(hardware_module_state_->code(),
              intrinsic_fbs::StateCode::kDeactivated);
    EXPECT_EQ(GetTestHardwareModule().GetCurrentState(),
              TestHardwareModule::State::kDeactivated);
  }
}

TEST_F(HardwareModuleRuntimeIpcFixture,
       FailingDeactivateResultsInFatallyFaulted) {
  GetTestHardwareModule().SetTransitionCallback(
      [](TestHardwareModule::TransitionRequest request) {
        if (request == TestHardwareModule::TransitionRequest::kDeactivate) {
          return FormatStatus(StatusCode::kInternal, "Deactivate failed");
        }
        return OkStatus();
      });
  runtime_->SetStateTestOnly(intrinsic_fbs::StateCode::kActivated);
  INTR_EXPECT_OK(Deactivate());
  INTR_ASSERT_OK_AND_ASSIGN(auto state, runtime_->GetHardwareModuleState());
  EXPECT_EQ(state.code(), intrinsic_fbs::StateCode::kFatallyFaulted);
  EXPECT_EQ(intrinsic_fbs::GetMessage(&state), "Deactivate failed");
}

TEST_F(HardwareModuleRuntimeIpcFixture, EnableMotion) {
  const auto noop_states =
      NoopStates(/*op_states=*/{intrinsic_fbs::StateCode::kActivated});

  for (const auto& noop_state : noop_states) {
    runtime_->SetStateTestOnly(noop_state);
    INTR_EXPECT_OK(ReadStatus());
    INTR_EXPECT_OK(EnableMotion());
    EXPECT_EQ(hardware_module_state_->code(), noop_state);
  }

  runtime_->SetStateTestOnly(intrinsic_fbs::StateCode::kActivated);
  INTR_EXPECT_OK(EnableMotion());
  EXPECT_EQ(hardware_module_state_->code(),
            intrinsic_fbs::StateCode::kMotionEnabled);
  EXPECT_EQ(GetTestHardwareModule().GetCurrentState(),
            TestHardwareModule::State::kMotionEnabled);
}

TEST_F(HardwareModuleRuntimeIpcFixture, FailingEnableMotionResultsInFaulted) {
  GetTestHardwareModule().SetTransitionCallback(
      [](TestHardwareModule::TransitionRequest request) {
        if (request == TestHardwareModule::TransitionRequest::kEnableMotion) {
          return FormatStatus(StatusCode::kInternal, "EnableMotion failed");
        }
        return OkStatus();
      });
  runtime_->SetStateTestOnly(intrinsic_fbs::StateCode::kActivated);
  INTR_EXPECT_OK(EnableMotion());
  EXPECT_EQ(hardware_module_state_->code(), intrinsic_fbs::StateCode::kFaulted);
  EXPECT_THAT(GetMessage(hardware_module_state_),
              HasSubstr("EnableMotion failed"));
}

TEST_F(HardwareModuleRuntimeIpcFixture,
       AbortedEnableMotionResultsInFatallyFaulted) {
  GetTestHardwareModule().SetTransitionCallback(
      [](TestHardwareModule::TransitionRequest request) {
        if (request == TestHardwareModule::TransitionRequest::kEnableMotion) {
          return FormatStatus(StatusCode::kAborted, "EnableMotion aborted");
        }
        return OkStatus();
      });
  runtime_->SetStateTestOnly(intrinsic_fbs::StateCode::kActivated);
  INTR_EXPECT_OK(EnableMotion());
  EXPECT_EQ(hardware_module_state_->code(),
            intrinsic_fbs::StateCode::kFatallyFaulted);
  EXPECT_THAT(GetMessage(hardware_module_state_),
              HasSubstr("EnableMotion aborted"));
}

TEST_F(HardwareModuleRuntimeIpcFixture, DisableMotion) {
  const auto noop_states =
      NoopStates({intrinsic_fbs::StateCode::kMotionEnabled});

  for (const auto& noop_state : noop_states) {
    runtime_->SetStateTestOnly(noop_state);
    INTR_EXPECT_OK(ReadStatus());
    INTR_EXPECT_OK(DisableMotion());
    EXPECT_EQ(hardware_module_state_->code(), noop_state);
  }

  runtime_->SetStateTestOnly(intrinsic_fbs::StateCode::kMotionEnabled);
  INTR_EXPECT_OK(DisableMotion());
  EXPECT_EQ(hardware_module_state_->code(),
            intrinsic_fbs::StateCode::kActivated);
  EXPECT_EQ(GetTestHardwareModule().GetCurrentState(),
            TestHardwareModule::State::kActivated);
}

TEST_F(HardwareModuleRuntimeIpcFixture, FailingDisableMotionResultsInFaulted) {
  GetTestHardwareModule().SetTransitionCallback(
      [](TestHardwareModule::TransitionRequest request) {
        if (request == TestHardwareModule::TransitionRequest::kDisableMotion) {
          return FormatStatus(StatusCode::kInternal, "DisableMotion failed");
        }
        return OkStatus();
      });
  runtime_->SetStateTestOnly(intrinsic_fbs::StateCode::kMotionEnabled);
  INTR_EXPECT_OK(DisableMotion());
  EXPECT_EQ(hardware_module_state_->code(), intrinsic_fbs::StateCode::kFaulted);
  EXPECT_THAT(GetMessage(hardware_module_state_),
              HasSubstr("DisableMotion failed"));
}

TEST_F(HardwareModuleRuntimeIpcFixture,
       AbortedDisableMotionResultsInFatallyFaulted) {
  GetTestHardwareModule().SetTransitionCallback(
      [](TestHardwareModule::TransitionRequest request) {
        if (request == TestHardwareModule::TransitionRequest::kDisableMotion) {
          return FormatStatus(StatusCode::kAborted, "DisableMotion aborted");
        }
        return OkStatus();
      });
  runtime_->SetStateTestOnly(intrinsic_fbs::StateCode::kMotionEnabled);
  INTR_EXPECT_OK(DisableMotion());
  EXPECT_EQ(hardware_module_state_->code(),
            intrinsic_fbs::StateCode::kFatallyFaulted);
  EXPECT_THAT(GetMessage(hardware_module_state_),
              HasSubstr("DisableMotion aborted"));
}

TEST_F(HardwareModuleRuntimeIpcFixture, ClearFaults) {
  const auto noop_states =
      NoopStates(/*op_states=*/{intrinsic_fbs::StateCode::kFaulted});

  for (const auto& noop_state : noop_states) {
    runtime_->SetStateTestOnly(noop_state);
    INTR_EXPECT_OK(ReadStatus());
    INTR_EXPECT_OK(ClearFaults());
    EXPECT_EQ(hardware_module_state_->code(), noop_state);
  }

  runtime_->SetStateTestOnly(intrinsic_fbs::StateCode::kFaulted);
  INTR_EXPECT_OK(ClearFaults());
  EXPECT_EQ(hardware_module_state_->code(),
            intrinsic_fbs::StateCode::kActivated);
}

TEST_F(HardwareModuleRuntimeIpcFixture, FailingClearFaultsResultsInFaulted) {
  GetTestHardwareModule().SetTransitionCallback(
      [](TestHardwareModule::TransitionRequest request) {
        if (request == TestHardwareModule::TransitionRequest::kClearFaults) {
          return FormatStatus(StatusCode::kInternal, "ClearFaults failed");
        }
        return OkStatus();
      });
  runtime_->SetStateTestOnly(intrinsic_fbs::StateCode::kFaulted);
  INTR_EXPECT_OK(ClearFaults());
  EXPECT_EQ(hardware_module_state_->code(), intrinsic_fbs::StateCode::kFaulted);
  EXPECT_THAT(GetMessage(hardware_module_state_),
              HasSubstr("ClearFaults failed"));
}

TEST_F(HardwareModuleRuntimeIpcFixture,
       AbortedClearFaultsResultsInFatallyFaulted) {
  GetTestHardwareModule().SetTransitionCallback(
      [](TestHardwareModule::TransitionRequest request) {
        if (request == TestHardwareModule::TransitionRequest::kClearFaults) {
          return FormatStatus(StatusCode::kAborted, "ClearFaults aborted");
        }
        return OkStatus();
      });
  runtime_->SetStateTestOnly(intrinsic_fbs::StateCode::kFaulted);
  INTR_EXPECT_OK(ClearFaults());
  EXPECT_EQ(hardware_module_state_->code(),
            intrinsic_fbs::StateCode::kFatallyFaulted);
  EXPECT_THAT(GetMessage(hardware_module_state_),
              HasSubstr("ClearFaults aborted"));
}

TEST_F(HardwareModuleRuntimeIpcFixture, RtStateCodeIsForwarded) {
  runtime_->SetStateTestOnly(intrinsic_fbs::StateCode::kFaulted);
  EXPECT_THAT(runtime_->GetHardwareModuleState(),
              IsOkAndHolds(Property("HardwareModuleState::code",
                                    &intrinsic_fbs::HardwareModuleState::code,
                                    intrinsic_fbs::StateCode::kFaulted)));
  runtime_->SetStateTestOnly(intrinsic_fbs::StateCode::kDeactivated);
  EXPECT_THAT(runtime_->GetHardwareModuleState(),
              IsOkAndHolds(Property("HardwareModuleState::code",
                                    &intrinsic_fbs::HardwareModuleState::code,
                                    intrinsic_fbs::StateCode::kDeactivated)));
}

TEST_F(HardwareModuleRuntimeIpcFixture, RtStateFaultReasonIsForwarded) {
  INTR_ASSERT_OK(Prepare());
  INTR_ASSERT_OK(Activate());
  GetTestHardwareModule().SetFaultReason("too much rain");
  INTR_EXPECT_OK(ReadStatus());
  INTR_ASSERT_OK_AND_ASSIGN(auto state, runtime_->GetHardwareModuleState());
  EXPECT_EQ(intrinsic_fbs::GetMessage(&state), "too much rain");
}

TEST_F(HardwareModuleRuntimeIpcFixture,
       EnabledIsCalledAfterCallingEnableMotion) {
  INTR_ASSERT_OK(Prepare());
  INTR_ASSERT_OK(Activate());
  std::atomic<bool> enabled_called{false};
  GetTestHardwareModule().SetEnabledCallback([&enabled_called](bool enabled) {
    if (enabled) {
      enabled_called.store(true);
    }
    return OkStatus();
  });
  auto future = AsyncEnableMotion();
  while (!enabled_called.load()) {
    INTR_EXPECT_OK(ReadStatus());
    std::this_thread::sleep_for(kShortSleepDuration);
  }
  INTR_ASSERT_OK(future.get());
  EXPECT_TRUE(enabled_called.load());
  EXPECT_EQ(GetTestHardwareModule().GetCurrentState(),
            TestHardwareModule::State::kMotionEnabled);
}

TEST_F(HardwareModuleRuntimeIpcFixture,
       ErrorDuringEnabledCallbackResultsInFaultedState) {
  INTR_ASSERT_OK(Prepare());
  INTR_ASSERT_OK(Activate());

  GetTestHardwareModule().SetEnabledCallback([](bool enabled) {
    if (enabled) {
      return FormatStatus(StatusCode::kInternal, "rain during enabled");
    }
    return OkStatus();
  });
  INTR_ASSERT_OK(EnableMotion());
  EXPECT_EQ(hardware_module_state_->code(), intrinsic_fbs::StateCode::kFaulted);
  EXPECT_THAT(GetMessage(hardware_module_state_),
              AllOf(HasSubstr("Enabled() callback failed"),
                    HasSubstr("rain during enabled")));
}

TEST_F(HardwareModuleRuntimeIpcFixture, DisabledIsCalledWhenFaulting) {
  INTR_ASSERT_OK(Prepare());
  INTR_ASSERT_OK(Activate());
  INTR_ASSERT_OK(EnableMotion());
  std::atomic<bool> disabled_called{false};
  GetTestHardwareModule().SetEnabledCallback([&disabled_called](bool enabled) {
    if (!enabled) {
      disabled_called.store(true);
    }
    return OkStatus();
  });
  GetTestHardwareModule().SetFaultReason("too much rain");
  while (!disabled_called.load()) {
    INTR_EXPECT_OK(ReadStatus());
    std::this_thread::sleep_for(kShortSleepDuration);
  }
  EXPECT_TRUE(disabled_called.load());
  EXPECT_EQ(GetTestHardwareModule().GetCurrentState(),
            TestHardwareModule::State::kActivated);
}

TEST_F(HardwareModuleRuntimeIpcFixture,
       ErrorDuringDisabledCallbackResultsInFaultedState) {
  INTR_ASSERT_OK(Prepare());
  INTR_ASSERT_OK(Activate());
  INTR_ASSERT_OK(EnableMotion());

  GetTestHardwareModule().SetEnabledCallback([](bool enabled) {
    if (!enabled) {
      return FormatStatus(StatusCode::kInternal, "too much rain");
    }
    return OkStatus();
  });
  auto future = AsyncDisableMotion();
  while (future.wait_for(kControlPeriod) != std::future_status::ready) {
    INTR_EXPECT_OK(ReadStatus());
  }
  (void)future.get();
  EXPECT_EQ(this->hardware_module_state_->code(),
            intrinsic_fbs::StateCode::kFaulted);
  EXPECT_THAT(
      GetMessage(this->hardware_module_state_),
      AllOf(HasSubstr("Disabled() failed"), HasSubstr("too much rain")));
}

TEST_F(HardwareModuleRuntimeIpcFixture, ControlPeriodIsPopulated) {
  INTR_ASSERT_OK_AND_ASSIGN(
      const auto segment_name_to_fd_map,
      GetSegmentNameToFileDescriptorMap(
          SocketDirectoryFromNamespace(memory_namespace_), kModuleName,
          kLongRunningOpTimeout, logger_.get()));

  INTR_ASSERT_OK_AND_ASSIGN(
      auto seg,
      (ReadOnlyMemorySegment<intrinsic_fbs::ControlPeriod>::Get(
          segment_name_to_fd_map, kControlPeriodInterfaceName, logger_.get())));

  HardwareInterfaceHandle<intrinsic_fbs::ControlPeriod> handle(std::move(seg));
  EXPECT_EQ(handle->control_period_ns(), 1'000'000);
}

TEST_F(HardwareModuleRuntimeIpcFixture,
       RestartServerSetsRestartRequestedExitCode) {
  auto future = exit_code_promise_->GetSharedFuture();
  INTR_ASSERT_OK(Restart());
  EXPECT_EQ(future.get(), HardwareModuleExitCode::kRestartRequested);
  EXPECT_TRUE(exit_code_promise_->HasBeenSet());
}

TEST_F(HardwareModuleRuntimeIpcFixture,
       RestartServerRejectsRestartWhenMotionEnabled) {
  INTR_ASSERT_OK(PrepareActivateAndEnableMotion());
  INTR_ASSERT_OK(Restart());
  EXPECT_FALSE(exit_code_promise_->HasBeenSet());
}

TEST_F(HardwareModuleRuntimeIpcFixture,
       RestartServerSetsFatalFaultDuringInitExitCode) {
  runtime_->SetStateTestOnly(intrinsic_fbs::StateCode::kInitFailed);
  auto future = exit_code_promise_->GetSharedFuture();
  INTR_ASSERT_OK(Restart());
  EXPECT_EQ(future.get(), HardwareModuleExitCode::kFatalFaultDuringInit);
  EXPECT_TRUE(exit_code_promise_->HasBeenSet());
}

TEST_F(HardwareModuleRuntimeIpcFixture,
       RestartServerSetsFatalFaultDuringExecExitCode) {
  runtime_->SetStateTestOnly(intrinsic_fbs::StateCode::kFatallyFaulted);
  auto future = exit_code_promise_->GetSharedFuture();
  INTR_ASSERT_OK(Restart());
  EXPECT_EQ(future.get(), HardwareModuleExitCode::kFatalFaultDuringExec);
  EXPECT_TRUE(exit_code_promise_->HasBeenSet());
}

}  // namespace
}  // namespace intrinsic::icon
