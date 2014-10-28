// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.net;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.os.Process;
import android.util.Log;

import org.chromium.base.CalledByNative;
import org.chromium.base.JNINamespace;

/**
 * Provides context for the native HTTP operations.
 */
@JNINamespace("cronet")
public class ChromiumUrlRequestContext {
    private static final int LOG_NONE = 3;  // LOG(FATAL), no VLOG.
    private static final int LOG_DEBUG = -1;  // LOG(FATAL...INFO), VLOG(1)
    private static final int LOG_VERBOSE = -2;  // LOG(FATAL...INFO), VLOG(2)
    static final String LOG_TAG = "ChromiumNetwork";

    /**
     * Native adapter object, owned by ChromiumUrlRequestContext.
     */
    private long mChromiumUrlRequestContextAdapter;

    /**
     * Constructor.
     */
    protected ChromiumUrlRequestContext(final Context context, String userAgent,
            String config) {
        mChromiumUrlRequestContextAdapter = nativeCreateRequestContextAdapter(
                context, userAgent, getLoggingLevel(), config);
        if (mChromiumUrlRequestContextAdapter == 0) {
            throw new NullPointerException("Context Adapter creation failed");
        }
        // Post a task to UI thread to init native Chromium URLRequestContext.
        // TODO(xunjieli): This constructor is not supposed to be invoked on
        // the main thread. Consider making the following code into a blocking
        // API to handle the case where we are already on main thread.
        Runnable task = new Runnable() {
            public void run() {
                NetworkChangeNotifier.init(context);
                // Registers to always receive network notifications. Note that
                // this call is fine for Cronet because Cronet embedders do not
                // have API access to create network change observers. Existing
                // observers in the net stack do not perform expensive work.
                NetworkChangeNotifier.registerToReceiveNotificationsAlways();
                nativeInitRequestContextOnMainThread(
                        mChromiumUrlRequestContextAdapter);
            }
        };
        new Handler(Looper.getMainLooper()).post(task);
    }

    /**
     * Returns the version of this network stack formatted as N.N.N.N/X where
     * N.N.N.N is the version of Chromium and X is the revision number.
     */
    public static String getVersion() {
        return Version.getVersion();
    }

    /**
     * Initializes statistics recorder.
     */
    public void initializeStatistics() {
        nativeInitializeStatistics();
    }

    /**
     * Gets current statistics recorded since |initializeStatistics| with
     * |filter| as a substring as JSON text (an empty |filter| will include all
     * registered histograms).
     */
    public String getStatisticsJSON(String filter) {
        return nativeGetStatisticsJSON(filter);
    }

    /**
     * Starts NetLog logging to a file named |fileName| in the
     * application temporary directory. |fileName| must not be empty. Log level
     * is LOG_ALL_BUT_BYTES. If the file exists it is truncated before starting.
     * If actively logging the call is ignored.
     */
    public void startNetLogToFile(String fileName) {
        nativeStartNetLogToFile(mChromiumUrlRequestContextAdapter, fileName);
    }

    /**
     * Stops NetLog logging and flushes file to disk. If a logging session is
     * not in progress this call is ignored.
     */
    public void stopNetLog() {
        nativeStopNetLog(mChromiumUrlRequestContextAdapter);
    }

    @CalledByNative
    private void initNetworkThread() {
        Thread.currentThread().setName("ChromiumNet");
        Process.setThreadPriority(Process.THREAD_PRIORITY_BACKGROUND);
    }

    @Override
    protected void finalize() throws Throwable {
        nativeReleaseRequestContextAdapter(mChromiumUrlRequestContextAdapter);
        super.finalize();
    }

    protected long getChromiumUrlRequestContextAdapter() {
        return mChromiumUrlRequestContextAdapter;
    }

    /**
     * @return loggingLevel see {@link #LOG_NONE}, {@link #LOG_DEBUG} and
     *         {@link #LOG_VERBOSE}.
     */
    private int getLoggingLevel() {
        int loggingLevel;
        if (Log.isLoggable(LOG_TAG, Log.VERBOSE)) {
            loggingLevel = LOG_VERBOSE;
        } else if (Log.isLoggable(LOG_TAG, Log.DEBUG)) {
            loggingLevel = LOG_DEBUG;
        } else {
            loggingLevel = LOG_NONE;
        }
        return loggingLevel;
    }

    // Returns an instance ChromiumUrlRequestContextAdapter to be stored in
    // mChromiumUrlRequestContextAdapter.
    private native long nativeCreateRequestContextAdapter(Context context,
            String userAgent, int loggingLevel, String config);

    private native void nativeReleaseRequestContextAdapter(
            long chromiumUrlRequestContextAdapter);

    private native void nativeInitializeStatistics();

    private native String nativeGetStatisticsJSON(String filter);

    private native void nativeStartNetLogToFile(
            long chromiumUrlRequestContextAdapter, String fileName);

    private native void nativeStopNetLog(long chromiumUrlRequestContextAdapter);

    private native void nativeInitRequestContextOnMainThread(
            long chromiumUrlRequestContextAdapter);
}
