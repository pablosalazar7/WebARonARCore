// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.externalauth;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anyInt;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.inOrder;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verifyZeroInteractions;
import static org.mockito.Mockito.when;

import android.content.Context;

import org.chromium.base.test.util.Feature;
import org.chromium.testing.local.LocalRobolectricTestRunner;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.InOrder;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.annotation.Config;

/**
 * Robolectric tests for {@link ExternalAuthUtils}.
 */
@RunWith(LocalRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class ExternalAuthUtilsTest {
    private static final int ERR = 999;
    @Mock private Context mContext;
    @Mock private ExternalAuthUtils mExternalAuthUtils;
    @Mock private UserRecoverableErrorHandler mUserRecoverableErrorHandler;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
    }

    @Test
    @Feature({"GooglePlayServices"})
    public void testCanUseGooglePlayServicesSuccess() {
        when(mExternalAuthUtils.canUseGooglePlayServices(any(Context.class),
                     any(UserRecoverableErrorHandler.class))).thenCallRealMethod();
        when(mExternalAuthUtils.checkGooglePlayServicesAvailable(mContext)).thenReturn(ERR);
        when(mExternalAuthUtils.isSuccess(ERR)).thenReturn(true); // Not an error.
        assertTrue(mExternalAuthUtils.canUseGooglePlayServices(
                mContext, mUserRecoverableErrorHandler));
        verifyZeroInteractions(mUserRecoverableErrorHandler);

        // Verifying stubs can be an anti-pattern but here it is important to
        // test that the real method canUseGooglePlayServices did invoke these
        // methods, which subclasses are expected to be able to override.
        InOrder inOrder = inOrder(mExternalAuthUtils);
        inOrder.verify(mExternalAuthUtils).checkGooglePlayServicesAvailable(mContext);
        inOrder.verify(mExternalAuthUtils).isSuccess(ERR);
        inOrder.verify(mExternalAuthUtils, never()).isUserRecoverableError(anyInt());
        inOrder.verify(mExternalAuthUtils, never()).describeError(anyInt());
    }

    @Test
    @Feature({"GooglePlayServices"})
    public void testCanUseGooglePlayServicesNonUserRecoverableFailure() {
        when(mExternalAuthUtils.canUseGooglePlayServices(any(Context.class),
                     any(UserRecoverableErrorHandler.class))).thenCallRealMethod();
        when(mExternalAuthUtils.checkGooglePlayServicesAvailable(mContext)).thenReturn(ERR);
        when(mExternalAuthUtils.isSuccess(ERR)).thenReturn(false); // Is an error.
        when(mExternalAuthUtils.isUserRecoverableError(ERR)).thenReturn(false); // Non-recoverable
        assertFalse(mExternalAuthUtils.canUseGooglePlayServices(
                mContext, mUserRecoverableErrorHandler));
        verifyZeroInteractions(mUserRecoverableErrorHandler);

        // Verifying stubs can be an anti-pattern but here it is important to
        // test that the real method canUseGooglePlayServices did invoke these
        // methods, which subclasses are expected to be able to override.
        InOrder inOrder = inOrder(mExternalAuthUtils);
        inOrder.verify(mExternalAuthUtils).checkGooglePlayServicesAvailable(mContext);
        inOrder.verify(mExternalAuthUtils).isSuccess(ERR);
        inOrder.verify(mExternalAuthUtils).isUserRecoverableError(ERR);
    }

    @Test
    @Feature({"GooglePlayServices"})
    public void testCanUseGooglePlayServicesUserRecoverableFailure() {
        when(mExternalAuthUtils.canUseGooglePlayServices(any(Context.class),
                     any(UserRecoverableErrorHandler.class))).thenCallRealMethod();
        doNothing().when(mUserRecoverableErrorHandler).handleError(mContext, ERR);
        when(mExternalAuthUtils.checkGooglePlayServicesAvailable(mContext)).thenReturn(ERR);
        when(mExternalAuthUtils.isSuccess(ERR)).thenReturn(false); // Is an error.
        when(mExternalAuthUtils.isUserRecoverableError(ERR)).thenReturn(true); // Recoverable
        when(mExternalAuthUtils.describeError(anyInt())).thenReturn("unused"); // For completeness
        assertFalse(mExternalAuthUtils.canUseGooglePlayServices(
                mContext, mUserRecoverableErrorHandler));

        // Verifying stubs can be an anti-pattern but here it is important to
        // test that the real method canUseGooglePlayServices did invoke these
        // methods, which subclasses are expected to be able to override.
        InOrder inOrder = inOrder(mExternalAuthUtils, mUserRecoverableErrorHandler);
        inOrder.verify(mExternalAuthUtils).checkGooglePlayServicesAvailable(mContext);
        inOrder.verify(mExternalAuthUtils).isSuccess(ERR);
        inOrder.verify(mExternalAuthUtils).isUserRecoverableError(ERR);
        inOrder.verify(mUserRecoverableErrorHandler).handleError(mContext, ERR);
    }
}