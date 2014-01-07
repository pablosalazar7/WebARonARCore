// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/extension_registry.h"

#include <string>

#include "base/memory/ref_counted.h"
#include "extensions/common/extension.h"
#include "extensions/common/test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace extensions {
namespace {

typedef testing::Test ExtensionRegistryTest;

TEST_F(ExtensionRegistryTest, FillAndClearRegistry) {
  ExtensionRegistry registry;
  scoped_refptr<Extension> extension1 = test_util::CreateExtensionWithID("id1");
  scoped_refptr<Extension> extension2 = test_util::CreateExtensionWithID("id2");
  scoped_refptr<Extension> extension3 = test_util::CreateExtensionWithID("id3");
  scoped_refptr<Extension> extension4 = test_util::CreateExtensionWithID("id4");

  // All the sets start empty.
  EXPECT_EQ(0u, registry.enabled_extensions().size());
  EXPECT_EQ(0u, registry.disabled_extensions().size());
  EXPECT_EQ(0u, registry.terminated_extensions().size());
  EXPECT_EQ(0u, registry.blacklisted_extensions().size());

  // Extensions can be added to each set.
  registry.AddEnabled(extension1);
  registry.AddDisabled(extension2);
  registry.AddTerminated(extension3);
  registry.AddBlacklisted(extension4);

  EXPECT_EQ(1u, registry.enabled_extensions().size());
  EXPECT_EQ(1u, registry.disabled_extensions().size());
  EXPECT_EQ(1u, registry.terminated_extensions().size());
  EXPECT_EQ(1u, registry.blacklisted_extensions().size());

  // Clearing the registry clears all sets.
  registry.ClearAll();

  EXPECT_EQ(0u, registry.enabled_extensions().size());
  EXPECT_EQ(0u, registry.disabled_extensions().size());
  EXPECT_EQ(0u, registry.terminated_extensions().size());
  EXPECT_EQ(0u, registry.blacklisted_extensions().size());
}

// A simple test of adding and removing things from sets.
TEST_F(ExtensionRegistryTest, AddAndRemoveExtensionFromRegistry) {
  ExtensionRegistry registry;

  // Adding an extension works.
  scoped_refptr<Extension> extension = test_util::CreateExtensionWithID("id");
  EXPECT_TRUE(registry.AddEnabled(extension));
  EXPECT_EQ(1u, registry.enabled_extensions().size());

  // The extension was only added to one set.
  EXPECT_EQ(0u, registry.disabled_extensions().size());
  EXPECT_EQ(0u, registry.terminated_extensions().size());
  EXPECT_EQ(0u, registry.blacklisted_extensions().size());

  // Removing an extension works.
  EXPECT_TRUE(registry.RemoveEnabled(extension->id()));
  EXPECT_EQ(0u, registry.enabled_extensions().size());

  // Trying to remove an extension that isn't in the set fails cleanly.
  EXPECT_FALSE(registry.RemoveEnabled(extension->id()));
}

TEST_F(ExtensionRegistryTest, AddExtensionToRegistryTwice) {
  ExtensionRegistry registry;
  scoped_refptr<Extension> extension = test_util::CreateExtensionWithID("id");

  // An extension can exist in two sets at once. It would be nice to eliminate
  // this functionality, but some users of ExtensionRegistry need it.
  EXPECT_TRUE(registry.AddEnabled(extension));
  EXPECT_TRUE(registry.AddDisabled(extension));

  EXPECT_EQ(1u, registry.enabled_extensions().size());
  EXPECT_EQ(1u, registry.disabled_extensions().size());
  EXPECT_EQ(0u, registry.terminated_extensions().size());
  EXPECT_EQ(0u, registry.blacklisted_extensions().size());
}

}  // namespace
}  // namespace extensions
