// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser.test.util;

import android.app.Instrumentation;
import android.os.SystemClock;
import android.view.KeyCharacterMap;
import android.view.KeyEvent;
import android.view.View;

import org.chromium.base.ThreadUtils;

/**
 * Collection of keyboard utilities.
 */
public class KeyUtils {
    /**
     * Sends (synchronously) a single key down/up pair of events to the specified view.
     * <p>
     * Does not use the event injecting framework, but instead relies on
     * {@link View#dispatchKeyEventPreIme(KeyEvent)} and {@link View#dispatchKeyEvent(KeyEvent)} of
     * the view itself
     * <p>
     * The event injecting framework will fail with a SecurityException if another window is
     * on top of Chrome ("Injecting to another application requires INJECT_EVENTS permission"). So,
     * we should use this instead of {@link android.test.InstrumentationTestCase#sendKeys(int...)}.
     *
     * @param i The application being instrumented.
     * @param v The view to receive the key event.
     * @param keyCode The keycode for the event to be issued.
     */
    public static void singleKeyEventView(Instrumentation i, final View v, int keyCode) {
        long downTime = SystemClock.uptimeMillis();
        long eventTime = SystemClock.uptimeMillis();

        final KeyEvent downEvent =
                new KeyEvent(downTime, eventTime, KeyEvent.ACTION_DOWN, keyCode, 0);
        dispatchKeyEvent(i, v, downEvent);

        downTime = SystemClock.uptimeMillis();
        eventTime = SystemClock.uptimeMillis();
        final KeyEvent upEvent =
                new KeyEvent(downTime, eventTime, KeyEvent.ACTION_UP, keyCode, 0);
        dispatchKeyEvent(i, v, upEvent);
    }

    /**
     * Types the given text (on character at a time) into the specified view.
     *
     * @param i The application being instrumented.
     * @param v The view to receive the text.
     * @param text The text to be input.
     */
    public static void typeTextIntoView(Instrumentation i, View v, String text) {
        KeyCharacterMap characterMap = KeyCharacterMap.load(KeyCharacterMap.VIRTUAL_KEYBOARD);
        KeyEvent[] events = characterMap.getEvents(text.toCharArray());
        for (KeyEvent event : events) {
            dispatchKeyEvent(i, v, event);
        }
    }

    private static void dispatchKeyEvent(final Instrumentation i, final View v,
            final KeyEvent event) {
        ThreadUtils.runOnUiThreadBlocking(new Runnable() {
            @Override
            public void run() {
                if (!v.dispatchKeyEventPreIme(event)) {
                    v.dispatchKeyEvent(event);
                }
            }
        });
        i.waitForIdleSync();
    }
}