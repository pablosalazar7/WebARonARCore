// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_webview;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.Picture;
import android.net.Uri;
import android.net.http.SslError;
import android.os.Looper;
import android.os.Message;
import android.view.KeyEvent;
import android.view.View;
import android.webkit.ConsoleMessage;
import android.webkit.GeolocationPermissions;
import android.webkit.ValueCallback;
import android.webkit.WebChromeClient;

import org.chromium.android_webview.permission.AwPermissionRequest;
import org.chromium.base.annotations.SuppressFBWarnings;

import java.security.Principal;
import java.util.HashMap;

/**
 * Base-class that an AwContents embedder derives from to receive callbacks.
 * This extends ContentViewClient, as in many cases we want to pass-thru ContentViewCore
 * callbacks right to our embedder, and this setup facilities that.
 * For any other callbacks we need to make transformations of (e.g. adapt parameters
 * or perform filtering) we can provide final overrides for methods here, and then introduce
 * new abstract methods that the our own client must implement.
 * i.e.: all methods in this class should either be final, or abstract.
 */
public abstract class AwContentsClient {

    private final AwContentsClientCallbackHelper mCallbackHelper;

    // Last background color reported from the renderer. Holds the sentinal value INVALID_COLOR
    // if not valid.
    private int mCachedRendererBackgroundColor = INVALID_COLOR;

    private static final int INVALID_COLOR = 0;

    public AwContentsClient() {
        this(Looper.myLooper());
    }

    // Alllow injection of the callback thread, for testing.
    public AwContentsClient(Looper looper) {
        mCallbackHelper = new AwContentsClientCallbackHelper(looper, this);
    }

    final AwContentsClientCallbackHelper getCallbackHelper() {
        return mCallbackHelper;
    }

    final int getCachedRendererBackgroundColor() {
        assert isCachedRendererBackgroundColorValid();
        return mCachedRendererBackgroundColor;
    }

    final boolean isCachedRendererBackgroundColorValid() {
        return mCachedRendererBackgroundColor != INVALID_COLOR;
    }

    final void onBackgroundColorChanged(int color) {
        // Avoid storing the sentinal INVALID_COLOR (note that both 0 and 1 are both
        // fully transparent so this transpose makes no visible difference).
        mCachedRendererBackgroundColor = color == INVALID_COLOR ? 1 : color;
    }

    //--------------------------------------------------------------------------------------------
    //             WebView specific methods that map directly to WebViewClient / WebChromeClient
    //--------------------------------------------------------------------------------------------

    /**
     * Parameters for the {@link AwContentsClient#shouldInterceptRequest} method.
     */
    @SuppressFBWarnings("URF_UNREAD_PUBLIC_OR_PROTECTED_FIELD")
    public static class AwWebResourceRequest {
        // Url of the request.
        public String url;
        // Is this for the main frame or a child iframe?
        public boolean isMainFrame;
        // Was a gesture associated with the request? Don't trust can easily be spoofed.
        public boolean hasUserGesture;
        // Method used (GET/POST/OPTIONS)
        public String method;
        // Headers that would have been sent to server.
        public HashMap<String, String> requestHeaders;
    }

    /**
     * Parameters for {@link AwContentsClient#onReceivedError} method.
     */
    public static class AwWebResourceError {
        public int errorCode = ErrorCodeConversionHelper.ERROR_UNKNOWN;
        public String description;
    }

    /**
     * Allow default implementations in chromium code.
     */
    public abstract boolean hasWebViewClient();

    public abstract void getVisitedHistory(ValueCallback<String[]> callback);

    public abstract void doUpdateVisitedHistory(String url, boolean isReload);

    public abstract void onProgressChanged(int progress);

    public abstract AwWebResourceResponse shouldInterceptRequest(
            AwWebResourceRequest request);

    public abstract boolean shouldOverrideKeyEvent(KeyEvent event);

    public abstract boolean shouldOverrideUrlLoading(String url);

    public abstract void onLoadResource(String url);

    public abstract void onUnhandledKeyEvent(KeyEvent event);

    public abstract boolean onConsoleMessage(ConsoleMessage consoleMessage);

    public abstract void onReceivedHttpAuthRequest(AwHttpAuthHandler handler,
            String host, String realm);

    public abstract void onReceivedSslError(ValueCallback<Boolean> callback, SslError error);

    public abstract void onReceivedClientCertRequest(
            final AwContentsClientBridge.ClientCertificateRequestCallback callback,
            final String[] keyTypes, final Principal[] principals, final String host,
            final int port);

    public abstract void onReceivedLoginRequest(String realm, String account, String args);

    public abstract void onFormResubmission(Message dontResend, Message resend);

    public abstract void onDownloadStart(String url, String userAgent, String contentDisposition,
            String mimeType, long contentLength);

    public static Uri[] parseFileChooserResult(int resultCode, Intent intent) {
        if (resultCode == Activity.RESULT_CANCELED) {
            return null;
        }
        Uri result =
                intent == null || resultCode != Activity.RESULT_OK ? null : intent.getData();

        Uri[] uris = null;
        if (result != null) {
            uris = new Uri[1];
            uris[0] = result;
        }
        return uris;
    }

    /**
     * Type adaptation class for FileChooserParams.
     */
    @SuppressLint("NewApi")  // WebChromeClient.FileChooserParams requires API level 21.
    public static class FileChooserParamsImpl extends WebChromeClient.FileChooserParams {
        private int mMode;
        private String mAcceptTypes;
        private String mTitle;
        private String mDefaultFilename;
        private boolean mCapture;

        public FileChooserParamsImpl(int mode, String acceptTypes, String title,
                String defaultFilename, boolean capture) {
            mMode = mode;
            mAcceptTypes = acceptTypes;
            mTitle = title;
            mDefaultFilename = defaultFilename;
            mCapture = capture;
        }

        public String getAcceptTypesString() {
            return mAcceptTypes;
        }

        @Override
        public int getMode() {
            return mMode;
        }

        @Override
        public String[] getAcceptTypes() {
            if (mAcceptTypes == null) {
                return new String[0];
            }
            return mAcceptTypes.split(";");
        }

        @Override
        public boolean isCaptureEnabled() {
            return mCapture;
        }

        @Override
        public CharSequence getTitle() {
            return mTitle;
        }

        @Override
        public String getFilenameHint() {
            return mDefaultFilename;
        }

        @Override
        public Intent createIntent() {
            // TODO: Move this code to Aw. Once code is moved
            // and merged to M37 get rid of this.
            String mimeType = "*/*";
            if (mAcceptTypes != null && !mAcceptTypes.trim().isEmpty())
                mimeType = mAcceptTypes.split(";")[0];

            Intent i = new Intent(Intent.ACTION_GET_CONTENT);
            i.addCategory(Intent.CATEGORY_OPENABLE);
            i.setType(mimeType);
            return i;
        }
    }

    public abstract void showFileChooser(ValueCallback<String[]> uploadFilePathsCallback,
            FileChooserParamsImpl fileChooserParams);

    public abstract void onGeolocationPermissionsShowPrompt(String origin,
            GeolocationPermissions.Callback callback);

    public abstract void onGeolocationPermissionsHidePrompt();

    public abstract void onPermissionRequest(AwPermissionRequest awPermissionRequest);

    public abstract void onPermissionRequestCanceled(AwPermissionRequest awPermissionRequest);

    public abstract void onScaleChangedScaled(float oldScale, float newScale);

    protected abstract void handleJsAlert(String url, String message, JsResultReceiver receiver);

    protected abstract void handleJsBeforeUnload(String url, String message,
            JsResultReceiver receiver);

    protected abstract void handleJsConfirm(String url, String message, JsResultReceiver receiver);

    protected abstract void handleJsPrompt(String url, String message, String defaultValue,
            JsPromptResultReceiver receiver);

    protected abstract boolean onCreateWindow(boolean isDialog, boolean isUserGesture);

    protected abstract void onCloseWindow();

    public abstract void onReceivedTouchIconUrl(String url, boolean precomposed);

    public abstract void onReceivedIcon(Bitmap bitmap);

    public abstract void onReceivedTitle(String title);

    protected abstract void onRequestFocus();

    protected abstract View getVideoLoadingProgressView();

    public abstract void onPageStarted(String url);

    public abstract void onPageFinished(String url);

    public abstract void onPageCommitVisible(String url);

    public final void onReceivedError(AwWebResourceRequest request, AwWebResourceError error) {
        // Only one of these callbacks actually reaches out the client code. The first callback
        // is used on API versions up to and including L, the second on subsequent releases.
        // Below is the calls diagram:
        //
        //                           Old (<= L) glue              Old (<= L) android.webkit API
        //                             onReceivedError --------->   onReceivedError
        //  AwContentsClient           onReceivedError2 ->X   /
        //   abs. onReceivedError                            /
        //   abs. onReceivedError2                          /
        //                           New (M+) glue         /      New (M+) android.webkit API
        //                             onReceivedError    /     ->  onReceviedError <new>
        //   "->X" = "do nothing"        if (!<M API>) ---     /      if (isMainFrame) -\
        //                               else ->X             /       else ->X          |
        //                             onReceivedError2      /                          V
        //                               if (<M API>) -------       onReceivedError <old>
        //                               else ->X
        if (request.isMainFrame) {
            onReceivedError(error.errorCode, error.description, request.url);
        }
        onReceivedError2(request, error);
    }

    protected abstract void onReceivedError(int errorCode, String description, String failingUrl);

    protected abstract void onReceivedError2(
            AwWebResourceRequest request, AwWebResourceError error);

    public abstract void onReceivedHttpError(AwWebResourceRequest request,
            AwWebResourceResponse response);

    public abstract void onShowCustomView(View view, WebChromeClient.CustomViewCallback callback);

    public abstract void onHideCustomView();

    public abstract Bitmap getDefaultVideoPoster();

    //--------------------------------------------------------------------------------------------
    //                              Other WebView-specific methods
    //--------------------------------------------------------------------------------------------
    //
    public abstract void onFindResultReceived(int activeMatchOrdinal, int numberOfMatches,
            boolean isDoneCounting);

    /**
     * Called whenever there is a new content picture available.
     * @param picture New picture.
     */
    public abstract void onNewPicture(Picture picture);

}
