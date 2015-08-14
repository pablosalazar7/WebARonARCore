// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_DBUS_FAKE_SESSION_MANAGER_CLIENT_H_
#define CHROMEOS_DBUS_FAKE_SESSION_MANAGER_CLIENT_H_

#include <map>
#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/observer_list.h"
#include "chromeos/dbus/session_manager_client.h"

namespace chromeos {

// A fake implementation of session_manager. Accepts policy blobs to be set and
// returns them unmodified.
class FakeSessionManagerClient : public SessionManagerClient {
 public:
  FakeSessionManagerClient();
  ~FakeSessionManagerClient() override;

  // SessionManagerClient overrides
  void Init(dbus::Bus* bus) override;
  void SetStubDelegate(StubDelegate* delegate) override;
  void AddObserver(Observer* observer) override;
  void RemoveObserver(Observer* observer) override;
  bool HasObserver(const Observer* observer) const override;
  bool IsScreenLocked() const override;
  void EmitLoginPromptVisible() override;
  void RestartJob(int pid, const std::string& command_line) override;
  void StartSession(const std::string& user_email) override;
  void StopSession() override;
  void NotifySupervisedUserCreationStarted() override;
  void NotifySupervisedUserCreationFinished() override;
  void StartDeviceWipe() override;
  void RequestLockScreen() override;
  void NotifyLockScreenShown() override;
  void NotifyLockScreenDismissed() override;
  void RetrieveActiveSessions(const ActiveSessionsCallback& callback) override;
  void RetrieveDevicePolicy(const RetrievePolicyCallback& callback) override;
  void RetrievePolicyForUser(const std::string& username,
                             const RetrievePolicyCallback& callback) override;
  std::string BlockingRetrievePolicyForUser(
      const std::string& username) override;
  void RetrieveDeviceLocalAccountPolicy(
      const std::string& account_id,
      const RetrievePolicyCallback& callback) override;
  void StoreDevicePolicy(const std::string& policy_blob,
                         const StorePolicyCallback& callback) override;
  void StorePolicyForUser(const std::string& username,
                          const std::string& policy_blob,
                          const StorePolicyCallback& callback) override;
  void StoreDeviceLocalAccountPolicy(
      const std::string& account_id,
      const std::string& policy_blob,
      const StorePolicyCallback& callback) override;
  void SetFlagsForUser(const std::string& username,
                       const std::vector<std::string>& flags) override;
  void GetServerBackedStateKeys(const StateKeysCallback& callback) override;

  const std::string& device_policy() const;
  void set_device_policy(const std::string& policy_blob);

  const std::string& user_policy(const std::string& username) const;
  void set_user_policy(const std::string& username,
                       const std::string& policy_blob);

  const std::string& device_local_account_policy(
      const std::string& account_id) const;
  void set_device_local_account_policy(const std::string& account_id,
                                       const std::string& policy_blob);

  // Notify observers about a property change completion.
  void OnPropertyChangeComplete(bool success);

  // Configures the list of state keys used to satisfy
  // GetServerBackedStateKeys() requests.
  void set_server_backed_state_keys(
      const std::vector<std::string>& state_keys) {
    server_backed_state_keys_ = state_keys;
  }

  int start_device_wipe_call_count() const {
    return start_device_wipe_call_count_;
  }

  // Returns how many times LockScreenShown() was called.
  int notify_lock_screen_shown_call_count() const {
    return notify_lock_screen_shown_call_count_;
  }

  // Returns how many times LockScreenDismissed() was called.
  int notify_lock_screen_dismissed_call_count() const {
    return notify_lock_screen_dismissed_call_count_;
  }

 private:
  std::string device_policy_;
  std::map<std::string, std::string> user_policies_;
  std::map<std::string, std::string> device_local_account_policy_;
  base::ObserverList<Observer> observers_;
  SessionManagerClient::ActiveSessionsMap user_sessions_;
  std::vector<std::string> server_backed_state_keys_;

  int start_device_wipe_call_count_;
  int notify_lock_screen_shown_call_count_;
  int notify_lock_screen_dismissed_call_count_;

  DISALLOW_COPY_AND_ASSIGN(FakeSessionManagerClient);
};

}  // namespace chromeos

#endif  // CHROMEOS_DBUS_FAKE_SESSION_MANAGER_CLIENT_H_
