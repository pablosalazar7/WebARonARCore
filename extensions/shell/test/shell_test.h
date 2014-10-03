// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_SHELL_TEST_SHELL_TEST_H_
#define EXTENSIONS_SHELL_TEST_SHELL_TEST_H_

#include "base/memory/scoped_ptr.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_base.h"

namespace base {
class FilePath;
}

namespace content {
class BrowserContext;
}

namespace extensions {

class ShellExtensionSystem;

// Base class for app shell browser tests.
class AppShellTest : public content::BrowserTestBase {
 public:
  AppShellTest();
  virtual ~AppShellTest();

  // content::BrowserTestBase implementation.
  virtual void SetUp() override;
  virtual void SetUpOnMainThread() override;
  virtual void RunTestOnMainThreadLoop() override;

  content::BrowserContext* browser_context() { return browser_context_; }

 protected:
  content::BrowserContext* browser_context_;
  ShellExtensionSystem* extension_system_;
};

}  // namespace extensions

#endif  // EXTENSIONS_SHELL_TEST_SHELL_TEST_H_
