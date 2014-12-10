// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/prefs/pref_service.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/login/login_manager_test.h"
#include "chrome/browser/chromeos/login/startup_utils.h"
#include "chrome/browser/chromeos/login/test/oobe_screen_waiter.h"
#include "chrome/browser/chromeos/login/ui/login_display_host_impl.h"
#include "chrome/browser/chromeos/login/ui/oobe_display.h"
#include "chrome/browser/chromeos/login/ui/webui_login_view.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/pref_names.h"
#include "chromeos/chromeos_switches.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/fake_debug_daemon_client.h"
#include "chromeos/dbus/fake_power_manager_client.h"
#include "chromeos/dbus/fake_update_engine_client.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_utils.h"

namespace chromeos {

class TestDebugDaemonClient : public FakeDebugDaemonClient {
 public:
  TestDebugDaemonClient()
      : got_reply_(false),
        num_query_debugging_features_(0),
        num_enable_debugging_features_(0),
        num_remove_protection_(0) {
  }

  virtual ~TestDebugDaemonClient() {
  }

  void ResetWait() {
    got_reply_ = false;
    num_query_debugging_features_ = 0;
    num_enable_debugging_features_ = 0;
    num_remove_protection_ = 0;
  }

  int num_query_debugging_features() {
    return num_query_debugging_features_;
  }

  int num_enable_debugging_features() {
    return num_enable_debugging_features_;
  }

  int num_remove_protection() {
    return num_remove_protection_;
  }

  virtual void SetDebuggingFeaturesStatus(int featues_mask) override {
    ResetWait();
    FakeDebugDaemonClient::SetDebuggingFeaturesStatus(featues_mask);
  }

  void WaitUntilCalled() {
    if (got_reply_)
      return;

    runner_ = new content::MessageLoopRunner;
    runner_->Run();
  }

  virtual void EnableDebuggingFeatures(
      const std::string& password,
      const EnableDebuggingCallback& callback) override {
    FakeDebugDaemonClient::EnableDebuggingFeatures(
        password,
        base::Bind(&TestDebugDaemonClient::OnEnableDebuggingFeatures,
                   base::Unretained(this),
                   callback));
  }

  virtual void RemoveRootfsVerification(
      const DebugDaemonClient::EnableDebuggingCallback& callback) override {
    FakeDebugDaemonClient::RemoveRootfsVerification(
        base::Bind(&TestDebugDaemonClient::OnRemoveRootfsVerification,
                   base::Unretained(this),
                   callback));
  }

  virtual void QueryDebuggingFeatures(
      const DebugDaemonClient::QueryDevFeaturesCallback& callback) override {
      LOG(WARNING) << "QueryDebuggingFeatures";
    FakeDebugDaemonClient::QueryDebuggingFeatures(
        base::Bind(&TestDebugDaemonClient::OnQueryDebuggingFeatures,
                   base::Unretained(this),
                   callback));
  }

  void OnRemoveRootfsVerification(
      const DebugDaemonClient::EnableDebuggingCallback& original_callback,
      bool succeeded) {
    LOG(WARNING) << "OnRemoveRootfsVerification: succeeded = " << succeeded;
    base::MessageLoop::current()->PostTask(
        FROM_HERE,
        base::Bind(original_callback, succeeded));
    if (runner_.get())
      runner_->Quit();
    else
      got_reply_ = true;

    num_remove_protection_++;
  }

  void OnQueryDebuggingFeatures(
      const DebugDaemonClient::QueryDevFeaturesCallback& original_callback,
      bool succeeded,
      int feature_mask) {
    LOG(WARNING) << "OnQueryDebuggingFeatures: succeeded = " << succeeded
                 << ", feature_mask = " << feature_mask;
    base::MessageLoop::current()->PostTask(
        FROM_HERE,
        base::Bind(original_callback, succeeded, feature_mask));
    if (runner_.get())
      runner_->Quit();
    else
      got_reply_ = true;

    num_query_debugging_features_++;
  }

  void OnEnableDebuggingFeatures(
      const DebugDaemonClient::EnableDebuggingCallback& original_callback,
      bool succeeded) {
    LOG(WARNING) << "OnEnableDebuggingFeatures: succeeded = " << succeeded
                 << ", feature_mask = ";
    base::MessageLoop::current()->PostTask(
        FROM_HERE,
        base::Bind(original_callback, succeeded));
    if (runner_.get())
      runner_->Quit();
    else
      got_reply_ = true;

    num_enable_debugging_features_++;
  }

 private:
  scoped_refptr<content::MessageLoopRunner> runner_;
  bool got_reply_;
  int num_query_debugging_features_;
  int num_enable_debugging_features_;
  int num_remove_protection_;
};

class EnableDebuggingTest : public LoginManagerTest {
 public:
  EnableDebuggingTest() : LoginManagerTest(false),
      debug_daemon_client_(NULL),
      power_manager_client_(NULL) {
  }
  virtual ~EnableDebuggingTest() {}

  virtual void SetUpCommandLine(base::CommandLine* command_line) override {
    LoginManagerTest::SetUpCommandLine(command_line);
    command_line->AppendSwitch(chromeos::switches::kSystemDevMode);
  }

  // LoginManagerTest overrides:
  virtual void SetUpInProcessBrowserTestFixture() override {
    scoped_ptr<DBusThreadManagerSetter> dbus_setter =
        chromeos::DBusThreadManager::GetSetterForTesting();
    power_manager_client_ = new FakePowerManagerClient;
    dbus_setter->SetPowerManagerClient(
        scoped_ptr<PowerManagerClient>(power_manager_client_));
    debug_daemon_client_ = new TestDebugDaemonClient;
    dbus_setter->SetDebugDaemonClient(
        scoped_ptr<DebugDaemonClient>(debug_daemon_client_));

    LoginManagerTest::SetUpInProcessBrowserTestFixture();
  }

  bool JSExecuted(const std::string& script) {
    return content::ExecuteScript(web_contents(), script);
  }

  void WaitUntilJSIsReady() {
    LoginDisplayHostImpl* host = static_cast<LoginDisplayHostImpl*>(
        LoginDisplayHostImpl::default_host());
    if (!host)
      return;
    chromeos::OobeUI* oobe_ui = host->GetOobeUI();
    if (!oobe_ui)
      return;
    base::RunLoop run_loop;
    const bool oobe_ui_ready = oobe_ui->IsJSReady(run_loop.QuitClosure());
    if (!oobe_ui_ready)
      run_loop.Run();
  }

  void InvokeEnableDebuggingScreen() {
    ASSERT_TRUE(JSExecuted("cr.ui.Oobe.handleAccelerator('debugging');"));
    OobeScreenWaiter(OobeDisplay::SCREEN_OOBE_ENABLE_DEBUGGING).Wait();
  }

  void CloseEnableDebuggingScreen() {
    ASSERT_TRUE(JSExecuted("$('debugging-cancel-button').click();"));
  }

  void ClickRemoveProtectionButton() {
    ASSERT_TRUE(JSExecuted("$('debugging-remove-protection-button').click();"));
  }

  void ClickEnableButton() {
    ASSERT_TRUE(JSExecuted("$('debugging-enable-button').click();"));
  }

  void ClickOKButton() {
    ASSERT_TRUE(JSExecuted("$('debugging-ok-button').click();"));
  }

  void ShowRemoveProtectionScreen() {
    debug_daemon_client_->SetDebuggingFeaturesStatus(
        DebugDaemonClient::DEV_FEATURE_NONE);
    WaitUntilJSIsReady();
    JSExpect("!!document.querySelector('#debugging.hidden')");
    InvokeEnableDebuggingScreen();
    JSExpect("!document.querySelector('#debugging.hidden')");
    debug_daemon_client_->WaitUntilCalled();
    base::MessageLoop::current()->RunUntilIdle();
    VerifyRemoveProtectionScreen();
  }

  void VerifyRemoveProtectionScreen() {
    JSExpect("!!document.querySelector('#debugging.remove-protection-view')");
    JSExpect("!document.querySelector('#debugging.setup-view')");
    JSExpect("!document.querySelector('#debugging.done-view')");
    JSExpect("!document.querySelector('#debugging.wait-view')");
  }

  void ShowSetupScreen() {
    debug_daemon_client_->SetDebuggingFeaturesStatus(
        DebugDaemonClient::DEV_FEATURE_ROOTFS_VERIFICATION_REMOVED);
    WaitUntilJSIsReady();
    JSExpect("!!document.querySelector('#debugging.hidden')");
    InvokeEnableDebuggingScreen();
    JSExpect("!document.querySelector('#debugging.hidden')");
    debug_daemon_client_->WaitUntilCalled();
    base::MessageLoop::current()->RunUntilIdle();
    JSExpect("!document.querySelector('#debugging.remove-protection-view')");
    JSExpect("!!document.querySelector('#debugging.setup-view')");
    JSExpect("!document.querySelector('#debugging.done-view')");
    JSExpect("!document.querySelector('#debugging.wait-view')");
  }

  TestDebugDaemonClient* debug_daemon_client_;
  FakePowerManagerClient* power_manager_client_;
};

// Show remove protection screen, click on [Cancel] button.
IN_PROC_BROWSER_TEST_F(EnableDebuggingTest, ShowAndCancelRemoveProtection) {
  ShowRemoveProtectionScreen();
  CloseEnableDebuggingScreen();
  JSExpect("!!document.querySelector('#debugging.hidden')");

  EXPECT_EQ(debug_daemon_client_->num_query_debugging_features(), 1);
  EXPECT_EQ(debug_daemon_client_->num_enable_debugging_features(), 0);
  EXPECT_EQ(debug_daemon_client_->num_remove_protection(), 0);
}

// Show remove protection, click on [Remove protection] button and wait for
// reboot.
IN_PROC_BROWSER_TEST_F(EnableDebuggingTest, ShowAndRemoveProtection) {
  ShowRemoveProtectionScreen();
  debug_daemon_client_->ResetWait();
  ClickRemoveProtectionButton();
  debug_daemon_client_->WaitUntilCalled();
  JSExpect("!!document.querySelector('#debugging.wait-view')");
  // Check if we have rebooted after enabling.
  base::MessageLoop::current()->RunUntilIdle();
  EXPECT_EQ(debug_daemon_client_->num_remove_protection(), 1);
  EXPECT_EQ(debug_daemon_client_->num_enable_debugging_features(), 0);
  EXPECT_EQ(power_manager_client_->num_request_restart_calls(), 1);

}

// Show setup screen. Click on [Enable] button. Wait until done screen is shown.
IN_PROC_BROWSER_TEST_F(EnableDebuggingTest, ShowSetup) {
  ShowSetupScreen();
  debug_daemon_client_->ResetWait();
  ClickEnableButton();
  debug_daemon_client_->WaitUntilCalled();
  base::MessageLoop::current()->RunUntilIdle();
  JSExpect("!!document.querySelector('#debugging.done-view')");
  EXPECT_EQ(debug_daemon_client_->num_enable_debugging_features(), 1);
  EXPECT_EQ(debug_daemon_client_->num_remove_protection(), 0);
}

// Test images come with some features enabled but still has rootfs protection.
// Invoking debug screen should show remove protection screen.
IN_PROC_BROWSER_TEST_F(EnableDebuggingTest, ShowOnTestImages) {
  debug_daemon_client_->SetDebuggingFeaturesStatus(
      DebugDaemonClient::DEV_FEATURE_SSH_SERVER_CONFIGURED |
      DebugDaemonClient::DEV_FEATURE_SYSTEM_ROOT_PASSWORD_SET);
  WaitUntilJSIsReady();
  JSExpect("!!document.querySelector('#debugging.hidden')");
  InvokeEnableDebuggingScreen();
  JSExpect("!document.querySelector('#debugging.hidden')");
  debug_daemon_client_->WaitUntilCalled();
  base::MessageLoop::current()->RunUntilIdle();
  VerifyRemoveProtectionScreen();

  EXPECT_EQ(debug_daemon_client_->num_query_debugging_features(), 1);
  EXPECT_EQ(debug_daemon_client_->num_enable_debugging_features(), 0);
  EXPECT_EQ(debug_daemon_client_->num_remove_protection(), 0);
}

class EnableDebuggingNonDevTest : public EnableDebuggingTest {
 public:
  EnableDebuggingNonDevTest() {
  }

  virtual void SetUpCommandLine(base::CommandLine* command_line) override {
    // Skip EnableDebuggingTest::SetUpCommandLine().
    LoginManagerTest::SetUpCommandLine(command_line);
  }

  // LoginManagerTest overrides:
  virtual void SetUpInProcessBrowserTestFixture() override {
    scoped_ptr<DBusThreadManagerSetter> dbus_setter =
        chromeos::DBusThreadManager::GetSetterForTesting();
    dbus_setter->SetDebugDaemonClient(
        scoped_ptr<DebugDaemonClient>(new FakeDebugDaemonClient));
    LoginManagerTest::SetUpInProcessBrowserTestFixture();
  }
};

// Try to show enable debugging dialog, we should see error screen here.
IN_PROC_BROWSER_TEST_F(EnableDebuggingNonDevTest, NoShowInNonDevMode) {
  JSExpect("!!document.querySelector('#debugging.hidden')");
  InvokeEnableDebuggingScreen();
  JSExpect("!document.querySelector('#debugging.hidden')");
  base::MessageLoop::current()->RunUntilIdle();
  JSExpect("!!document.querySelector('#debugging.error-view')");
  JSExpect("!document.querySelector('#debugging.remove-protection-view')");
  JSExpect("!document.querySelector('#debugging.setup-view')");
  JSExpect("!document.querySelector('#debugging.done-view')");
  JSExpect("!document.querySelector('#debugging.wait-view')");
}

}  // namespace chromeos
