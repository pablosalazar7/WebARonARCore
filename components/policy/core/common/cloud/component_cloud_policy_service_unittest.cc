// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/policy/core/common/cloud/component_cloud_policy_service.h"

#include <map>
#include <string>

#include "base/callback.h"
#include "base/files/scoped_temp_dir.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/stl_util.h"
#include "base/values.h"
#include "components/policy/core/common/cloud/cloud_policy_constants.h"
#include "components/policy/core/common/cloud/mock_cloud_policy_client.h"
#include "components/policy/core/common/cloud/mock_cloud_policy_store.h"
#include "components/policy/core/common/cloud/policy_builder.h"
#include "components/policy/core/common/cloud/resource_cache.h"
#include "components/policy/core/common/external_data_fetcher.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/core/common/policy_types.h"
#include "components/policy/core/common/schema.h"
#include "components/policy/core/common/schema_map.h"
#include "crypto/sha2.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"
#include "policy/proto/chrome_extension_policy.pb.h"
#include "policy/proto/device_management_backend.pb.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace em = enterprise_management;

using testing::Mock;

namespace policy {

namespace {

const char kTestExtension[] = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
const char kTestExtension2[] = "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";
const char kTestDownload[] = "http://example.com/getpolicy?id=123";

const char kTestPolicy[] =
    "{"
    "  \"Name\": {"
    "    \"Value\": \"disabled\""
    "  },"
    "  \"Second\": {"
    "    \"Value\": \"maybe\","
    "    \"Level\": \"Recommended\""
    "  }"
    "}";

const char kInvalidTestPolicy[] =
    "{"
    "  \"Name\": {"
    "    \"Value\": \"published\""
    "  },"
    "  \"Undeclared Name\": {"
    "    \"Value\": \"not published\""
    "  }"
    "}";

const char kTestSchema[] =
    "{"
    "  \"type\": \"object\","
    "  \"properties\": {"
    "    \"Name\": { \"type\": \"string\" },"
    "    \"Second\": { \"type\": \"string\" }"
    "  }"
    "}";

class MockComponentCloudPolicyDelegate
    : public ComponentCloudPolicyService::Delegate {
 public:
  virtual ~MockComponentCloudPolicyDelegate() {}

  MOCK_METHOD0(OnComponentCloudPolicyUpdated, void());
};

class TestURLRequestContextGetter : public net::URLRequestContextGetter {
 public:
  explicit TestURLRequestContextGetter(
      scoped_refptr<base::SingleThreadTaskRunner> task_runner)
      : task_runner_(task_runner) {}
  net::URLRequestContext* GetURLRequestContext() override { return nullptr; }
  scoped_refptr<base::SingleThreadTaskRunner> GetNetworkTaskRunner()
      const override {
    return task_runner_;
  }

 private:
  ~TestURLRequestContextGetter() override {}

  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
};

}  // namespace

class ComponentCloudPolicyServiceTest : public testing::Test {
 protected:
  ComponentCloudPolicyServiceTest()
      : request_context_(new TestURLRequestContextGetter(loop_.task_runner())),
        cache_(nullptr),
        client_(nullptr),
        core_(dm_protocol::kChromeUserPolicyType,
              std::string(),
              &store_,
              loop_.task_runner()) {}

  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());

    owned_cache_.reset(
        new ResourceCache(temp_dir_.path(), loop_.task_runner()));
    cache_ = owned_cache_.get();

    builder_.policy_data().set_policy_type(
        dm_protocol::kChromeExtensionPolicyType);
    builder_.policy_data().set_settings_entity_id(kTestExtension);
    builder_.payload().set_download_url(kTestDownload);
    builder_.payload().set_secure_hash(crypto::SHA256HashString(kTestPolicy));

    expected_policy_.Set("Name",
                         POLICY_LEVEL_MANDATORY,
                         POLICY_SCOPE_USER,
                         new base::StringValue("disabled"),
                         nullptr);
    expected_policy_.Set("Second",
                         POLICY_LEVEL_RECOMMENDED,
                         POLICY_SCOPE_USER,
                         new base::StringValue("maybe"),
                         nullptr);
  }

  void TearDown() override {
    // The service cleans up its backend on the background thread.
    service_.reset();
    RunUntilIdle();
  }

  void RunUntilIdle() {
    base::RunLoop().RunUntilIdle();
  }

  void Connect() {
    client_ = new MockCloudPolicyClient();
    service_.reset(new ComponentCloudPolicyService(
        &delegate_, &registry_, &core_, client_, owned_cache_.Pass(),
        request_context_, loop_.task_runner(), loop_.task_runner()));

    client_->SetDMToken(ComponentPolicyBuilder::kFakeToken);
    EXPECT_EQ(1u, client_->types_to_fetch_.size());
    core_.Connect(scoped_ptr<CloudPolicyClient>(client_));
    EXPECT_EQ(2u, client_->types_to_fetch_.size());

    // Also initialize the refresh scheduler, so that calls to
    // core()->RefreshSoon() trigger a FetchPolicy() call on the mock |client_|.
    // The |service_| should never trigger new fetches.
    EXPECT_CALL(*client_, FetchPolicy());
    core_.StartRefreshScheduler();
    RunUntilIdle();
    Mock::VerifyAndClearExpectations(client_);
  }

  void LoadStore() {
    EXPECT_FALSE(store_.is_initialized());

    em::PolicyData* data = new em::PolicyData();
    data->set_username(ComponentPolicyBuilder::kFakeUsername);
    data->set_request_token(ComponentPolicyBuilder::kFakeToken);
    store_.policy_.reset(data);

    store_.NotifyStoreLoaded();
    RunUntilIdle();
    EXPECT_TRUE(store_.is_initialized());
  }

  void InitializeRegistry() {
    registry_.RegisterComponent(
        PolicyNamespace(POLICY_DOMAIN_EXTENSIONS, kTestExtension),
        CreateTestSchema());
    registry_.SetReady(POLICY_DOMAIN_CHROME);
    registry_.SetReady(POLICY_DOMAIN_EXTENSIONS);
  }

  void PopulateCache() {
    EXPECT_TRUE(cache_->Store(
        "extension-policy", kTestExtension, CreateSerializedResponse()));
    EXPECT_TRUE(
        cache_->Store("extension-policy-data", kTestExtension, kTestPolicy));

    builder_.policy_data().set_settings_entity_id(kTestExtension2);
    EXPECT_TRUE(cache_->Store(
        "extension-policy", kTestExtension2, CreateSerializedResponse()));
    EXPECT_TRUE(
        cache_->Store("extension-policy-data", kTestExtension2, kTestPolicy));
  }

  scoped_ptr<em::PolicyFetchResponse> CreateResponse() {
    builder_.Build();
    return make_scoped_ptr(new em::PolicyFetchResponse(builder_.policy()));
  }

  std::string CreateSerializedResponse() {
    builder_.Build();
    return builder_.GetBlob();
  }

  Schema CreateTestSchema() {
    std::string error;
    Schema schema = Schema::Parse(kTestSchema, &error);
    EXPECT_TRUE(schema.valid()) << error;
    return schema;
  }

  base::MessageLoop loop_;
  base::ScopedTempDir temp_dir_;
  scoped_refptr<TestURLRequestContextGetter> request_context_;
  net::TestURLFetcherFactory fetcher_factory_;
  MockComponentCloudPolicyDelegate delegate_;
  // |cache_| is owned by the |service_| and is invalid once the |service_|
  // is destroyed.
  scoped_ptr<ResourceCache> owned_cache_;
  ResourceCache* cache_;
  MockCloudPolicyClient* client_;
  MockCloudPolicyStore store_;
  CloudPolicyCore core_;
  SchemaRegistry registry_;
  scoped_ptr<ComponentCloudPolicyService> service_;
  ComponentPolicyBuilder builder_;
  PolicyMap expected_policy_;
};

TEST_F(ComponentCloudPolicyServiceTest, InitializeStoreThenRegistry) {
  Connect();

  EXPECT_CALL(delegate_, OnComponentCloudPolicyUpdated()).Times(0);
  EXPECT_CALL(*client_, FetchPolicy()).Times(0);
  LoadStore();
  Mock::VerifyAndClearExpectations(client_);
  Mock::VerifyAndClearExpectations(&delegate_);
  EXPECT_FALSE(service_->is_initialized());

  EXPECT_CALL(delegate_, OnComponentCloudPolicyUpdated());
  EXPECT_CALL(*client_, FetchPolicy()).Times(0);
  InitializeRegistry();
  RunUntilIdle();
  Mock::VerifyAndClearExpectations(client_);
  Mock::VerifyAndClearExpectations(&delegate_);
  EXPECT_TRUE(service_->is_initialized());

  const PolicyBundle empty_bundle;
  EXPECT_TRUE(service_->policy().Equals(empty_bundle));
}

TEST_F(ComponentCloudPolicyServiceTest, InitializeRegistryThenStore) {
  Connect();

  EXPECT_CALL(delegate_, OnComponentCloudPolicyUpdated()).Times(0);
  EXPECT_CALL(*client_, FetchPolicy()).Times(0);
  InitializeRegistry();
  RunUntilIdle();
  Mock::VerifyAndClearExpectations(client_);
  Mock::VerifyAndClearExpectations(&delegate_);
  EXPECT_FALSE(service_->is_initialized());

  EXPECT_CALL(delegate_, OnComponentCloudPolicyUpdated());
  EXPECT_CALL(*client_, FetchPolicy()).Times(0);
  LoadStore();
  Mock::VerifyAndClearExpectations(client_);
  Mock::VerifyAndClearExpectations(&delegate_);
  EXPECT_TRUE(service_->is_initialized());
  EXPECT_EQ(2u, client_->types_to_fetch_.size());
  const PolicyBundle empty_bundle;
  EXPECT_TRUE(service_->policy().Equals(empty_bundle));
}

TEST_F(ComponentCloudPolicyServiceTest, InitializeWithCachedPolicy) {
  PopulateCache();
  Connect();

  EXPECT_CALL(delegate_, OnComponentCloudPolicyUpdated());
  EXPECT_CALL(*client_, FetchPolicy()).Times(0);
  InitializeRegistry();
  LoadStore();
  Mock::VerifyAndClearExpectations(client_);
  Mock::VerifyAndClearExpectations(&delegate_);

  EXPECT_TRUE(service_->is_initialized());
  EXPECT_EQ(2u, client_->types_to_fetch_.size());

  // kTestExtension2 is not in the registry so it was dropped.
  std::map<std::string, std::string> contents;
  cache_->LoadAllSubkeys("extension-policy", &contents);
  ASSERT_EQ(1u, contents.size());
  EXPECT_EQ(kTestExtension, contents.begin()->first);

  PolicyBundle expected_bundle;
  const PolicyNamespace ns(POLICY_DOMAIN_EXTENSIONS, kTestExtension);
  expected_bundle.Get(ns).CopyFrom(expected_policy_);
  EXPECT_TRUE(service_->policy().Equals(expected_bundle));
}

TEST_F(ComponentCloudPolicyServiceTest, FetchPolicy) {
  Connect();
  // Initialize the store and create the backend.
  // A refresh is not needed, because no components are registered yet.
  EXPECT_CALL(delegate_, OnComponentCloudPolicyUpdated());
  EXPECT_CALL(*client_, FetchPolicy()).Times(0);
  registry_.SetReady(POLICY_DOMAIN_CHROME);
  registry_.SetReady(POLICY_DOMAIN_EXTENSIONS);
  LoadStore();
  Mock::VerifyAndClearExpectations(client_);
  Mock::VerifyAndClearExpectations(&delegate_);
  EXPECT_TRUE(service_->is_initialized());

  // Register the components to fetch. The |service_| issues a new update
  // because the new schema may filter different policies from the store.
  EXPECT_CALL(*client_, FetchPolicy()).Times(0);
  EXPECT_CALL(delegate_, OnComponentCloudPolicyUpdated());
  registry_.RegisterComponent(
      PolicyNamespace(POLICY_DOMAIN_EXTENSIONS, kTestExtension),
      CreateTestSchema());
  RunUntilIdle();
  Mock::VerifyAndClearExpectations(client_);
  Mock::VerifyAndClearExpectations(&delegate_);

  // Send back a fake policy fetch response.
  client_->SetPolicy(dm_protocol::kChromeExtensionPolicyType, kTestExtension,
                     *CreateResponse());
  service_->OnPolicyFetched(client_);
  RunUntilIdle();

  // That should have triggered the download fetch.
  net::TestURLFetcher* fetcher = fetcher_factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kTestDownload), fetcher->GetOriginalURL());
  fetcher->set_response_code(200);
  fetcher->SetResponseString(kTestPolicy);
  fetcher->delegate()->OnURLFetchComplete(fetcher);

  EXPECT_CALL(delegate_, OnComponentCloudPolicyUpdated());
  RunUntilIdle();
  Mock::VerifyAndClearExpectations(&delegate_);

  // The policy is now being served.
  const PolicyNamespace ns(POLICY_DOMAIN_EXTENSIONS, kTestExtension);
  PolicyBundle expected_bundle;
  expected_bundle.Get(ns).CopyFrom(expected_policy_);
  EXPECT_TRUE(service_->policy().Equals(expected_bundle));
}

TEST_F(ComponentCloudPolicyServiceTest, LoadAndPurgeCache) {
  Connect();
  // Insert data in the cache.
  PopulateCache();
  registry_.RegisterComponent(
      PolicyNamespace(POLICY_DOMAIN_EXTENSIONS, kTestExtension2),
      CreateTestSchema());
  InitializeRegistry();

  // Load the initial cache.
  EXPECT_CALL(delegate_, OnComponentCloudPolicyUpdated());
  EXPECT_CALL(*client_, FetchPolicy()).Times(0);
  LoadStore();
  Mock::VerifyAndClearExpectations(client_);
  Mock::VerifyAndClearExpectations(&delegate_);

  PolicyBundle expected_bundle;
  PolicyNamespace ns(POLICY_DOMAIN_EXTENSIONS, kTestExtension);
  expected_bundle.Get(ns).CopyFrom(expected_policy_);
  ns.component_id = kTestExtension2;
  expected_bundle.Get(ns).CopyFrom(expected_policy_);
  EXPECT_TRUE(service_->policy().Equals(expected_bundle));

  // Now purge one of the extensions. This generates 2 notifications: one for
  // the new, immediate filtering, and another once the backend comes back
  // after purging the cache.
  EXPECT_CALL(delegate_, OnComponentCloudPolicyUpdated()).Times(2);
  registry_.UnregisterComponent(
      PolicyNamespace(POLICY_DOMAIN_EXTENSIONS, kTestExtension));
  RunUntilIdle();
  Mock::VerifyAndClearExpectations(&delegate_);

  ns.component_id = kTestExtension;
  expected_bundle.Get(ns).Clear();
  EXPECT_TRUE(service_->policy().Equals(expected_bundle));

  std::map<std::string, std::string> contents;
  cache_->LoadAllSubkeys("extension-policy", &contents);
  EXPECT_EQ(1u, contents.size());
  EXPECT_TRUE(ContainsKey(contents, kTestExtension2));
}

TEST_F(ComponentCloudPolicyServiceTest, SignInAfterStartup) {
  registry_.SetReady(POLICY_DOMAIN_CHROME);
  registry_.SetReady(POLICY_DOMAIN_EXTENSIONS);

  // Initialize the store without credentials.
  EXPECT_FALSE(store_.is_initialized());
  store_.NotifyStoreLoaded();
  RunUntilIdle();

  // Register an extension.
  registry_.RegisterComponent(
      PolicyNamespace(POLICY_DOMAIN_EXTENSIONS, kTestExtension),
      CreateTestSchema());
  RunUntilIdle();

  // Now signin. The service will finish loading its backend (which is empty
  // for now, because there are no credentials) and issue a notification.
  EXPECT_CALL(delegate_, OnComponentCloudPolicyUpdated());
  Connect();
  Mock::VerifyAndClearExpectations(&delegate_);

  // Send the response to the service. The response data will be ignored,
  // because the store doesn't have the updated credentials yet.
  client_->SetPolicy(dm_protocol::kChromeExtensionPolicyType, kTestExtension,
                     *CreateResponse());
  service_->OnPolicyFetched(client_);
  RunUntilIdle();

  // The policy was ignored and no download is started because the store
  // doesn't have credentials.
  net::TestURLFetcher* fetcher = fetcher_factory_.GetFetcherByID(0);
  EXPECT_FALSE(fetcher);

  // Now update the |store_| with the updated policy, which includes
  // credentials. The responses in the |client_| will be reloaded.
  em::PolicyData* data = new em::PolicyData();
  data->set_username(ComponentPolicyBuilder::kFakeUsername);
  data->set_request_token(ComponentPolicyBuilder::kFakeToken);
  store_.policy_.reset(data);
  store_.NotifyStoreLoaded();
  RunUntilIdle();

  // The extension policy was validated this time, and the download is started.
  fetcher = fetcher_factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);
  EXPECT_EQ(GURL(kTestDownload), fetcher->GetOriginalURL());
  fetcher->set_response_code(200);
  fetcher->SetResponseString(kTestPolicy);
  fetcher->delegate()->OnURLFetchComplete(fetcher);

  EXPECT_CALL(delegate_, OnComponentCloudPolicyUpdated());
  RunUntilIdle();
  Mock::VerifyAndClearExpectations(&delegate_);

  // The policy is now being served.
  PolicyNamespace ns(POLICY_DOMAIN_EXTENSIONS, kTestExtension);
  PolicyBundle expected_bundle;
  expected_bundle.Get(ns).CopyFrom(expected_policy_);
  EXPECT_TRUE(service_->policy().Equals(expected_bundle));
}

TEST_F(ComponentCloudPolicyServiceTest, SignOut) {
  // Initialize everything and serve policy for a component.
  PopulateCache();
  LoadStore();
  InitializeRegistry();

  // The initial, cached policy will be served once the backend is initialized.
  EXPECT_CALL(delegate_, OnComponentCloudPolicyUpdated());
  Connect();
  Mock::VerifyAndClearExpectations(&delegate_);
  PolicyBundle expected_bundle;
  const PolicyNamespace ns(POLICY_DOMAIN_EXTENSIONS, kTestExtension);
  expected_bundle.Get(ns).CopyFrom(expected_policy_);
  EXPECT_TRUE(service_->policy().Equals(expected_bundle));
  std::map<std::string, std::string> contents;
  cache_->LoadAllSubkeys("extension-policy", &contents);
  ASSERT_EQ(1u, contents.size());

  // Signing out removes all of the component policies from the service and
  // from the cache. It does not trigger a refresh.
  EXPECT_CALL(delegate_, OnComponentCloudPolicyUpdated());
  core_.Disconnect();
  store_.policy_.reset();
  store_.NotifyStoreLoaded();
  RunUntilIdle();
  Mock::VerifyAndClearExpectations(&delegate_);
  const PolicyBundle empty_bundle;
  EXPECT_TRUE(service_->policy().Equals(empty_bundle));
  cache_->LoadAllSubkeys("extension-policy", &contents);
  ASSERT_EQ(0u, contents.size());
}

TEST_F(ComponentCloudPolicyServiceTest, LoadInvalidPolicyFromCache) {
  // Put the invalid test policy in the cache. One of its policies will be
  // loaded, the other should be filtered out by the schema.
  builder_.payload().set_secure_hash(
      crypto::SHA256HashString(kInvalidTestPolicy));
  EXPECT_TRUE(cache_->Store(
      "extension-policy", kTestExtension, CreateSerializedResponse()));
  EXPECT_TRUE(cache_->Store(
      "extension-policy-data", kTestExtension, kInvalidTestPolicy));

  LoadStore();
  InitializeRegistry();

  // The initial, cached policy will be served once the backend is initialized.
  EXPECT_CALL(delegate_, OnComponentCloudPolicyUpdated());
  Connect();
  Mock::VerifyAndClearExpectations(&delegate_);

  PolicyBundle expected_bundle;
  const PolicyNamespace ns(POLICY_DOMAIN_EXTENSIONS, kTestExtension);
  expected_bundle.Get(ns).Set("Name",
                              POLICY_LEVEL_MANDATORY,
                              POLICY_SCOPE_USER,
                              new base::StringValue("published"),
                              nullptr);
  EXPECT_TRUE(service_->policy().Equals(expected_bundle));
}

TEST_F(ComponentCloudPolicyServiceTest, PurgeWhenServerRemovesPolicy) {
  // Initialize with cached policy.
  PopulateCache();
  Connect();

  EXPECT_CALL(delegate_, OnComponentCloudPolicyUpdated());
  EXPECT_CALL(*client_, FetchPolicy()).Times(0);
  registry_.RegisterComponent(
      PolicyNamespace(POLICY_DOMAIN_EXTENSIONS, kTestExtension2),
      CreateTestSchema());
  InitializeRegistry();
  LoadStore();
  Mock::VerifyAndClearExpectations(client_);
  Mock::VerifyAndClearExpectations(&delegate_);

  EXPECT_TRUE(service_->is_initialized());
  EXPECT_EQ(2u, client_->types_to_fetch_.size());

  // Verify that policy for 2 extensions has been loaded from the cache.
  std::map<std::string, std::string> contents;
  cache_->LoadAllSubkeys("extension-policy", &contents);
  ASSERT_EQ(2u, contents.size());
  EXPECT_TRUE(ContainsKey(contents, kTestExtension));
  EXPECT_TRUE(ContainsKey(contents, kTestExtension2));

  PolicyBundle expected_bundle;
  const PolicyNamespace ns(POLICY_DOMAIN_EXTENSIONS, kTestExtension);
  expected_bundle.Get(ns).CopyFrom(expected_policy_);
  const PolicyNamespace ns2(POLICY_DOMAIN_EXTENSIONS, kTestExtension2);
  expected_bundle.Get(ns2).CopyFrom(expected_policy_);
  EXPECT_TRUE(service_->policy().Equals(expected_bundle));

  // Receive an updated fetch response from the server. There is no response for
  // extension 2, so it will be dropped from the cache. This triggers an
  // immediate notification to the delegate.
  EXPECT_CALL(delegate_, OnComponentCloudPolicyUpdated());
  client_->SetPolicy(dm_protocol::kChromeExtensionPolicyType, kTestExtension,
                     *CreateResponse());
  service_->OnPolicyFetched(client_);
  RunUntilIdle();
  Mock::VerifyAndClearExpectations(&delegate_);

  // That should have triggered the download fetch for the first extension.
  net::TestURLFetcher* fetcher = fetcher_factory_.GetFetcherByID(0);
  ASSERT_TRUE(fetcher);

  // The cache should have dropped the entries for the second extension.
  contents.clear();
  cache_->LoadAllSubkeys("extension-policy", &contents);
  ASSERT_EQ(1u, contents.size());
  EXPECT_TRUE(ContainsKey(contents, kTestExtension));
  EXPECT_FALSE(ContainsKey(contents, kTestExtension2));

  // And the service isn't publishing policy for the second extension anymore.
  expected_bundle.Clear();
  expected_bundle.Get(ns).CopyFrom(expected_policy_);
  EXPECT_TRUE(service_->policy().Equals(expected_bundle));
}

}  // namespace policy
