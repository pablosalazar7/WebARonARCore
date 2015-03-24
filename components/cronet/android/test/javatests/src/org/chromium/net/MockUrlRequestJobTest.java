// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.net;

import android.test.suitebuilder.annotation.LargeTest;
import android.test.suitebuilder.annotation.SmallTest;

import org.chromium.base.annotations.SuppressFBWarnings;
import org.chromium.base.test.util.Feature;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.List;
import java.util.concurrent.Executors;

/**
 * Tests that use mock URLRequestJobs to simulate URL requests.
 */
public class MockUrlRequestJobTest extends CronetTestBase {
    private static final String TAG = "MockURLRequestJobTest";

    private CronetTestActivity mActivity;
    private MockUrlRequestJobFactory mMockUrlRequestJobFactory;
    private TestHttpUrlRequestListener mListener;
    private HttpUrlRequest mRequest;

    // Helper function to create a HttpUrlRequest with the specified url.
    private void createRequestAndWaitForComplete(
            String url, boolean disableRedirects) {
        HashMap<String, String> headers = new HashMap<String, String>();
        mListener = new TestHttpUrlRequestListener();
        mRequest = mActivity.mRequestFactory.createRequest(
                url,
                HttpUrlRequest.REQUEST_PRIORITY_MEDIUM,
                headers,
                mListener);
        if (disableRedirects) {
            mRequest.disableRedirects();
        }
        mRequest.start();
        mListener.blockForComplete();
    }

    /**
     * Helper method to check that we are not calling methods on the native
     * adapter after it has been destroyed.
     */
    private static void checkAfterAdapterDestroyed(HttpUrlRequest request) {
        try {
            request.getAllHeaders();
            fail();
        } catch (IllegalStateException e) {
            assertEquals("Adapter has been destroyed", e.getMessage());
        }
        try {
            request.getHeader("foo");
            fail();
        } catch (IllegalStateException e) {
            assertEquals("Adapter has been destroyed", e.getMessage());
        }
        try {
            request.getNegotiatedProtocol();
            fail();
        } catch (IllegalStateException e) {
            assertEquals("Adapter has been destroyed", e.getMessage());
        }
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mActivity = launchCronetTestApp();
        mMockUrlRequestJobFactory = new MockUrlRequestJobFactory(
                getInstrumentation().getTargetContext());
    }

    @SmallTest
    @Feature({"Cronet"})
    public void testSuccessURLRequest() throws Exception {
        createRequestAndWaitForComplete(
                MockUrlRequestJobFactory.SUCCESS_URL, false);
        assertEquals(MockUrlRequestJobFactory.SUCCESS_URL, mListener.mUrl);
        assertEquals(200, mListener.mHttpStatusCode);
        assertEquals("OK", mListener.mHttpStatusText);
        assertEquals("this is a text file\n",
                new String(mListener.mResponseAsBytes));
        // Test that ChromiumUrlRequest caches information which is available
        // after the native request adapter has been destroyed.
        assertEquals(200, mRequest.getHttpStatusCode());
        assertEquals("OK", mRequest.getHttpStatusText());
        assertNull(mRequest.getException());
        checkAfterAdapterDestroyed(mRequest);
    }

    @SmallTest
    @Feature({"Cronet"})
    public void testRedirectURLRequest() throws Exception {
        createRequestAndWaitForComplete(
                MockUrlRequestJobFactory.REDIRECT_URL, false);
        // ChromiumUrlRequest does not expose the url after redirect.
        assertEquals(MockUrlRequestJobFactory.REDIRECT_URL, mListener.mUrl);
        assertEquals(200, mListener.mHttpStatusCode);
        assertEquals("OK", mListener.mHttpStatusText);
        // Expect that the request is redirected to success.txt.
        assertEquals("this is a text file\n",
                new String(mListener.mResponseAsBytes));
        // Test that ChromiumUrlRequest caches information which is available
        // after the native request adapter has been destroyed.
        assertEquals(200, mRequest.getHttpStatusCode());
        assertEquals("OK", mRequest.getHttpStatusText());
        assertNull(mRequest.getException());
        checkAfterAdapterDestroyed(mRequest);
    }

    @SmallTest
    @Feature({"Cronet"})
    public void testNotFoundURLRequest() throws Exception {
        createRequestAndWaitForComplete(
                MockUrlRequestJobFactory.NOTFOUND_URL, false);
        assertEquals(MockUrlRequestJobFactory.NOTFOUND_URL, mListener.mUrl);
        assertEquals(404, mListener.mHttpStatusCode);
        assertEquals("Not Found", mListener.mHttpStatusText);
        assertEquals(
                "<!DOCTYPE html>\n<html>\n<head>\n<title>Not found</title>\n"
                + "<p>Test page loaded.</p>\n</head>\n</html>\n",
                new String(mListener.mResponseAsBytes));
        // Test that ChromiumUrlRequest caches information which is available
        // after the native request adapter has been destroyed.
        assertEquals(404, mRequest.getHttpStatusCode());
        assertEquals("Not Found", mRequest.getHttpStatusText());
        assertNull(mRequest.getException());
        checkAfterAdapterDestroyed(mRequest);
    }

    @SmallTest
    @Feature({"Cronet"})
    public void testFailedURLRequest() throws Exception {
        createRequestAndWaitForComplete(
                MockUrlRequestJobFactory.FAILED_URL, false);
        assertEquals(MockUrlRequestJobFactory.FAILED_URL, mListener.mUrl);
        assertEquals("", mListener.mHttpStatusText);
        assertEquals(0, mListener.mHttpStatusCode);
        // Test that ChromiumUrlRequest caches information which is available
        // after the native request adapter has been destroyed.
        assertEquals(0, mRequest.getHttpStatusCode());
        assertEquals("", mRequest.getHttpStatusText());
        assertEquals("System error: net::ERR_FAILED(-2)",
                mRequest.getException().getMessage());
        checkAfterAdapterDestroyed(mRequest);
    }

    @SmallTest
    @Feature({"Cronet"})
    // Test that redirect can be disabled for a request.
    public void testDisableRedirects() throws Exception {
        createRequestAndWaitForComplete(
                MockUrlRequestJobFactory.REDIRECT_URL, true);
        // Currently Cronet does not expose the url after redirect.
        assertEquals(MockUrlRequestJobFactory.REDIRECT_URL, mListener.mUrl);
        assertEquals(302, mListener.mHttpStatusCode);
        // MockUrlRequestJob somehow does not populate status text as "Found".
        assertEquals("", mListener.mHttpStatusText);
        // Expect that the request is not redirected to success.txt.
        assertNotNull(mListener.mResponseHeaders);
        List<String> entry = mListener.mResponseHeaders.get("redirect-header");
        assertEquals(1, entry.size());
        assertEquals("header-value", entry.get(0));
        List<String> location = mListener.mResponseHeaders.get("Location");
        assertEquals(1, location.size());
        assertEquals("/success.txt", location.get(0));
        assertEquals("Request failed because there were too many redirects or "
                         + "redirects have been disabled",
                     mListener.mException.getMessage());
        // Test that ChromiumUrlRequest caches information which is available
        // after the native request adapter has been destroyed.
        assertEquals(302, mRequest.getHttpStatusCode());
        // MockUrlRequestJob somehow does not populate status text as "Found".
        assertEquals("", mRequest.getHttpStatusText());
        assertEquals("Request failed because there were too many redirects "
                + "or redirects have been disabled",
                mRequest.getException().getMessage());
        checkAfterAdapterDestroyed(mRequest);
    }

    /**
     * TestByteChannel is used for making sure write is not called after the
     * channel has been closed. Can synchronously cancel a request when write is
     * called.
     */
    static class TestByteChannel extends ChunkedWritableByteChannel {
        HttpUrlRequest mRequestToCancelOnWrite;

        @Override
        public int write(ByteBuffer byteBuffer) throws IOException {
            assertTrue(isOpen());
            if (mRequestToCancelOnWrite != null) {
                assertFalse(mRequestToCancelOnWrite.isCanceled());
                mRequestToCancelOnWrite.cancel();
                mRequestToCancelOnWrite = null;
            }
            return super.write(byteBuffer);
        }

        @Override
        public void close() {
            assertTrue(isOpen());
            super.close();
        }

        /**
         * Set request that will be synchronously canceled when write is called.
         */
        public void setRequestToCancelOnWrite(HttpUrlRequest request) {
            mRequestToCancelOnWrite = request;
        }
    }

    @LargeTest
    @Feature({"Cronet"})
    public void testNoWriteAfterCancelOnAnotherThread() throws Exception {
        // This test verifies that WritableByteChannel.write is not called after
        // WritableByteChannel.close if request is canceled from another
        // thread.
        for (int i = 0; i < 100; ++i) {
            HashMap<String, String> headers = new HashMap<String, String>();
            TestByteChannel channel = new TestByteChannel();
            TestHttpUrlRequestListener listener =
                    new TestHttpUrlRequestListener();

            // Create request.
            final HttpUrlRequest request =
                    mActivity.mRequestFactory.createRequest(
                            MockUrlRequestJobFactory.SUCCESS_URL,
                            HttpUrlRequest.REQUEST_PRIORITY_LOW, headers,
                            channel, listener);
            request.start();
            listener.blockForStart();
            Runnable cancelTask = new Runnable() {
                public void run() {
                    request.cancel();
                }
            };
            Executors.newCachedThreadPool().execute(cancelTask);
            listener.blockForComplete();
            assertFalse(channel.isOpen());
            // Since getAllHeaders and other methods in
            // checkAfterAdapterDestroyed() acquire mLock, so this will happen
            // after the adapter is destroyed.
            checkAfterAdapterDestroyed(request);
        }
    }

    @SuppressFBWarnings("DLS_DEAD_LOCAL_STORE")
    @SmallTest
    @Feature({"Cronet"})
    public void testNoWriteAfterSyncCancel() throws Exception {
        HashMap<String, String> headers = new HashMap<String, String>();
        TestByteChannel channel = new TestByteChannel();
        TestHttpUrlRequestListener listener = new TestHttpUrlRequestListener();

        String data = "MyBigFunkyData";
        int dataLength = data.length();
        int repeatCount = 10000;
        String mockUrl = mMockUrlRequestJobFactory.getMockUrlForData(data,
                repeatCount);

        // Create request.
        final HttpUrlRequest request =
                mActivity.mRequestFactory.createRequest(
                        mockUrl,
                        HttpUrlRequest.REQUEST_PRIORITY_LOW, headers,
                        channel, listener);
        // Channel will cancel the request from the network thread during the
        // first write.
        channel.setRequestToCancelOnWrite(request);
        request.start();
        listener.blockForComplete();
        assertTrue(request.isCanceled());
        assertFalse(channel.isOpen());
        // Test that ChromiumUrlRequest caches information which is available
        // after the native request adapter has been destroyed.
        assertEquals(-1, request.getHttpStatusCode());
        assertEquals("", request.getHttpStatusText());
        checkAfterAdapterDestroyed(request);
    }

    @SmallTest
    @Feature({"Cronet"})
    public void testBigDataSyncReadRequest() throws Exception {
        String data = "MyBigFunkyData";
        int dataLength = data.length();
        int repeatCount = 100000;
        String mockUrl = mMockUrlRequestJobFactory.getMockUrlForData(data,
                repeatCount);
        createRequestAndWaitForComplete(mockUrl, false);
        assertEquals(mockUrl, mListener.mUrl);
        for (int i = 0; i < repeatCount; ++i) {
            assertEquals(data, mListener.mResponseAsString.substring(
                    dataLength * i, dataLength * (i + 1)));
        }
    }
}
