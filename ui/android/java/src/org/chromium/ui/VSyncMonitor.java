// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.ui;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.view.Choreographer;
import android.view.WindowManager;

import org.chromium.base.TraceEvent;

/**
 * Notifies clients of the default displays's vertical sync pulses.
 */
public class VSyncMonitor {
    private static final long NANOSECONDS_PER_SECOND = 1000000000;
    private static final long NANOSECONDS_PER_MICROSECOND = 1000;

    private boolean mInsideVSync = false;

    // Conservative guess about vsync's consecutivity.
    // If true, next tick is guaranteed to be consecutive.
    private boolean mConsecutiveVSync = false;

    /**
     * VSync listener class
     */
    public interface Listener {
        /**
         * Called very soon after the start of the display's vertical sync period.
         * @param monitor The VSyncMonitor that triggered the signal.
         * @param vsyncTimeMicros Absolute frame time in microseconds.
         */
        public void onVSync(VSyncMonitor monitor, long vsyncTimeMicros);
    }

    private Listener mListener;

    // Display refresh rate as reported by the system.
    private long mRefreshPeriodNano;

    private boolean mHaveRequestInFlight;

    private final Choreographer mChoreographer;
    private final Choreographer.FrameCallback mVSyncFrameCallback;
    private long mGoodStartingPointNano;

    // If the monitor is activated after having been idle, we synthesize the first vsync to reduce
    // latency.
    private final Handler mHandler = new Handler();
    private final Runnable mSyntheticVSyncRunnable;
    private long mLastVSyncCpuTimeNano;

    /**
     * Constructs a VSyncMonitor
     * @param context The application context.
     * @param listener The listener receiving VSync notifications.
     */
    public VSyncMonitor(Context context, VSyncMonitor.Listener listener) {
        mListener = listener;
        float refreshRate = ((WindowManager) context.getSystemService(Context.WINDOW_SERVICE))
                .getDefaultDisplay().getRefreshRate();
        final boolean useEstimatedRefreshPeriod = refreshRate < 30;

        if (refreshRate <= 0) refreshRate = 60;
        mRefreshPeriodNano = (long) (NANOSECONDS_PER_SECOND / refreshRate);

        mChoreographer = Choreographer.getInstance();
        mVSyncFrameCallback = new Choreographer.FrameCallback() {
            @Override
            public void doFrame(long frameTimeNanos) {
                TraceEvent.begin("VSync");
                mHandler.removeCallbacks(mSyntheticVSyncRunnable);
                if (useEstimatedRefreshPeriod && mConsecutiveVSync) {
                    // Display.getRefreshRate() is unreliable on some platforms.
                    // Adjust refresh period- initial value is based on Display.getRefreshRate()
                    // after that it asymptotically approaches the real value.
                    long lastRefreshDurationNano = frameTimeNanos - mGoodStartingPointNano;
                    float lastRefreshDurationWeight = 0.1f;
                    mRefreshPeriodNano += (long) (lastRefreshDurationWeight
                            * (lastRefreshDurationNano - mRefreshPeriodNano));
                }
                mGoodStartingPointNano = frameTimeNanos;
                onVSyncCallback(frameTimeNanos, getCurrentNanoTime());
                TraceEvent.end("VSync");
            }
        };
        mSyntheticVSyncRunnable = new Runnable() {
            @Override
            public void run() {
                TraceEvent.begin("VSyncSynthetic");
                mChoreographer.removeFrameCallback(mVSyncFrameCallback);
                final long currentTime = getCurrentNanoTime();
                onVSyncCallback(estimateLastVSyncTime(currentTime), currentTime);
                TraceEvent.end("VSyncSynthetic");
            }
        };
        mGoodStartingPointNano = getCurrentNanoTime();
    }

    /**
     * Returns the time interval between two consecutive vsync pulses in microseconds.
     */
    public long getVSyncPeriodInMicroseconds() {
        return mRefreshPeriodNano / NANOSECONDS_PER_MICROSECOND;
    }

    /**
     * Request to be notified of the closest display vsync events. This should
     * always be called on the same thread used to create the VSyncMonitor.
     * Listener.onVSync() will be called soon after the upcoming vsync pulses.
     */
    public void requestUpdate() {
        assert mHandler.getLooper() == Looper.myLooper();
        postCallback();
    }

    /**
     * @return true if onVSync handler is executing. If onVSync handler
     * introduces invalidations, View#invalidate() should be called. If
     * View#postInvalidateOnAnimation is called instead, the corresponding onDraw
     * will be delayed by one frame. The embedder of VSyncMonitor should check
     * this value if it wants to post an invalidation.
     */
    public boolean isInsideVSync() {
        return mInsideVSync;
    }

    private long getCurrentNanoTime() {
        return System.nanoTime();
    }

    private void onVSyncCallback(long frameTimeNanos, long currentTimeNanos) {
        assert mHaveRequestInFlight;
        mInsideVSync = true;
        mHaveRequestInFlight = false;
        mLastVSyncCpuTimeNano = currentTimeNanos;
        try {
            if (mListener != null) {
                mListener.onVSync(this, frameTimeNanos / NANOSECONDS_PER_MICROSECOND);
            }
        } finally {
            mInsideVSync = false;
        }
    }

    private void postCallback() {
        if (mHaveRequestInFlight) return;
        mHaveRequestInFlight = true;
        mConsecutiveVSync = mInsideVSync;
        // There's no way to tell if we're currently in the scope of a
        // choregrapher frame callback, which might in turn allow us to honor
        // the vsync callback in the current frame. Thus, we eagerly post the
        // frame callback even when we post a synthetic frame callback. If the
        // frame callback is honored before the synthetic callback, we simply
        // remove the synthetic callback.
        postSyntheticVSyncIfNecessary();
        mChoreographer.postFrameCallback(mVSyncFrameCallback);
    }

    private void postSyntheticVSyncIfNecessary() {
        // TODO(jdduke): Consider removing synthetic vsyncs altogether if
        // they're found to be no longer necessary.
        final long currentTime = getCurrentNanoTime();
        // Only trigger a synthetic vsync if we've been idle for long enough and the upcoming real
        // vsync is more than half a frame away.
        if (currentTime - mLastVSyncCpuTimeNano < 2 * mRefreshPeriodNano) return;
        if (currentTime - estimateLastVSyncTime(currentTime) > mRefreshPeriodNano / 2) return;
        mHandler.post(mSyntheticVSyncRunnable);
    }

    private long estimateLastVSyncTime(long currentTime) {
        final long lastRefreshTime = mGoodStartingPointNano
                + ((currentTime - mGoodStartingPointNano) / mRefreshPeriodNano)
                * mRefreshPeriodNano;
        return lastRefreshTime;
    }
}
