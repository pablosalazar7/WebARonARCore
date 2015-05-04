// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/proxy/multi_threaded_proxy_resolver.h"

#include <vector>

#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/stl_util.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/synchronization/lock.h"
#include "base/synchronization/waitable_event.h"
#include "base/threading/platform_thread.h"
#include "net/base/net_errors.h"
#include "net/base/test_completion_callback.h"
#include "net/log/net_log.h"
#include "net/log/test_net_log.h"
#include "net/log/test_net_log_entry.h"
#include "net/log/test_net_log_util.h"
#include "net/proxy/mock_proxy_resolver.h"
#include "net/proxy/proxy_info.h"
#include "net/proxy/proxy_resolver_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using base::ASCIIToUTF16;

namespace net {

namespace {

// A synchronous mock ProxyResolver implementation, which can be used in
// conjunction with MultiThreadedProxyResolver.
//       - returns a single-item proxy list with the query's host.
class MockProxyResolver : public ProxyResolver {
 public:
  MockProxyResolver()
      : ProxyResolver(true /*expects_pac_bytes*/),
        worker_loop_(base::MessageLoop::current()),
        request_count_(0) {}

  // ProxyResolver implementation.
  int GetProxyForURL(const GURL& query_url,
                     ProxyInfo* results,
                     const CompletionCallback& callback,
                     RequestHandle* request,
                     const BoundNetLog& net_log) override {
    if (resolve_latency_ != base::TimeDelta())
      base::PlatformThread::Sleep(resolve_latency_);

    CheckIsOnWorkerThread();

    EXPECT_TRUE(callback.is_null());
    EXPECT_TRUE(request == NULL);

    // Write something into |net_log| (doesn't really have any meaning.)
    net_log.BeginEvent(NetLog::TYPE_PAC_JAVASCRIPT_ALERT);

    results->UseNamedProxy(query_url.host());

    // Return a success code which represents the request's order.
    return request_count_++;
  }

  void CancelRequest(RequestHandle request) override { NOTREACHED(); }

  LoadState GetLoadState(RequestHandle request) const override {
    NOTREACHED();
    return LOAD_STATE_IDLE;
  }

  void CancelSetPacScript() override { NOTREACHED(); }
  int SetPacScript(const scoped_refptr<ProxyResolverScriptData>& script_data,
                   const CompletionCallback& callback) override {
    NOTREACHED();
    return ERR_NOT_IMPLEMENTED;
  }

  int request_count() const { return request_count_; }

  void SetResolveLatency(base::TimeDelta latency) {
    resolve_latency_ = latency;
  }

 private:
  void CheckIsOnWorkerThread() {
    EXPECT_EQ(base::MessageLoop::current(), worker_loop_);
  }

  base::MessageLoop* worker_loop_;
  int request_count_;
  base::TimeDelta resolve_latency_;
};


// A mock synchronous ProxyResolver which can be set to block upon reaching
// GetProxyForURL().
// TODO(eroman): WaitUntilBlocked() *must* be called before calling Unblock(),
//               otherwise there will be a race on |should_block_| since it is
//               read without any synchronization.
class BlockableProxyResolver : public MockProxyResolver {
 public:
  BlockableProxyResolver()
      : should_block_(false),
        unblocked_(true, true),
        blocked_(true, false) {
  }

  void Block() {
    should_block_ = true;
    unblocked_.Reset();
  }

  void Unblock() {
    should_block_ = false;
    blocked_.Reset();
    unblocked_.Signal();
  }

  void WaitUntilBlocked() {
    blocked_.Wait();
  }

  int GetProxyForURL(const GURL& query_url,
                     ProxyInfo* results,
                     const CompletionCallback& callback,
                     RequestHandle* request,
                     const BoundNetLog& net_log) override {
    if (should_block_) {
      blocked_.Signal();
      unblocked_.Wait();
    }

    return MockProxyResolver::GetProxyForURL(
        query_url, results, callback, request, net_log);
  }

 private:
  bool should_block_;
  base::WaitableEvent unblocked_;
  base::WaitableEvent blocked_;
};

// This factory returns new instances of BlockableProxyResolver.
class BlockableProxyResolverFactory : public ProxyResolverFactory {
 public:
  BlockableProxyResolverFactory() : ProxyResolverFactory(false) {}

  ~BlockableProxyResolverFactory() override {}

  int CreateProxyResolver(
      const scoped_refptr<ProxyResolverScriptData>& script_data,
      scoped_ptr<ProxyResolver>* result,
      const CompletionCallback& callback,
      scoped_ptr<Request>* request) override {
    BlockableProxyResolver* resolver = new BlockableProxyResolver;
    result->reset(resolver);
    base::AutoLock l(lock_);
    resolvers_.push_back(resolver);
    script_data_.push_back(script_data);
    return OK;
  }

  std::vector<BlockableProxyResolver*> resolvers() {
    base::AutoLock l(lock_);
    return resolvers_;
  }

  const std::vector<scoped_refptr<ProxyResolverScriptData>> script_data() {
    base::AutoLock l(lock_);
    return script_data_;
  }

 private:
  std::vector<BlockableProxyResolver*> resolvers_;
  std::vector<scoped_refptr<ProxyResolverScriptData>> script_data_;
  base::Lock lock_;
};

class SingleShotMultiThreadedProxyResolverFactory
    : public MultiThreadedProxyResolverFactory {
 public:
  SingleShotMultiThreadedProxyResolverFactory(
      size_t max_num_threads,
      scoped_ptr<ProxyResolverFactory> factory)
      : MultiThreadedProxyResolverFactory(max_num_threads, false),
        factory_(factory.Pass()) {}

  scoped_ptr<ProxyResolverFactory> CreateProxyResolverFactory() override {
    DCHECK(factory_);
    return factory_.Pass();
  }

 private:
  scoped_ptr<ProxyResolverFactory> factory_;
};

class MultiThreadedProxyResolverTest : public testing::Test {
 public:
  void Init(size_t num_threads) {
    scoped_ptr<BlockableProxyResolverFactory> factory_owner(
        new BlockableProxyResolverFactory);
    factory_ = factory_owner.get();
    resolver_factory_.reset(new SingleShotMultiThreadedProxyResolverFactory(
        num_threads, factory_owner.Pass()));
    TestCompletionCallback ready_callback;
    scoped_ptr<ProxyResolverFactory::Request> request;
    resolver_factory_->CreateProxyResolver(
        ProxyResolverScriptData::FromUTF8("pac script bytes"), &resolver_,
        ready_callback.callback(), &request);
    EXPECT_TRUE(request);
    ASSERT_EQ(OK, ready_callback.WaitForResult());

    // Verify that the script data reaches the synchronous resolver factory.
    ASSERT_EQ(1u, factory_->script_data().size());
    EXPECT_EQ(ASCIIToUTF16("pac script bytes"),
              factory_->script_data()[0]->utf16());
  }

  void ClearResolver() { resolver_.reset(); }

  BlockableProxyResolverFactory& factory() {
    DCHECK(factory_);
    return *factory_;
  }
  ProxyResolver& resolver() {
    DCHECK(resolver_);
    return *resolver_;
  }

 private:
  BlockableProxyResolverFactory* factory_ = nullptr;
  scoped_ptr<ProxyResolverFactory> factory_owner_;
  scoped_ptr<MultiThreadedProxyResolverFactory> resolver_factory_;
  scoped_ptr<ProxyResolver> resolver_;
};

TEST_F(MultiThreadedProxyResolverTest, SingleThread_Basic) {
  const size_t kNumThreads = 1u;
  ASSERT_NO_FATAL_FAILURE(Init(kNumThreads));

  // Start request 0.
  int rv;
  TestCompletionCallback callback0;
  BoundTestNetLog log0;
  ProxyInfo results0;
  rv = resolver().GetProxyForURL(GURL("http://request0"), &results0,
                                 callback0.callback(), NULL, log0.bound());
  EXPECT_EQ(ERR_IO_PENDING, rv);

  // Wait for request 0 to finish.
  rv = callback0.WaitForResult();
  EXPECT_EQ(0, rv);
  EXPECT_EQ("PROXY request0:80", results0.ToPacString());

  // The mock proxy resolver should have written 1 log entry. And
  // on completion, this should have been copied into |log0|.
  // We also have 1 log entry that was emitted by the
  // MultiThreadedProxyResolver.
  TestNetLogEntry::List entries0;
  log0.GetEntries(&entries0);

  ASSERT_EQ(2u, entries0.size());
  EXPECT_EQ(NetLog::TYPE_SUBMITTED_TO_RESOLVER_THREAD, entries0[0].type);

  // Start 3 more requests (request1 to request3).

  TestCompletionCallback callback1;
  ProxyInfo results1;
  rv = resolver().GetProxyForURL(GURL("http://request1"), &results1,
                                 callback1.callback(), NULL, BoundNetLog());
  EXPECT_EQ(ERR_IO_PENDING, rv);

  TestCompletionCallback callback2;
  ProxyInfo results2;
  rv = resolver().GetProxyForURL(GURL("http://request2"), &results2,
                                 callback2.callback(), NULL, BoundNetLog());
  EXPECT_EQ(ERR_IO_PENDING, rv);

  TestCompletionCallback callback3;
  ProxyInfo results3;
  rv = resolver().GetProxyForURL(GURL("http://request3"), &results3,
                                 callback3.callback(), NULL, BoundNetLog());
  EXPECT_EQ(ERR_IO_PENDING, rv);

  // Wait for the requests to finish (they must finish in the order they were
  // started, which is what we check for from their magic return value)

  rv = callback1.WaitForResult();
  EXPECT_EQ(1, rv);
  EXPECT_EQ("PROXY request1:80", results1.ToPacString());

  rv = callback2.WaitForResult();
  EXPECT_EQ(2, rv);
  EXPECT_EQ("PROXY request2:80", results2.ToPacString());

  rv = callback3.WaitForResult();
  EXPECT_EQ(3, rv);
  EXPECT_EQ("PROXY request3:80", results3.ToPacString());
}

// Tests that the NetLog is updated to include the time the request was waiting
// to be scheduled to a thread.
TEST_F(MultiThreadedProxyResolverTest,
       SingleThread_UpdatesNetLogWithThreadWait) {
  const size_t kNumThreads = 1u;
  ASSERT_NO_FATAL_FAILURE(Init(kNumThreads));

  int rv;

  // Block the proxy resolver, so no request can complete.
  factory().resolvers()[0]->Block();

  // Start request 0.
  ProxyResolver::RequestHandle request0;
  TestCompletionCallback callback0;
  ProxyInfo results0;
  BoundTestNetLog log0;
  rv = resolver().GetProxyForURL(GURL("http://request0"), &results0,
                                 callback0.callback(), &request0, log0.bound());
  EXPECT_EQ(ERR_IO_PENDING, rv);

  // Start 2 more requests (request1 and request2).

  TestCompletionCallback callback1;
  ProxyInfo results1;
  BoundTestNetLog log1;
  rv = resolver().GetProxyForURL(GURL("http://request1"), &results1,
                                 callback1.callback(), NULL, log1.bound());
  EXPECT_EQ(ERR_IO_PENDING, rv);

  ProxyResolver::RequestHandle request2;
  TestCompletionCallback callback2;
  ProxyInfo results2;
  BoundTestNetLog log2;
  rv = resolver().GetProxyForURL(GURL("http://request2"), &results2,
                                 callback2.callback(), &request2, log2.bound());
  EXPECT_EQ(ERR_IO_PENDING, rv);

  // Unblock the worker thread so the requests can continue running.
  factory().resolvers()[0]->WaitUntilBlocked();
  factory().resolvers()[0]->Unblock();

  // Check that request 0 completed as expected.
  // The NetLog has 1 entry that came from the MultiThreadedProxyResolver, and
  // 1 entry from the mock proxy resolver.
  EXPECT_EQ(0, callback0.WaitForResult());
  EXPECT_EQ("PROXY request0:80", results0.ToPacString());

  TestNetLogEntry::List entries0;
  log0.GetEntries(&entries0);

  ASSERT_EQ(2u, entries0.size());
  EXPECT_EQ(NetLog::TYPE_SUBMITTED_TO_RESOLVER_THREAD,
            entries0[0].type);

  // Check that request 1 completed as expected.
  EXPECT_EQ(1, callback1.WaitForResult());
  EXPECT_EQ("PROXY request1:80", results1.ToPacString());

  TestNetLogEntry::List entries1;
  log1.GetEntries(&entries1);

  ASSERT_EQ(4u, entries1.size());
  EXPECT_TRUE(LogContainsBeginEvent(
      entries1, 0,
      NetLog::TYPE_WAITING_FOR_PROXY_RESOLVER_THREAD));
  EXPECT_TRUE(LogContainsEndEvent(
      entries1, 1,
      NetLog::TYPE_WAITING_FOR_PROXY_RESOLVER_THREAD));

  // Check that request 2 completed as expected.
  EXPECT_EQ(2, callback2.WaitForResult());
  EXPECT_EQ("PROXY request2:80", results2.ToPacString());

  TestNetLogEntry::List entries2;
  log2.GetEntries(&entries2);

  ASSERT_EQ(4u, entries2.size());
  EXPECT_TRUE(LogContainsBeginEvent(
      entries2, 0,
      NetLog::TYPE_WAITING_FOR_PROXY_RESOLVER_THREAD));
  EXPECT_TRUE(LogContainsEndEvent(
      entries2, 1,
      NetLog::TYPE_WAITING_FOR_PROXY_RESOLVER_THREAD));
}

// Cancel a request which is in progress, and then cancel a request which
// is pending.
TEST_F(MultiThreadedProxyResolverTest, SingleThread_CancelRequest) {
  const size_t kNumThreads = 1u;
  ASSERT_NO_FATAL_FAILURE(Init(kNumThreads));

  int rv;

  // Block the proxy resolver, so no request can complete.
  factory().resolvers()[0]->Block();

  // Start request 0.
  ProxyResolver::RequestHandle request0;
  TestCompletionCallback callback0;
  ProxyInfo results0;
  rv =
      resolver().GetProxyForURL(GURL("http://request0"), &results0,
                                callback0.callback(), &request0, BoundNetLog());
  EXPECT_EQ(ERR_IO_PENDING, rv);

  // Wait until requests 0 reaches the worker thread.
  factory().resolvers()[0]->WaitUntilBlocked();

  // Start 3 more requests (request1 : request3).

  TestCompletionCallback callback1;
  ProxyInfo results1;
  rv = resolver().GetProxyForURL(GURL("http://request1"), &results1,
                                 callback1.callback(), NULL, BoundNetLog());
  EXPECT_EQ(ERR_IO_PENDING, rv);

  ProxyResolver::RequestHandle request2;
  TestCompletionCallback callback2;
  ProxyInfo results2;
  rv =
      resolver().GetProxyForURL(GURL("http://request2"), &results2,
                                callback2.callback(), &request2, BoundNetLog());
  EXPECT_EQ(ERR_IO_PENDING, rv);

  TestCompletionCallback callback3;
  ProxyInfo results3;
  rv = resolver().GetProxyForURL(GURL("http://request3"), &results3,
                                 callback3.callback(), NULL, BoundNetLog());
  EXPECT_EQ(ERR_IO_PENDING, rv);

  // Cancel request0 (inprogress) and request2 (pending).
  resolver().CancelRequest(request0);
  resolver().CancelRequest(request2);

  // Unblock the worker thread so the requests can continue running.
  factory().resolvers()[0]->Unblock();

  // Wait for requests 1 and 3 to finish.

  rv = callback1.WaitForResult();
  EXPECT_EQ(1, rv);
  EXPECT_EQ("PROXY request1:80", results1.ToPacString());

  rv = callback3.WaitForResult();
  // Note that since request2 was cancelled before reaching the resolver,
  // the request count is 2 and not 3 here.
  EXPECT_EQ(2, rv);
  EXPECT_EQ("PROXY request3:80", results3.ToPacString());

  // Requests 0 and 2 which were cancelled, hence their completion callbacks
  // were never summoned.
  EXPECT_FALSE(callback0.have_result());
  EXPECT_FALSE(callback2.have_result());
}

// Test that deleting MultiThreadedProxyResolver while requests are
// outstanding cancels them (and doesn't leak anything).
TEST_F(MultiThreadedProxyResolverTest, SingleThread_CancelRequestByDeleting) {
  const size_t kNumThreads = 1u;
  ASSERT_NO_FATAL_FAILURE(Init(kNumThreads));

  ASSERT_EQ(1u, factory().resolvers().size());

  // Block the proxy resolver, so no request can complete.
  factory().resolvers()[0]->Block();

  int rv;
  // Start 3 requests.

  TestCompletionCallback callback0;
  ProxyInfo results0;
  rv = resolver().GetProxyForURL(GURL("http://request0"), &results0,
                                 callback0.callback(), NULL, BoundNetLog());
  EXPECT_EQ(ERR_IO_PENDING, rv);

  TestCompletionCallback callback1;
  ProxyInfo results1;
  rv = resolver().GetProxyForURL(GURL("http://request1"), &results1,
                                 callback1.callback(), NULL, BoundNetLog());
  EXPECT_EQ(ERR_IO_PENDING, rv);

  TestCompletionCallback callback2;
  ProxyInfo results2;
  rv = resolver().GetProxyForURL(GURL("http://request2"), &results2,
                                 callback2.callback(), NULL, BoundNetLog());
  EXPECT_EQ(ERR_IO_PENDING, rv);

  // Wait until request 0 reaches the worker thread.
  factory().resolvers()[0]->WaitUntilBlocked();

  // Add some latency, to improve the chance that when
  // MultiThreadedProxyResolver is deleted below we are still running inside
  // of the worker thread. The test will pass regardless, so this race doesn't
  // cause flakiness. However the destruction during execution is a more
  // interesting case to test.
  factory().resolvers()[0]->SetResolveLatency(
      base::TimeDelta::FromMilliseconds(100));

  // Unblock the worker thread and delete the underlying
  // MultiThreadedProxyResolver immediately.
  factory().resolvers()[0]->Unblock();
  ClearResolver();

  // Give any posted tasks a chance to run (in case there is badness).
  base::MessageLoop::current()->RunUntilIdle();

  // Check that none of the outstanding requests were completed.
  EXPECT_FALSE(callback0.have_result());
  EXPECT_FALSE(callback1.have_result());
  EXPECT_FALSE(callback2.have_result());
}

// Tests setting the PAC script once, lazily creating new threads, and
// cancelling requests.
TEST_F(MultiThreadedProxyResolverTest, ThreeThreads_Basic) {
  const size_t kNumThreads = 3u;
  ASSERT_NO_FATAL_FAILURE(Init(kNumThreads));

  // Verify that it reaches the synchronous resolver.
  // One thread has been provisioned (i.e. one ProxyResolver was created).
  ASSERT_EQ(1u, factory().resolvers().size());

  const int kNumRequests = 9;
  int rv;
  TestCompletionCallback callback[kNumRequests];
  ProxyInfo results[kNumRequests];
  ProxyResolver::RequestHandle request[kNumRequests];

  // Start request 0 -- this should run on thread 0 as there is nothing else
  // going on right now.
  rv = resolver().GetProxyForURL(GURL("http://request0"), &results[0],
                                 callback[0].callback(), &request[0],
                                 BoundNetLog());
  EXPECT_EQ(ERR_IO_PENDING, rv);

  // Wait for request 0 to finish.
  rv = callback[0].WaitForResult();
  EXPECT_EQ(0, rv);
  EXPECT_EQ("PROXY request0:80", results[0].ToPacString());
  ASSERT_EQ(1u, factory().resolvers().size());
  EXPECT_EQ(1, factory().resolvers()[0]->request_count());

  base::MessageLoop::current()->RunUntilIdle();

  // We now start 8 requests in parallel -- this will cause the maximum of
  // three threads to be provisioned (an additional two from what we already
  // have).

  for (int i = 1; i < kNumRequests; ++i) {
    rv = resolver().GetProxyForURL(
        GURL(base::StringPrintf("http://request%d", i)), &results[i],
        callback[i].callback(), &request[i], BoundNetLog());
    EXPECT_EQ(ERR_IO_PENDING, rv);
  }

  // Cancel 3 of the 8 oustanding requests.
  resolver().CancelRequest(request[1]);
  resolver().CancelRequest(request[3]);
  resolver().CancelRequest(request[6]);

  // Wait for the remaining requests to complete.
  int kNonCancelledRequests[] = {2, 4, 5, 7, 8};
  for (size_t i = 0; i < arraysize(kNonCancelledRequests); ++i) {
    int request_index = kNonCancelledRequests[i];
    EXPECT_GE(callback[request_index].WaitForResult(), 0);
  }

  // Check that the cancelled requests never invoked their callback.
  EXPECT_FALSE(callback[1].have_result());
  EXPECT_FALSE(callback[3].have_result());
  EXPECT_FALSE(callback[6].have_result());

  // We should now have a total of 3 threads, each with its own ProxyResolver
  // that will get initialized with the same data.
  ASSERT_EQ(3u, factory().resolvers().size());

  ASSERT_EQ(3u, factory().script_data().size());
  for (int i = 0; i < 3; ++i) {
    EXPECT_EQ(ASCIIToUTF16("pac script bytes"),
              factory().script_data()[i]->utf16())
        << "i=" << i;
  }

  // We don't know the exact ordering that requests ran on threads with,
  // but we do know the total count that should have reached the threads.
  // 8 total were submitted, and three were cancelled. Of the three that
  // were cancelled, one of them (request 1) was cancelled after it had
  // already been posted to the worker thread. So the resolvers will
  // have seen 6 total (and 1 from the run prior).
  int total_count = 0;
  for (int i = 0; i < 3; ++i) {
    total_count += factory().resolvers()[i]->request_count();
  }
  EXPECT_EQ(7, total_count);
}

// Tests using two threads. The first request hangs the first thread. Checks
// that other requests are able to complete while this first request remains
// stalled.
TEST_F(MultiThreadedProxyResolverTest, OneThreadBlocked) {
  const size_t kNumThreads = 2u;
  ASSERT_NO_FATAL_FAILURE(Init(kNumThreads));

  int rv;

  // One thread has been provisioned (i.e. one ProxyResolver was created).
  ASSERT_EQ(1u, factory().resolvers().size());
  EXPECT_EQ(ASCIIToUTF16("pac script bytes"),
            factory().script_data()[0]->utf16());

  const int kNumRequests = 4;
  TestCompletionCallback callback[kNumRequests];
  ProxyInfo results[kNumRequests];
  ProxyResolver::RequestHandle request[kNumRequests];

  // Start a request that will block the first thread.

  factory().resolvers()[0]->Block();

  rv = resolver().GetProxyForURL(GURL("http://request0"), &results[0],
                                 callback[0].callback(), &request[0],
                                 BoundNetLog());

  EXPECT_EQ(ERR_IO_PENDING, rv);
  factory().resolvers()[0]->WaitUntilBlocked();

  // Start 3 more requests -- they should all be serviced by thread #2
  // since thread #1 is blocked.

  for (int i = 1; i < kNumRequests; ++i) {
    rv = resolver().GetProxyForURL(
        GURL(base::StringPrintf("http://request%d", i)), &results[i],
        callback[i].callback(), &request[i], BoundNetLog());
    EXPECT_EQ(ERR_IO_PENDING, rv);
  }

  // Wait for the three requests to complete (they should complete in FIFO
  // order).
  for (int i = 1; i < kNumRequests; ++i) {
    EXPECT_EQ(i - 1, callback[i].WaitForResult());
  }

  // Unblock the first thread.
  factory().resolvers()[0]->Unblock();
  EXPECT_EQ(0, callback[0].WaitForResult());

  // All in all, the first thread should have seen just 1 request. And the
  // second thread 3 requests.
  ASSERT_EQ(2u, factory().resolvers().size());
  EXPECT_EQ(1, factory().resolvers()[0]->request_count());
  EXPECT_EQ(3, factory().resolvers()[1]->request_count());
}

class FailingProxyResolverFactory : public ProxyResolverFactory {
 public:
  FailingProxyResolverFactory() : ProxyResolverFactory(false) {}

  // ProxyResolverFactory override.
  int CreateProxyResolver(
      const scoped_refptr<ProxyResolverScriptData>& script_data,
      scoped_ptr<ProxyResolver>* result,
      const CompletionCallback& callback,
      scoped_ptr<Request>* request) override {
    return ERR_PAC_SCRIPT_FAILED;
  }
};

// Test that an error when creating the synchronous resolver causes the
// MultiThreadedProxyResolverFactory create request to fail with that error.
TEST_F(MultiThreadedProxyResolverTest, ProxyResolverFactoryError) {
  const size_t kNumThreads = 1u;
  SingleShotMultiThreadedProxyResolverFactory resolver_factory(
      kNumThreads, make_scoped_ptr(new FailingProxyResolverFactory));
  TestCompletionCallback ready_callback;
  scoped_ptr<ProxyResolverFactory::Request> request;
  scoped_ptr<ProxyResolver> resolver;
  EXPECT_EQ(ERR_IO_PENDING,
            resolver_factory.CreateProxyResolver(
                ProxyResolverScriptData::FromUTF8("pac script bytes"),
                &resolver, ready_callback.callback(), &request));
  EXPECT_TRUE(request);
  EXPECT_EQ(ERR_PAC_SCRIPT_FAILED, ready_callback.WaitForResult());
  EXPECT_FALSE(resolver);
}

void Fail(int error) {
  FAIL() << "Unexpected callback with error " << error;
}

// Test that cancelling an in-progress create request works correctly.
TEST_F(MultiThreadedProxyResolverTest, CancelCreate) {
  const size_t kNumThreads = 1u;
  {
    SingleShotMultiThreadedProxyResolverFactory resolver_factory(
        kNumThreads, make_scoped_ptr(new BlockableProxyResolverFactory));
    scoped_ptr<ProxyResolverFactory::Request> request;
    scoped_ptr<ProxyResolver> resolver;
    EXPECT_EQ(ERR_IO_PENDING,
              resolver_factory.CreateProxyResolver(
                  ProxyResolverScriptData::FromUTF8("pac script bytes"),
                  &resolver, base::Bind(&Fail), &request));
    EXPECT_TRUE(request);
    request.reset();
  }
  // The factory destructor will block until the worker thread stops, but it may
  // post tasks to the origin message loop which are still pending. Run them
  // now to ensure it works as expected.
  base::RunLoop().RunUntilIdle();
}

void DeleteRequest(const CompletionCallback& callback,
                   scoped_ptr<ProxyResolverFactory::Request>* request,
                   int result) {
  callback.Run(result);
  request->reset();
}

// Test that delete the Request during the factory callback works correctly.
TEST_F(MultiThreadedProxyResolverTest, DeleteRequestInFactoryCallback) {
  const size_t kNumThreads = 1u;
  SingleShotMultiThreadedProxyResolverFactory resolver_factory(
      kNumThreads, make_scoped_ptr(new BlockableProxyResolverFactory));
  scoped_ptr<ProxyResolverFactory::Request> request;
  scoped_ptr<ProxyResolver> resolver;
  TestCompletionCallback callback;
  EXPECT_EQ(ERR_IO_PENDING,
            resolver_factory.CreateProxyResolver(
                ProxyResolverScriptData::FromUTF8("pac script bytes"),
                &resolver, base::Bind(&DeleteRequest, callback.callback(),
                                      base::Unretained(&request)),
                &request));
  EXPECT_TRUE(request);
  EXPECT_EQ(OK, callback.WaitForResult());
}

// Test that deleting the factory with a request in-progress works correctly.
TEST_F(MultiThreadedProxyResolverTest, DestroyFactoryWithRequestsInProgress) {
  const size_t kNumThreads = 1u;
  scoped_ptr<ProxyResolverFactory::Request> request;
  scoped_ptr<ProxyResolver> resolver;
  {
    SingleShotMultiThreadedProxyResolverFactory resolver_factory(
        kNumThreads, make_scoped_ptr(new BlockableProxyResolverFactory));
    EXPECT_EQ(ERR_IO_PENDING,
              resolver_factory.CreateProxyResolver(
                  ProxyResolverScriptData::FromUTF8("pac script bytes"),
                  &resolver, base::Bind(&Fail), &request));
    EXPECT_TRUE(request);
  }
  // The factory destructor will block until the worker thread stops, but it may
  // post tasks to the origin message loop which are still pending. Run them
  // now to ensure it works as expected.
  base::RunLoop().RunUntilIdle();
}

}  // namespace

}  // namespace net
