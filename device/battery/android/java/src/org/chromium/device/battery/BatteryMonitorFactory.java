// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.device.battery;

import android.content.Context;
import android.util.Log;

import org.chromium.base.ThreadUtils;
import org.chromium.device.battery.BatteryStatusManager.BatteryStatusCallback;
import org.chromium.mojom.device.BatteryMonitor;
import org.chromium.mojom.device.BatteryStatus;

import java.util.HashSet;

/**
 * Factory that creates instances of BatteryMonitor implementations and notifies them about battery
 * status changes.
 */
public class BatteryMonitorFactory {
    static final String TAG = "BatteryMonitorFactory";

    // Backing source of battery information.
    private final BatteryStatusManager mManager;
    // Monitors currently interested in the battery status notifications.
    private final HashSet<BatteryMonitorImpl> mSubscribedMonitors =
            new HashSet<BatteryMonitorImpl>();

    private final BatteryStatusCallback mCallback = new BatteryStatusCallback() {
        @Override
        public void onBatteryStatusChanged(BatteryStatus batteryStatus) {
            ThreadUtils.assertOnUiThread();

            for (BatteryMonitorImpl monitor : mSubscribedMonitors) {
                monitor.didChange(batteryStatus);
            }
        }
    };

    public BatteryMonitorFactory(Context applicationContext) {
        assert applicationContext != null;
        mManager = new BatteryStatusManager(applicationContext, mCallback);
    }

    public BatteryMonitor createMonitor() {
        ThreadUtils.assertOnUiThread();

        if (mSubscribedMonitors.isEmpty() && !mManager.start()) {
            Log.e(TAG, "BatteryStatusManager failed to start.");
        }
        // TODO(ppi): record the "BatteryStatus.StartAndroid" histogram here once we have a Java API
        //            for UMA - http://crbug.com/442300.

        BatteryMonitorImpl monitor = new BatteryMonitorImpl(this);
        mSubscribedMonitors.add(monitor);
        return monitor;
    }

    void unsubscribe(BatteryMonitorImpl monitor) {
        ThreadUtils.assertOnUiThread();

        assert mSubscribedMonitors.contains(monitor);
        mSubscribedMonitors.remove(monitor);
        if (mSubscribedMonitors.isEmpty()) {
            mManager.stop();
        }
    }
}
