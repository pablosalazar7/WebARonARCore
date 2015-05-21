// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.webview_shell;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;

import android.view.KeyEvent;
import android.view.MenuItem;
import android.view.View;
import android.view.View.OnKeyListener;
import android.view.inputmethod.InputMethodManager;

import android.webkit.GeolocationPermissions;
import android.webkit.PermissionRequest;
import android.webkit.WebChromeClient;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;

import android.widget.EditText;
import android.widget.LinearLayout.LayoutParams;
import android.widget.PopupMenu;
import android.widget.TextView;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * This activity is designed for starting a "mini-browser" for manual testing of WebView.
 * It takes an optional URL as an argument, and displays the page. There is a URL bar
 * on top of the webview for manually specifying URLs to load.
 */
public class WebViewBrowserActivity extends Activity implements PopupMenu.OnMenuItemClickListener {
    private EditText mUrlBar;
    private WebView mWebView;
    private String mWebViewVersion;

    private static final Pattern WEBVIEW_VERSION_PATTERN =
            Pattern.compile("(Chrome/)([\\d\\.]+)\\s");

    private static final String[] AUTOMATICALLY_GRANT =
            { PermissionRequest.RESOURCE_VIDEO_CAPTURE, PermissionRequest.RESOURCE_AUDIO_CAPTURE };

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_webview_browser);
        mWebView = (WebView) findViewById(R.id.webview);
        WebSettings settings = mWebView.getSettings();
        initializeSettings(settings);

        Matcher matcher = WEBVIEW_VERSION_PATTERN.matcher(settings.getUserAgentString());
        if (matcher.find()) {
            mWebViewVersion = matcher.group(2);
        } else {
            mWebViewVersion = "-";
        }
        setTitle(getResources().getString(R.string.title_activity_browser) + " " + mWebViewVersion);

        mWebView.setWebViewClient(new WebViewClient() {
            @Override
            public boolean shouldOverrideUrlLoading(WebView webView, String url) {
                return false;
            }
        });

        mWebView.setWebChromeClient(new WebChromeClient() {
            @Override
            public void onGeolocationPermissionsShowPrompt(String origin,
                    GeolocationPermissions.Callback callback) {
                callback.invoke(origin, true, false);
            }

            @Override
            public void onPermissionRequest(PermissionRequest request) {
                request.grant(AUTOMATICALLY_GRANT);
            }
        });

        mUrlBar = (EditText) findViewById(R.id.url_field);
        mUrlBar.setOnKeyListener(new OnKeyListener() {
            public boolean onKey(View view, int keyCode, KeyEvent event) {
                if (keyCode == KeyEvent.KEYCODE_ENTER && event.getAction() == KeyEvent.ACTION_UP) {
                    loadUrlFromUrlBar(view);
                    return true;
                }
                return false;
            }
        });

        String url = getUrlFromIntent(getIntent());
        if (url != null) {
            mUrlBar.setText(url, TextView.BufferType.EDITABLE);
            loadUrl(url);
        }
    }

    public void loadUrlFromUrlBar(View view) {
        loadUrl(mUrlBar.getText().toString());
        hideKeyboard(mUrlBar);
    }

    public void showPopup(View v) {
        PopupMenu popup = new PopupMenu(this, v);
        popup.setOnMenuItemClickListener(this);
        popup.inflate(R.menu.main_menu);
        popup.show();
    }

    @Override
    public boolean onMenuItemClick(MenuItem item) {
        switch(item.getItemId()) {
            case R.id.menu_about:
                about();
                hideKeyboard(mUrlBar);
                return true;
            default:
                return false;
        }
    }

    private void initializeSettings(WebSettings settings) {
        settings.setJavaScriptEnabled(true);
        settings.setGeolocationEnabled(true);
    }

    private void about() {
        WebSettings settings = mWebView.getSettings();
        StringBuilder summary = new StringBuilder();
        summary.append("WebView version : " + mWebViewVersion + "\n");

        for (Method method : settings.getClass().getMethods()) {
            if (!methodIsSimpleInspector(method)) continue;
            try {
                summary.append(method.getName() + " : " + method.invoke(settings) + "\n");
            } catch (IllegalAccessException e) {
            } catch (InvocationTargetException e) { }
        }

        AlertDialog dialog = new AlertDialog.Builder(this)
                .setTitle(getResources().getString(R.string.menu_about))
                .setMessage(summary)
                .setPositiveButton("OK", null)
                .create();
        dialog.show();
        dialog.getWindow().setLayout(LayoutParams.FILL_PARENT, LayoutParams.FILL_PARENT);
    }

    // Returns true is a method has no arguments and returns either a boolean or a String.
    private boolean methodIsSimpleInspector(Method method) {
        Class<?> returnType = method.getReturnType();
        return ((returnType.equals(boolean.class) || returnType.equals(String.class))
                && method.getParameterTypes().length == 0);
    }

    private void loadUrl(String url) {
        mWebView.loadUrl(url);
        mWebView.requestFocus();
    }

    /**
     * Hides the keyboard.
     * @param view The {@link View} that is currently accepting input.
     * @return Whether the keyboard was visible before.
     */
    private static boolean hideKeyboard(View view) {
        InputMethodManager imm = (InputMethodManager) view.getContext().getSystemService(
                Context.INPUT_METHOD_SERVICE);
        return imm.hideSoftInputFromWindow(view.getWindowToken(), 0);
    }

    private static String getUrlFromIntent(Intent intent) {
        return intent != null ? intent.getDataString() : null;
    }
}
